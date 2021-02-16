#include "hook.h"
#include "MinHook.h"

Hook* Hook::s_pFirst = NULL;
Hook* Hook::s_pLast = NULL;

void Hook::Init()
{
	MH_Initialize();
}

void Hook::Exit()
{
	UnHookAll();
	MH_Uninitialize();
}

void Hook::UnHookAll()
{
	Hook* pItem = s_pFirst;
	if (pItem)
	{
		Hook* pNext;
		do {
			pNext = pItem->m_pNext;
			
			pItem->DoUnHook();
		} while (pItem = pNext);
	}

	s_pFirst = NULL;
	s_pLast = NULL;
}

void Hook::DoHook()
{
	m_pPrev = s_pLast;

	if (!s_pFirst) s_pFirst = this;
	if (s_pLast) s_pLast->m_pNext = this;

	s_pLast = this;
	//Delete!
	delete this;
}

void Hook::DoUnHook()
{
	if (m_pPrev) m_pPrev->m_pNext = m_pNext;
	if (m_pNext) m_pNext->m_pPrev = m_pPrev;
}

class FHook : public Hook
{
public:
	FHook(LPVOID pFunction, LPVOID pHook)
		: m_pFunction(pFunction), m_pHook(pHook)
	{
	}

	virtual void DoHook()
	{
		MH_CreateHook(m_pFunction, m_pHook, &m_pTrampoline);
		MH_EnableHook(m_pFunction);

		Hook::DoHook();
	}

	virtual void DoUnHook()
	{
		MH_DisableHook(m_pFunction);
		MH_RemoveHook(m_pFunction);

		Hook::DoUnHook();
	}

	LPVOID HookFunction()
	{
		DoHook();
		return m_pTrampoline;
	}
private:
	LPVOID m_pFunction;
	LPVOID m_pHook;
	LPVOID m_pTrampoline;
};

LPVOID Hook::HookFunction(LPVOID pFunction, LPVOID pDetour, Hook** ppHook)
{
	FHook* pHook = new FHook(pFunction, pDetour);
	LPVOID pTrampoline = pHook->HookFunction();
	if (ppHook) *ppHook = pHook;
	return pTrampoline;
}

class VMTHook : public Hook
{
public:
	VMTHook(LPVOID pInterface, UINT uIndex, LPVOID pHook)
	{
		m_pVMT = *(LPVOID**)pInterface;
		m_uIndex = uIndex;
		m_pHook = pHook;
	}

	void EnableProtection(bool bEnable)
	{
		LPVOID* pPtr = &m_pVMT[m_uIndex];
		DWORD dwOldProtect;
		VirtualProtect((LPVOID)pPtr, 4096, bEnable 
			? PAGE_READONLY : PAGE_READWRITE, &dwOldProtect);
	}

	virtual void DoHook()
	{
		m_pOriginal = m_pVMT[m_uIndex];
		EnableProtection(false);
		m_pVMT[m_uIndex] = m_pHook;
		EnableProtection(true);

		Hook::DoHook();
	}

	virtual void DoUnHook()
	{
		EnableProtection(false);
		m_pVMT[m_uIndex] = m_pOriginal;
		EnableProtection(true);

		Hook::DoUnHook();
	}

	LPVOID HookVFunction()
	{
		DoHook();
		return m_pOriginal;
	}
private:
	LPVOID* m_pVMT;
	UINT m_uIndex;
	LPVOID m_pHook;
	LPVOID m_pOriginal;
};

LPVOID Hook::HookVFunction(LPVOID pInterface, UINT uIndex, 
	LPVOID pDetour, Hook** ppHook)
{
	VMTHook* pHook = new VMTHook(pInterface, uIndex, pDetour);
	LPVOID pOriginal = pHook->HookVFunction();
	if (ppHook) *ppHook = pHook;
	return pOriginal;
}
