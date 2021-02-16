#ifndef __HOOK_H
#define __HOOK_H

#include <Windows.h>

template<typename T>
class FunctionPtr
{
public:
	explicit FunctionPtr(void* pFunc)
		: m_pFuncPtr(pFunc)
	{
	}

	explicit FunctionPtr(DWORD_PTR dwFunc)
		: m_pFuncPtr((void*)dwFunc)
	{
	}

	explicit FunctionPtr(T Func)
		: m_Func(Func)
	{
	}

	FunctionPtr()
		: m_pFuncPtr(NULL)
	{
	}

	T& get()
	{
		return m_Func;
	}

	void* ptr()
	{
		return m_pFuncPtr;
	}

	void set(LPVOID pAddr)
	{
		m_pFuncPtr = pAddr;
	}
private:
	union {
		void* m_pFuncPtr;
		T m_Func;
	};
};

class Hook
{
public:
	static void Init();
	static void Exit();

	static void UnHookAll();

	static LPVOID HookFunction(LPVOID pFunction, LPVOID pDetour, Hook** ppHook = NULL);
	static LPVOID HookVFunction(LPVOID pInterface, UINT uIndex, LPVOID pDetour, Hook** ppHook = NULL);
	
	virtual void DoHook();
	virtual void DoUnHook();

private:
	Hook* m_pPrev;
	Hook* m_pNext;

	static Hook* s_pFirst;
	static Hook* s_pLast;
};

#endif