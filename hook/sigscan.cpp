#include "sigscan.h"

LPVOID SigScan::FindFunction(HMODULE hDll, const char* pSig, const char* pMask)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)hDll;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)
		((char*)pDos + pDos->e_lfanew);
	char* pCode = (char*)pDos + pNt->OptionalHeader.BaseOfCode;
	DWORD dwCodeLen = pNt->OptionalHeader.SizeOfCode;
	DWORD dwSigLen = strlen(pMask);

	for (DWORD i = 0; i < dwCodeLen - dwSigLen; i++)
	{
		DWORD j;
		for (j = 0; j < dwSigLen; j++)
		{
			if (pCode[i + j] != pSig[j] && pMask[j] != '?')
				break;
		}
		if (j == dwSigLen)
			return pCode + i;
	}
	return NULL;
}
