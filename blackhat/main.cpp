#include <Windows.h>
#include "hack.h"

DWORD WINAPI HackThread(LPVOID lpArg)
{
	g_Hack.Init((HMODULE)lpArg);
	return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CreateThread(NULL, 0, HackThread, hInst, 0, NULL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		g_Hack.Exit();
	}
	return TRUE;
}