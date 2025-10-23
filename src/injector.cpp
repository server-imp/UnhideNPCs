#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdio>
#include <string>
#include <filesystem>

#define ModuleName ("UnhideNPCs.dll")
#define TargetName ("Gw2-64.exe")

DWORD getPidByName(const char* name)
{
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (strcmp(entry.szExeFile, name) == 0)
            {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        }
    }

    CloseHandle(snapshot);
    return 0;
}

bool findModule(const DWORD dwPid, const std::string& name)
{
    MODULEENTRY32 entry;
    entry.dwSize = sizeof(entry);

    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPid);

    if (Module32First(snapshot, &entry))
    {
        do
        {
            if (entry.szModule == name)
            {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return false;
}

int main()
{
    namespace fs = std::filesystem;
    const auto path = fs::absolute(fs::path(ModuleName));

    if (!fs::exists(path))
    {
        printf("%s missing!\n", ModuleName);
        system("pause");
        return 1;
    }

    const DWORD pid = getPidByName(TargetName);

    if (pid == 0)
    {
        printf("%s not running!\n", TargetName);
        system("pause");
        return 2;
    }

    if (findModule(pid, "UnhideNPCs"))
    {
        printf("%s already loaded!\n", ModuleName);
        system("pause");
        return 3;
    }

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc)
    {
        printf("OpenProcess failed: %ld\n", GetLastError());
        system("pause");
        return 4;
    }

    const auto kernel32     = GetModuleHandleA("kernel32.dll");
    const auto loadLibraryA = reinterpret_cast<LPVOID>(GetProcAddress(kernel32, "LoadLibraryA"));

    const auto buffer = VirtualAllocEx
        (hProc, nullptr, strlen(path.string().c_str()), MEM_COMMIT, PAGE_EXECUTE_READWRITE);

    if (!buffer)
    {
        printf("VirtualAllocEx failed: %ld\n", GetLastError());
        CloseHandle(hProc);
        system("PAUSE");
        return 5;
    }

    if (!WriteProcessMemory(hProc, buffer, path.string().c_str(), strlen(path.string().c_str()), nullptr))
    {
        printf("WriteProcessMemory failed: %ld\n", GetLastError());
        VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("PAUSE");
        return 6;
    }

    const auto hThread = CreateRemoteThread
        (hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryA), buffer, 0, nullptr);
    if (hThread == nullptr)
    {
        printf("CreateRemoteThread failed: %ld\n", GetLastError());
        VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("PAUSE");
        return 7;
    }

    DWORD waitResult = WaitForSingleObject(hThread, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        printf("WaitForSingleObject failed: %lu\n", GetLastError());
        CloseHandle(hThread);
        VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("PAUSE");
        return 8;
    }

    DWORD_PTR threadResult = 0;
    if (!GetExitCodeThread(hThread, reinterpret_cast<LPDWORD>(&threadResult)))
    {
        printf("GetExitCodeThread failed: %lu\n", GetLastError());
        CloseHandle(hThread);
        VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("PAUSE");
        return 9;
    }

    if (threadResult == 0)
    {
        printf("Remote thread LoadLibrary failed\n");
        CloseHandle(hThread);
        VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
        CloseHandle(hProc);
        system("PAUSE");
        return 10;
    }

    CloseHandle(hThread);
    VirtualFreeEx(hProc, buffer, 0, MEM_RELEASE);
    CloseHandle(hProc);

    printf("OK\n");
    return 0;
}
