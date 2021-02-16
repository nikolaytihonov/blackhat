#include "sigfunc.h"
#include "sigscan.h"

FunctionPtr<void(__thiscall*)(void* thisptr, Vector& vel)> C_BaseEntity__GetVelocity;

bool SigInit()
{
	HMODULE hClientDLL = GetModuleHandle("client.dll");
	C_BaseEntity__GetVelocity.set(SigScan::FindFunction(hClientDLL, GETVELOCITY_SIG, GETVELOCITY_MASK));
	if (!C_BaseEntity__GetVelocity.ptr())
	{
		Warning("C_BaseEntity::EstimateAbsVelocity not found!\n");
		return false;
	}
	Msg("C_BaseEntity::EstimateAbsVelocity %p\n", C_BaseEntity__GetVelocity.ptr());

	return true;
}
