#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <string.h>

#define PROCESS_NAME "hl2.exe"
#define MODULE_NAME "blackhat.dll"

bool IsFileExists(const char* pFile)
{
	HANDLE hFile = CreateFile(pFile, GENERIC_READ, FILE_SHARE_READ, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		return true;
	}
	return false;
}

#define INVALID_PID (DWORD)-1

DWORD FindProcess(const char* pExeName)
{
	static DWORD s_dwProcessList[1024];

	DWORD cbNeeded;
	EnumProcesses(s_dwProcessList, sizeof(s_dwProcessList), &cbNeeded);
	for (DWORD i = 0; i < cbNeeded / sizeof(DWORD); i++)
	{
		char szPath[MAX_PATH];
		DWORD dwChars = sizeof(szPath);
		DWORD dwPid = s_dwProcessList[i];
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION 
			| PROCESS_QUERY_INFORMATION, 
		FALSE, dwPid);
		if (hProcess == INVALID_HANDLE_VALUE)
			continue;
		QueryFullProcessImageName(hProcess, 0, szPath, &dwChars);
		CloseHandle(hProcess);
		if (strstr(szPath, pExeName))
			return dwPid;
	}
	return INVALID_PID;
}

void Inject(HANDLE hProcess, const char* pModulePath)
{
	DWORD_PTR dwFunc = (DWORD_PTR)GetProcAddress(
		GetModuleHandle("kernel32.dll"), "LoadLibraryA");
	DWORD dwPathSize = strlen(pModulePath) + 1;
	LPVOID lpFullPath = VirtualAllocEx(hProcess, NULL, dwPathSize, 
		MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	DWORD dwWrite;
	WriteProcessMemory(hProcess, lpFullPath, pModulePath, (SIZE_T)dwPathSize, &dwWrite);
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)dwFunc, lpFullPath, 0, NULL);
	WaitForSingleObject(hThread,INFINITE);
	CloseHandle(hThread);
	VirtualFreeEx(hProcess, lpFullPath, dwPathSize, MEM_DECOMMIT | MEM_RELEASE);
}

int main()
{
	if (!IsFileExists(MODULE_NAME))
	{
		fprintf(stderr, "%s doesn't exists!\n", MODULE_NAME);
		return 1;
	}

	char szFullPath[MAX_PATH];
	GetFullPathName(MODULE_NAME, MAX_PATH, szFullPath, NULL);

	DWORD dwPid;
	printf("Waiting for %s\n", PROCESS_NAME);
	while ((dwPid = FindProcess(PROCESS_NAME)) == INVALID_PID)
	{
		Sleep(100);
	}

	printf("%d\t%s\n", dwPid, PROCESS_NAME);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "Failed to open %s!\n", PROCESS_NAME);
		return 1;
	}
	
	Inject(hProcess, szFullPath);
	printf("Injected.");

	CloseHandle(hProcess);
	return 0;
}