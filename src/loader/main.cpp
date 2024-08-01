#include <Windows.h>
#include <Shlwapi.h>
#include <tlhelp32.h>

#include <string.h>
#include <stdio.h>
#include <filesystem>
#include <string>

#include <wininternal.h>

bool CheckMutex(const char* mutex_name)
{
	auto result = OpenMutexA(SYNCHRONIZE, FALSE, mutex_name);
	if (result) {
		CloseHandle(result);
		return true;
	}
	return false;
}

bool CheckIfAnyGame()
{
	if (CheckMutex("Touhou Koumakyou App"))
		return true;
	return false;
}


DWORD attachToProcess(HANDLE hProc, const wchar_t* const dllName) {
	LPVOID remoteMem = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!remoteMem) {
		return 0;
	}

	DWORD byteRet;
	WriteProcessMemory(hProc, remoteMem, dllName, wcslen(dllName) * sizeof(wchar_t), &byteRet);

	HANDLE hRemoteThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(&LoadLibraryW), remoteMem, 0, nullptr);
	if (!hRemoteThread) {
		fprintf_s(stderr, "CreateRemoteThread failed: %ul", GetLastError());
		return 0;
	}

	DWORD dllAddr;

	WaitForSingleObject(hRemoteThread, INFINITE);
	GetExitCodeThread(hRemoteThread, &dllAddr);
	CloseHandle(hRemoteThread);

	VirtualFreeEx(hProc, remoteMem, MAX_PATH, MEM_DECOMMIT | MEM_RELEASE);

	printf("EveryVersus.dll injected at 0x%8x\n", dllAddr);

	return dllAddr;
}

bool injectIntoRunning() {

	bool hasPrompted = false;

	if (!CheckIfAnyGame()) {
		return false;
	}

	PROCESSENTRY32W entry = {};
	entry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32FirstW(snapshot, &entry)) {
		do {
			if (wcscmp(L"東方紅魔郷.exe", entry.szExeFile) == 0 || wcscmp(L"th06.exe", entry.szExeFile) == 0) {
				HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, entry.th32ProcessID);

				wchar_t dllName[MAX_PATH + 1] = {};
				GetCurrentDirectoryW(MAX_PATH, dllName);
				PathAppendW(dllName, L"EveryVersus.dll");
				
				attachToProcess(hProc, dllName);

				CloseHandle(snapshot);
				return true;

			}
		} while (Process32NextW(snapshot, &entry));
	}
	CloseHandle(snapshot);
	return false;

}

int wmain(int argc, wchar_t** argv) {
	wchar_t* exeName = argv[argc - 1];
	DWORD byteRet = 0;
	wchar_t dllName[MAX_PATH + 1] = {};
	wchar_t* exeDir;
	STARTUPINFOW si = { .cb = sizeof(si) };
	LPVOID remoteMem;
	PROCESS_INFORMATION pi = {};
	DWORD dllAddr = 0;

	if (injectIntoRunning()) {
		goto ending;
	}

	if (argc < 2) {
		return EXIT_FAILURE;
	}

	GetCurrentDirectoryW(MAX_PATH, dllName);
	PathAppendW(dllName, L"EveryVersus.dll");	
	
	exeDir = _wcsdup(exeName);
	PathRemoveFileSpecW(exeDir);

	CreateProcessW(exeName, nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, exeDir, &si, &pi);

	attachToProcess(pi.hProcess, dllName);
	ResumeThread(pi.hThread);
ending:

#if 1
	HANDLE hInputPipe = CreateFileW(L"\\\\.\\pipe\\EveryVersus_input", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	HANDLE hOutputPipe = CreateFileW(L"\\\\.\\pipe\\EveryVersus_output", GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);

	char PipeMsg[2048] = {};

	while (ReadFile(hOutputPipe, PipeMsg, sizeof(PipeMsg), &byteRet, nullptr)) {
		printf("Recieved %d bytes\n", byteRet);
		WriteFile(hInputPipe, PipeMsg, byteRet, &byteRet, nullptr);
	}
#endif
	return 0;
}