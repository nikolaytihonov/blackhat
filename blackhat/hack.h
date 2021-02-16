#ifndef __HACK_H
#define __HACK_H

#include <Windows.h>
#include <string>
#include "hook.h"
#include "draw.h"
#include "cdll_int.h"
#include "mathlib.h"
#include "edict.h"
#include "dt_entity.h"
#include "usercmd.h"
#include <map>

class Hack : public IRenderCallback
{
public:
	void Init(HMODULE hThisModule);
	void Exit();

	virtual void Render(IDirect3DDevice9* pDevice);

	void InitGameVariables();

	void LevelInitPreEntity(const char* pMapName);
	void LevelInitPostEntity();
	void LevelShutdown();

	void FrameStageNotify(ClientFrameStage_t stage);
	
	Entity* AddEntity(IClientEntity* entity);
	Entity* GetEntity(int entindex);
	Entity* GetEntity(const CBaseHandle& refHandle);
	void RemoveEntity(Entity* hackEntity);
	void RemoveAllEntities();
	
	void SetupRender();
	void WorldUpdate();
	void EntityUpdate(Entity* entity, IClientEntity* clientEntity);
	void PlayerUpdate(Player* player);
	void RenderUpdate();

	void GetHitboxPosition(Player* player, int iHitbox, Vector& vOut);

	bool IsInCameraView(const Vector& world);
	bool WorldToScreen(const Vector& world, Vector& screen);

	void DrawESPBox(const Vector& vOrigin, const QAngle& qAngle,
		const Vector& vMin, const Vector& vMax,
		D3DCOLOR color);

	bool IsPlayerEnemy(Player* localPlayer, Player* player);

	void FrameThink();
	void RenderGame();

	void GetEyePosition(Vector& vOut);
	void AimTraceLine(trace_t& tr, Vector vEnd);
	void AimTraceLine(trace_t& tr);
	void CreateMove(float fInputTime, CUserCmd* pUserCmd);

	Entity* GetActiveWeapon();

	void AimTo(const Vector& vSrc, const Vector& vDst, QAngle& qAngle);
	Player* AimFindScreenTarget(Vector& vShoot, bool bAimHeadFirst = false);
	void Aimbot();
	void BulletAimbot();
	
	float GetWeaponProjectileSpeed();
	void PredictPlayerPos(Player* player, float fDelta, Vector& vOut);
	void ProjectileAimbot();
private:
	HMODULE m_ThisModule;
	Draw m_Draw;

	VMatrix m_WorldToScreen;

	bool m_bInGameRender;
	int m_iEndSceneCounter;

	int m_iWidth;
	int m_iHeight;

	FunctionPtr<void(__thiscall*)(void* thisptr, const char*)> m_LevelInitPreEntity;
	FunctionPtr<void(__thiscall*)(void* thisptr)> m_LevelInitPostEntity;
	FunctionPtr<void(__thiscall*)(void* thisptr)> m_LevelShutdown;

	std::string m_MapName;
	bool m_bInGame;

	FunctionPtr<void(__thiscall*)(void* thisptr, ClientFrameStage_t)> m_FrameStageNotify;
	
	Entity* m_pFirstEntity;
	Entity* m_pLastEntity;

	float m_fLastFrameUpdate;
	float m_fFrameDelta;
	
	Player* m_pLocalPlayer;

	FunctionPtr<void(__thiscall*)(void* thisptr, float, CUserCmd*)> m_CreateMove;

	float m_fInputSampleTime;
	CUserCmd* m_pUserCmd;
};

extern Hack g_Hack;

#endif