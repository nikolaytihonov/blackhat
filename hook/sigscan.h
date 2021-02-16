#ifndef __SIGSCAN_H
#define __SIGSCAN_H

#include <Windows.h>

class SigScan
{
public:
	static LPVOID FindFunction(HMODULE hDll, const char* pSig, const char* pMask);
};

#endif