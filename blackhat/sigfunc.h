#ifndef __SIGFUNC_H
#define __SIGFUNC_H

#include "hook.h"
#include "vector.h"

#define GETVELOCITY_SIG "\x55\x8B\xEC\x83\xEC\x0C\x56\x8B\xF1\xE8\x00\x00\x00\x00\x3B\xF0\x75\x2B\x8B\xCE\xE8\x00\x00\x00\x00\x8B\x45\x08\xD9\x86\x00\x00\x00\x00\xD9\x18"
#define GETVELOCITY_MASK "xxxxxxxxxx????xxxxxxx????xxxxx????xx"

extern FunctionPtr<void(__thiscall*)(void* thisptr, Vector& vel)> C_BaseEntity__GetVelocity;

bool SigInit();

#endif