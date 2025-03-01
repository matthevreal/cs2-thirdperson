
// dump offsets and replace them before using this
// made by mateusz thx for using


#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>


// offsets
const DWORD64 CInput_OffsetFromModule = 0x8CC5B8;  
const DWORD64 ThirdPerson_Offset = 0x251;          

// get the pid
DWORD GetProcessId(const wchar_t* processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create snapshot." << std::endl;
        return 0;
    }

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (wcscmp(processEntry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return 0;
}

// get module base
DWORD64 GetModuleBase(DWORD pid, const wchar_t* modName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    MODULEENTRY32W modEntry;
    modEntry.dwSize = sizeof(modEntry);

    for (BOOL ok = Module32FirstW(hSnap, &modEntry); ok; ok = Module32NextW(hSnap, &modEntry)) {
        if (wcscmp(modEntry.szModule, modName) == 0) {
            CloseHandle(hSnap);
            return (DWORD64)modEntry.modBaseAddr;
        }
    }

    CloseHandle(hSnap);
    return 0;
}

// toggle function
void toggleThirdPerson(HANDLE hProcess, DWORD64 baseAddress) {
    // calculating the thirdperson offset
    DWORD64 thirdPersonAddress = baseAddress + CInput_OffsetFromModule + ThirdPerson_Offset;

    // read the third person flag
    bool thirdPerson;
    ReadProcessMemory(hProcess, (LPVOID)thirdPersonAddress, &thirdPerson, sizeof(thirdPerson), NULL);

    // toggle the value
    thirdPerson = !thirdPerson;

    // write the new value
    WriteProcessMemory(hProcess, (LPVOID)thirdPersonAddress, &thirdPerson, sizeof(thirdPerson), NULL);
}

int main() {
    // get pid + added error handling
    DWORD processId = GetProcessId(L"cs2.exe");
    if (processId == 0) {
        std::cerr << "CS2 process not found." << std::endl;
        return 1;
    }

    // open handle
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (hProcess == NULL) {
        std::cerr << "Failed to open process." << std::endl;
        return 1;
    }

    // get base adress from client.dll
    DWORD64 baseAddress = GetModuleBase(processId, L"client.dll");
    if (baseAddress == 0) {
        std::cerr << "Failed to get module base address." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    // toggle the thing
    toggleThirdPerson(hProcess, baseAddress);

    // close handle
    CloseHandle(hProcess);

    // success message
    std::cout << "Third-person view toggled." << std::endl;
    return 0;
}