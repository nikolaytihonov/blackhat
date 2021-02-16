#include "hack.h"
#include "cvar.h"
#include "convar.h"
#include "vstdlib.h"
#include "tier1.h"
#include "tier2.h"
#include "mathlib.h"
#include "cdll_int.h"
#include "engine/ivmodelinfo.h"
#include "engine/IEngineTrace.h"
#include "edict.h"
#include "icliententitylist.h"
#include "icliententity.h"
#include "iclientrenderable.h"
#include "iclientnetworkable.h"
#include "client_class.h"
#include "vmatrix.h"
#include "dt_entity.h"
#include "sigfunc.h"
#include "iclientmode.h"
#include "in_buttons.h"
#include <math.h>

Hack g_Hack;

IVEngineClient013* engine = NULL;
IBaseClientDLL* clientdll = NULL;
IClientEntityList* entitylist = NULL;
IVModelInfoClient* modelinfo = NULL;
IEngineTrace* enginetrace = NULL;

CGlobalVars* gpGlobals = NULL;
IClientMode *g_pClientMode = NULL;

#define CLIENT_GLOBALVARS_OFFSET 0x5A
#define CLIENT_GETCLIENTMODENORMAL_OFFSET 0x04

#define VCLIENT_INIT 0
#define VCLIENT_LEVELINITPREENTITY 5
#define VCLIENT_LEVELINITPOSTENTITY 6
#define VCLIENT_LEVELSHUTDOWN 7
#define VCLIENT_CREATEMOVE 21
#define VCLIENT_FRAMESTAGENOTIFY 35
#define VCLIENT_CANRECORDDEMO 50

#define CLIENTMODE_CREATEMOVE 21

class CAimbotTraceFilter : public ITraceFilter
{
public:
	virtual bool ShouldHitEntity(IHandleEntity* pHandle, int contentsMask)
	{
		Entity* pEntity = g_Hack.GetEntity(pHandle->GetRefEHandle());
		if (!pEntity)
			return false;
		if (!pEntity->IsPlayer())
			return false;
		Player* pLocalPlayer = (Player*)g_Hack.GetEntity(engine->GetLocalPlayer());
		Player* pPlayer = (Player*)pEntity;

		return g_Hack.IsPlayerEnemy(pLocalPlayer, pPlayer);
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_EVERYTHING;
	}
} g_AimbotTraceFilter;

void __fastcall LevelInitPreEntity_Hook(void* thisptr, void* edx, const char* pMapName)
{
	g_Hack.LevelInitPreEntity(pMapName);
}

void __fastcall LevelInitPostEntity_Hook(void* thisptr, void* edx)
{
	g_Hack.LevelInitPostEntity();
}

void __fastcall LevelShutdown_Hook(void* thisptr, void* edx)
{
	g_Hack.LevelShutdown();
}

void __fastcall FrameStageNotify_Hook(void* thisptr, void* edx, ClientFrameStage_t stage)
{
	g_Hack.FrameStageNotify(stage);
}

void __fastcall CreateMove_Hook(void* thisptr, void* edx, float fInputTime, CUserCmd* pUserCmd)
{
	g_Hack.CreateMove(fInputTime, pUserCmd);
}

LPVOID FunctionCallPtr(LPVOID lpAddr)
{
	LONG_PTR iOffset = *(PLONG_PTR)lpAddr;
	return (LPVOID)(iOffset + (LONG_PTR)lpAddr + 0x04);
}

void Hack::Init(HMODULE hThisModule)
{
	m_WorldToScreen.Identity();

	InitGameVariables();

	m_ThisModule = hThisModule;
	Hook::Init();

	CreateInterfaceFn vstdlibFactory = VStdLib_GetICVarFactory();
	ConnectTier1Libraries(&vstdlibFactory, 1);
	MathLib_Init(2.2f, 2.2f, 0.0f, 2.0f);

	ConVar_Register();

	CreateInterfaceFn engineFactory = Sys_GetFactory("engine.dll");
	CreateInterfaceFn clientFactory = Sys_GetFactory("client.dll");
	
	engine = (IVEngineClient013*)engineFactory(VENGINE_CLIENT_INTERFACE_VERSION_13, NULL);
	modelinfo = (IVModelInfoClient*)engineFactory(VMODELINFO_CLIENT_INTERFACE_VERSION, NULL);
	enginetrace = (IEngineTrace*)engineFactory(INTERFACEVERSION_ENGINETRACE_CLIENT, NULL);

	clientdll = (IBaseClientDLL*)clientFactory(CLIENT_DLL_INTERFACE_VERSION, NULL);
	entitylist = (IClientEntityList*)clientFactory(VCLIENTENTITYLIST_INTERFACE_VERSION, NULL);

	Msg("engine %p\n", engine);
	Msg("clientdll %p\n", clientdll);

	if (!SigInit())
	{
		Warning("!SigInit()");
		return;
	}

	ULONG_PTR ClientInit = (*(PULONG_PTR*)clientdll)[VCLIENT_INIT];
	gpGlobals = (CGlobalVars*)**(LPVOID**)((char*)ClientInit + CLIENT_GLOBALVARS_OFFSET);

	ULONG_PTR CanRecordDemo = (*(PULONG_PTR*)clientdll)[VCLIENT_CANRECORDDEMO];
	Msg("IBaseClientDLL::CanRecordDemo %p\n", CanRecordDemo);
	
	FunctionPtr<IClientMode* (__cdecl*)()> GetClientModeNormal;
	GetClientModeNormal.set(FunctionCallPtr(
		(char*)CanRecordDemo + CLIENT_GETCLIENTMODENORMAL_OFFSET)
	);
	g_pClientMode = GetClientModeNormal.get()();
	Msg("g_pClientMode %p\n", g_pClientMode);

	m_CreateMove.set(Hook::HookVFunction(g_pClientMode, CLIENTMODE_CREATEMOVE, CreateMove_Hook));

	m_LevelInitPreEntity.set(Hook::HookVFunction(clientdll, VCLIENT_LEVELINITPREENTITY, LevelInitPreEntity_Hook));
	m_LevelInitPostEntity.set(Hook::HookVFunction(clientdll, VCLIENT_LEVELINITPOSTENTITY, LevelInitPostEntity_Hook));
	m_LevelShutdown.set(Hook::HookVFunction(clientdll, VCLIENT_LEVELSHUTDOWN, LevelShutdown_Hook));

	m_FrameStageNotify.set(Hook::HookVFunction(clientdll, VCLIENT_FRAMESTAGENOTIFY, FrameStageNotify_Hook));

	DT_InitOffsets();

	engine->GetScreenSize(m_iWidth, m_iHeight);

	m_Draw.Init();
	m_Draw.SetRenderCallback(this);
}

void Hack::Exit()
{
	ConVar_Unregister();

	m_Draw.Exit();

	Hook::Exit();
}

void Hack::Render(IDirect3DDevice9* pDevice)
{
	m_Draw.Print("blackhat");
	m_Draw.Print("m_iEndSceneCounter %d", m_iEndSceneCounter);
	m_Draw.Print("realtime %f", gpGlobals->realtime);
	m_Draw.Print("frametime %f", gpGlobals->frametime);
	m_Draw.Print("curtime %f", gpGlobals->curtime);
	m_Draw.Print("interval_per_tick %f", gpGlobals->interval_per_tick);

	m_Draw.Print("");

	m_Draw.Print("Hack::m_bInGame %d", m_bInGame);
	if (m_bInGame)
	{
		m_Draw.Print("Map name %s", m_MapName.c_str());
	}

	RenderGame();
}

void Hack::InitGameVariables()
{
	m_bInGameRender = false;
	m_iEndSceneCounter = 0;

	m_MapName = "";
	m_bInGame = false;

	m_pFirstEntity = NULL;
	m_pLastEntity = NULL;

	m_fLastFrameUpdate = 0;
	m_fFrameDelta = 0;

	m_pLocalPlayer = NULL;
}

void Hack::LevelInitPreEntity(const char* pMapName)
{
	m_LevelInitPreEntity.get()(clientdll, pMapName);

	InitGameVariables();
	m_MapName = std::string(pMapName);
}

void Hack::LevelInitPostEntity()
{
	m_LevelInitPostEntity.get()(clientdll);

	m_pLocalPlayer = (Player*)GetEntity(engine->GetLocalPlayer());
	m_bInGame = true;
}

void Hack::LevelShutdown()
{
	m_LevelShutdown.get()(clientdll);
	
	m_bInGame = false;

	RemoveAllEntities();
}

void Hack::FrameStageNotify(ClientFrameStage_t stage)
{
	m_FrameStageNotify.get()(clientdll, stage);

	//FRAME_RENDER_END
	
	switch (stage)
	{
	case FRAME_NET_UPDATE_END:
		//UpdateWorld();
		break;
	case FRAME_RENDER_START:
		//Interpolation and prediction comes after FRAME_RENDER_START
		m_iEndSceneCounter = 0;
		m_bInGameRender = true;
		
		SetupRender();

		WorldUpdate();
		RenderUpdate();
		break;
	case FRAME_RENDER_END:
		if (m_bInGame)
			FrameThink();
		m_bInGameRender = false;
		break;
	}
}

Entity* Hack::AddEntity(IClientEntity* entity)
{
	Entity* hackEntity;
	if (!strcmp(entity->GetClientClass()->GetName(), "CTFPlayer"))
		hackEntity = new Player();
	else hackEntity = new Entity();
	DT_ReadEntity(hackEntity, entity);

	if (!m_pFirstEntity) m_pFirstEntity = hackEntity;
	if (m_pLastEntity) m_pLastEntity->m_pNext = hackEntity;

	hackEntity->m_pPrev = m_pLastEntity;
	m_pLastEntity = hackEntity;

	return hackEntity;
}

Entity* Hack::GetEntity(int entindex)
{
	Entity* item = m_pFirstEntity;
	if (item)
	{
		do {
			if (item->entindex == entindex)
				return item;
		} while (item = item->m_pNext);
	}
	return NULL;
}

Entity* Hack::GetEntity(const CBaseHandle& refHandle)
{
	Entity* item = m_pFirstEntity;
	if (item)
	{
		do {
			if (!item->m_pEntity)
				continue;
			if (item->entindex == refHandle.GetEntryIndex())
				return item;
		} while (item = item->m_pNext);
	}
	return NULL;
}

void Hack::RemoveEntity(Entity* hackEntity)
{
	if (hackEntity == m_pFirstEntity)
		m_pFirstEntity = hackEntity->m_pNext;
	if (hackEntity == m_pLastEntity)
		m_pLastEntity = hackEntity->m_pPrev;
	if (hackEntity->m_pPrev)
		hackEntity->m_pPrev->m_pNext = hackEntity->m_pNext;
	if (hackEntity->m_pNext)
		hackEntity->m_pNext->m_pPrev = hackEntity->m_pPrev;
	delete hackEntity;
}

void Hack::RemoveAllEntities()
{
	Entity* item = m_pFirstEntity;
	Entity* next;
	if (!item)
		return;
	m_pFirstEntity = NULL;
	m_pLastEntity = NULL;
	do {
		next = item->m_pNext;

		delete item;
	} while (item = next);
}

void Hack::SetupRender()
{
	memcpy(&m_WorldToScreen, &engine->WorldToScreenMatrix(), sizeof(VMatrix));
}

void Hack::WorldUpdate()
{
	Entity* entity;

	if (!m_bInGame)
		return;

	if (m_fLastFrameUpdate != 0)
	{
		m_fFrameDelta = gpGlobals->realtime - m_fLastFrameUpdate;
	}
	m_fLastFrameUpdate = gpGlobals->realtime;

	for (int i = 0; i < entitylist->GetHighestEntityIndex(); i++)
	{
		IClientEntity* clientEntity = entitylist->GetClientEntity(i);
		if (!clientEntity)
			continue;
		if (clientEntity->IsDormant())
			continue;
		
		entity = GetEntity(clientEntity->entindex());
		if (!entity)
			entity = AddEntity(clientEntity);

		EntityUpdate(entity, clientEntity);
	}

	entity = m_pFirstEntity;
	while (entity)
	{
		Entity* next = entity->m_pNext;
		if (!entitylist->GetClientEntity(entity->entindex))
			RemoveEntity(entity);
		entity = next;
	}

	m_pLocalPlayer = (Player*)GetEntity(engine->GetLocalPlayer());
}

void Hack::EntityUpdate(Entity* entity, IClientEntity* clientEntity)
{
	DT_ReadEntity(entity, clientEntity);

	C_BaseEntity* baseEntity = clientEntity->GetBaseEntity();
	if (baseEntity)
	{
		C_BaseEntity__GetVelocity.get()(baseEntity, entity->m_vecVelocity);
	}
}

void Hack::PlayerUpdate(Player* player)
{
	player->m_pModel = modelinfo->GetStudiomodel(player->m_pEntity->GetModel());
	if (player->m_pModel)
	{
		player->m_pEntity->SetupBones(player->m_Bones, MAXSTUDIOBONES,
			BONE_USED_BY_HITBOX, engine->GetLastTimeStamp());
		mstudiohitboxset_t* pSet = player->m_pModel
			->pHitboxSet(player->m_nHitboxSet);
		for (int i = 0; i < pSet->numhitboxes; i++)
		{
			mstudiobbox_t* pHitbox = pSet->pHitbox(i);

			const char* pBoneName = player->m_pModel->pBone(pHitbox->bone)->pszName();
			if (strstr(pBoneName, "head"))
				player->m_iHead = i;

			int iBigHitbox = -1;
			float fBitHitboxSize = 0;

			Vector vSize = pHitbox->bbmin - pHitbox->bbmax;
			float fHitboxSize = vSize.x * vSize.y * vSize.z;
			if (fHitboxSize >= fBitHitboxSize)
			{
				iBigHitbox = i;
				fBitHitboxSize = fHitboxSize;
			}
		}
	}
}

void Hack::RenderUpdate()
{
	Entity* entity = m_pFirstEntity;
	while (entity)
	{
		IClientRenderable* render = entity->m_pEntity->GetClientRenderable();
		if (render)
		{
			entity->m_vecRenderOrigin = render->GetRenderOrigin();
			entity->m_angRenderAngles = render->GetRenderAngles();
		}

		if (entity->IsPlayer())
		{
			PlayerUpdate((Player*)entity);
		}

		entity = entity->m_pNext;
	}
}

void Hack::GetHitboxPosition(Player* player, int iHitbox, Vector& vOut)
{
	mstudiohitboxset_t* pSet = player->m_pModel
		->pHitboxSet(player->m_nHitboxSet);
	mstudiobbox_t* pHitbox = pSet->pHitbox(iHitbox);
	int iBone = pHitbox->bone;
	Vector vMin, vMax;
	VectorTransform(pHitbox->bbmin, player->m_Bones[iBone], vMin);
	VectorTransform(pHitbox->bbmax, player->m_Bones[iBone], vMax);
	vOut = (vMax + vMin) / 2;
}

bool Hack::IsInCameraView(const Vector& world)
{
	const VMatrix& W2S = m_WorldToScreen;
	
	float w = W2S[3][0] * world[0] + W2S[3][1] * world[1] + W2S[3][2] * world[2] + W2S[3][3];
	return w > 0.01;
}

bool Hack::WorldToScreen(const Vector& world, Vector& screen)
{
	const VMatrix& W2S = m_WorldToScreen;

	float w = W2S[3][0] * world[0] + W2S[3][1] * world[1] + W2S[3][2] * world[2] + W2S[3][3];
	if (w > 0.01)
	{
		float fl1DBw = 1 / w;
		screen.x = (m_iWidth / 2) + (0.5 * ((W2S[0][0] * world[0] + W2S[0][1] * world[1] 
			+ W2S[0][2] * world[2] + W2S[0][3]) * fl1DBw) * m_iWidth + 0.5);
		screen.y = (m_iHeight / 2) - (0.5 * ((W2S[1][0] * world[0] + W2S[1][1] * world[1] 
			+ W2S[1][2] * world[2] + W2S[1][3]) * fl1DBw) * m_iHeight + 0.5);

		screen.z = 0;
		return true;
	}

	screen = Vector(0);
	return false;
}

void Hack::DrawESPBox(const Vector& vOrigin, const QAngle& qAngle,
	const Vector& vMin, const Vector& vMax,
	D3DCOLOR color)
{
	const unsigned int VERTEX_NUM = 5;
	Vector vertex[VERTEX_NUM];
	D3DXVECTOR2 points[VERTEX_NUM];

	if (!IsInCameraView(vOrigin + vMin) && !IsInCameraView(vOrigin + vMax))
		return;

	matrix3x4_t posMatrix;
	AngleMatrix(qAngle, vOrigin, posMatrix);

	float x = vMax.x - vMin.x;
	float y = vMax.y - vMin.y;
	float z = vMax.z - vMin.z;

	//1. Connected bottom vertices
	//2. Build up lines
	//3. Connected top vertices

	vertex[0] = vMin;
	vertex[1] = vertex[0] + Vector(x, 0, 0);
	vertex[2] = vertex[1] + Vector(0, y, 0);
	vertex[3] = vertex[0] + Vector(0, y, 0);
	vertex[4] = vertex[0];
	
	//Bottom
	for (int i = 0; i < VERTEX_NUM; i++)
	{
		Vector vOut;
		VectorTransform(vertex[i], posMatrix, vOut);

		Vector vScreen;
		WorldToScreen(vOut, vScreen);
		points[i] = D3DXVECTOR2(vScreen.x, vScreen.y);
	}

	m_Draw.DrawLines(points, VERTEX_NUM, color);

	//Lines
	for (int i = 0; i < 4; i++)
	{
		Vector vStart1 = vertex[i];
		Vector vEnd1 = vStart1 + Vector(0, 0, z);
		
		Vector vStart,vEnd;
		VectorTransform(vStart1, posMatrix, vStart);
		VectorTransform(vEnd1, posMatrix, vEnd);

		Vector vScrStart, vScrEnd;
		WorldToScreen(vStart, vScrStart);
		WorldToScreen(vEnd, vScrEnd);
		m_Draw.DrawLine(vScrStart.x, vScrStart.y, vScrEnd.x, vScrEnd.y, color);
	}

	//Top
	for (int i = 0; i < VERTEX_NUM; i++)
	{
		Vector vOut;
		vertex[i] += Vector(0, 0, z);
		VectorTransform(vertex[i], posMatrix, vOut);

		Vector vScreen;
		WorldToScreen(vOut, vScreen);
		points[i] = D3DXVECTOR2(vScreen.x, vScreen.y);
	}

	m_Draw.DrawLines(points, VERTEX_NUM, color);
}

bool Hack::IsPlayerEnemy(Player* localPlayer, Player* player)
{
	if (localPlayer == player)
		return false;
	if (localPlayer->m_iTeamNum == player->m_iTeamNum || player->m_iTeamNum == 0)
		return false;
	if (player->m_iHealth <= 0 || player->deadflag)
		return false;
	return true;
}

void Hack::FrameThink()
{
	IClientEntity* localPlayer = entitylist->GetClientEntity(engine->GetLocalPlayer());
	if (!localPlayer)
		return;
	ClientClass* clientClass = localPlayer->GetClientClass();
}

static ConVar blackhat_esp_box("blackhat_esp_box", "1");
static ConVar blackhat_esp_dormant("blackhat_esp_dormant", "0");
static ConVar blackhat_esp_velocity("blackhat_esp_velocity", "0");
static ConVar blackhat_esp_hitbox("blackhat_esp_hitbox", "0");
static ConVar blackhat_esp_hitbox_name("blackhat_esp_hitbox_name", "0");

void Hack::RenderGame()
{
	Entity* entity = m_pFirstEntity;
	if (!entity)
		return;
	Player* localPlayer = (Player*)GetEntity(engine->GetLocalPlayer());
	if (!localPlayer)
		return;

	bool bDormantCheck = blackhat_esp_dormant.GetBool();
	do
	{
		if (entity == localPlayer)
			continue;
		if (entity->IsPlayer())
		{
			Player* player = (Player*)entity;
			if (!bDormantCheck)
			{
				if (player->m_bDormant)
					continue;
			}
			if (!IsPlayerEnemy(localPlayer, player))
				continue;
			Vector vScreen;
			Vector& vOrigin = player->m_vecRenderOrigin;
			if (WorldToScreen(vOrigin, vScreen))
			{
				player_info_t info;
				engine->GetPlayerInfo(player->entindex, &info);

				wchar_t wszNickName[64];
				MultiByteToWideChar(CP_UTF8, 0, info.name, -1, wszNickName, 64);

				if (blackhat_esp_box.GetBool())
				{
					DrawESPBox(vOrigin, player->m_angRotation,
						player->m_vecMins, player->m_vecMaxs,
						D3DCOLOR_RGBA(0, 255, 0, 255));
				}
				m_Draw.DrawTextW(vScreen.x, vScreen.y, wszNickName, D3DCOLOR_RGBA(0, 255, 0, 255));

				if (blackhat_esp_velocity.GetBool())
				{
					Vector vStart1 = vOrigin;
					Vector vEnd1 = vStart1 + player->m_vecVelocity;

					Vector vStart, vEnd;
					WorldToScreen(vStart1, vStart);
					WorldToScreen(vEnd1, vEnd);

					m_Draw.DrawLine(vStart.x, vStart.y, vEnd.x, vEnd.y, D3DCOLOR_RGBA(0, 255, 255, 255));
				}

				if (blackhat_esp_hitbox.GetBool() && player->m_pModel)
				{
					mstudiohitboxset_t* pSet = player->m_pModel
						->pHitboxSet(player->m_nHitboxSet);
					for (int i = 0; i < pSet->numhitboxes; i++)
					{
						mstudiobbox_t* pHitbox = pSet->pHitbox(i);
						
						Vector vOrigin;
						QAngle qAngle;

						MatrixAngles(player->m_Bones[pHitbox->bone], qAngle, vOrigin);

						Vector vOrigin2D;
						if (WorldToScreen(vOrigin, vOrigin2D))
						{
							DrawESPBox(vOrigin, qAngle, pHitbox->bbmin, pHitbox->bbmax, D3DCOLOR_RGBA(0, 255, 0, 255));
							if (blackhat_esp_hitbox_name.GetBool())
							{
								const char* pBoneName = player->m_pModel->pBone(pHitbox->bone)->pszName();
								m_Draw.DrawTextA(vOrigin2D.x, vOrigin2D.y, 
									pBoneName, D3DCOLOR_RGBA(0, 255, 0, 255));
							}
						}
					}					
				}
			}
		}
	} while (entity = entity->m_pNext);

	m_Draw.Print("m_fLastFrameUpdate %f", m_fLastFrameUpdate);
	m_Draw.Print("m_fFrameDelta %f", m_fFrameDelta);
	
	if (localPlayer)
	{
		m_Draw.Print("m_iClass %d", localPlayer->m_iClass);
		m_Draw.Print("m_iTeamNum %d", localPlayer->m_iTeamNum);
		m_Draw.Print("m_iHealth %d", localPlayer->m_iHealth);
		Entity* pWeapon = GetEntity(localPlayer->m_hActiveWeapon);
		if (pWeapon)
		{
			m_Draw.Print("m_hActiveWeapon %s", pWeapon->m_pClassName);
		}

		m_Draw.Print("VELOCITY %f %f %f",
			localPlayer->m_vecVelocity.x,
			localPlayer->m_vecVelocity.y,
			localPlayer->m_vecVelocity.z
		);
	}
}

void Hack::GetEyePosition(Vector& vOut)
{
	vOut = m_pLocalPlayer->m_vecOrigin + m_pLocalPlayer->m_vecViewOffset;
}

void Hack::AimTraceLine(trace_t& tr, Vector vEnd)
{
	Ray_t ray;
	
	Vector vOrigin = m_pLocalPlayer->m_vecOrigin;
	Vector vStart = vOrigin + m_pLocalPlayer->m_vecViewOffset;
	
	ray.Init(vStart, vEnd);
	enginetrace->TraceRay(ray, MASK_SHOT, &g_AimbotTraceFilter, &tr);
}

void Hack::AimTraceLine(trace_t& tr)
{
	Vector vOrigin = m_pLocalPlayer->m_vecOrigin;
	Vector vStart;
	Vector vDir;

	GetEyePosition(vStart);
	AngleVectors(m_pLocalPlayer->v_angle, &vDir);
	Vector vEnd = vStart + vDir * 2048.0f;

	CAimbotTraceFilter filter;

	AimTraceLine(tr, vEnd);
}

void Hack::CreateMove(float fInputTime, CUserCmd* pUserCmd)
{
	m_CreateMove.get()(g_pClientMode, fInputTime, pUserCmd);

	m_fInputSampleTime = m_fInputSampleTime;
	m_pUserCmd = pUserCmd;
	if (m_pLocalPlayer)
		m_pLocalPlayer->v_angle = pUserCmd->viewangles;

	Aimbot();
}

Entity* Hack::GetActiveWeapon()
{
	if (!m_pLocalPlayer->m_hActiveWeapon.IsValid())
		return NULL;
	return GetEntity(m_pLocalPlayer->m_hActiveWeapon);
}

#define RADPI 57.295779513082f

void Hack::AimTo(const Vector& vSrc, const Vector& vDst, QAngle& qAngle)
{
	Vector vDelta = vSrc - vDst;
	float fHyp = vDelta.Length2D();
	qAngle[0] = atanf(vDelta[2] / fHyp) * RADPI;
	qAngle[1] = atanf(vDelta[1] / vDelta[0]) * RADPI;
	qAngle[2] = 0;
	if (vDelta[0] >= 0.0)
		qAngle[1] += 180.0;
	//Normalize angles
	for (int i = 0; i < 2; i++)
	{
		while (qAngle[i] > 180.0)
			qAngle[i] -= 360.0;
		while (qAngle[i] < -180.0)
			qAngle[i] += 360.0;
	}
}

Player* Hack::AimFindScreenTarget(Vector& vShoot, bool bAimHeadFirst)
{
	Player* pTarget = NULL;
	Vector vTargetHitbox;
	float fNear = FLT_MAX;

	Entity* pEntity = m_pFirstEntity;
	if (!pEntity)
		return NULL;
	do {
		if (!pEntity->IsPlayer())
			continue;
		Player* pPlayer = (Player*)pEntity;
		if (pPlayer->m_bDormant)
			continue;
		if (!IsPlayerEnemy(m_pLocalPlayer, pPlayer))
			continue;
		
		Vector vOrigin = pPlayer->m_vecOrigin;
		Vector vScreen;
		if (!WorldToScreen(vOrigin, vScreen))
			continue;

		float fScrDistance = (vScreen - Vector(m_iWidth / 2, m_iHeight / 2, 0)).Length2D();
		if (fScrDistance < fNear)
		{
			//Trace hitboxes
			trace_t tr;
			Vector vHitbox;

			tr.m_pEnt = NULL;

			if (bAimHeadFirst)
			{
				GetHitboxPosition(pPlayer, pPlayer->m_iHead, vHitbox);
				AimTraceLine(tr, vHitbox);
			}

			//Aim body if we can't trace head
			if (tr.m_pEnt != pPlayer->m_pEntity->GetBaseEntity())
			{
				GetHitboxPosition(pPlayer, pPlayer->m_iBigHitbox, vHitbox);
				AimTraceLine(tr, vHitbox);
			}

			//Check hitboxes
			if (tr.m_pEnt != pPlayer->m_pEntity->GetBaseEntity())
			{
				//We can't hit player
				continue;
			}

			//We found best target
			pTarget = pPlayer;
			vTargetHitbox = vHitbox;
			fNear = fScrDistance;
		}
	} while (pEntity = pEntity->m_pNext);

	vShoot = vTargetHitbox;
	return pTarget;
}

static ConVar blackhat_aimbot("blackhat_aimbot", "1");

void Hack::Aimbot()
{
	if (!blackhat_aimbot.GetBool())
		return;
	if (!(m_pUserCmd->buttons & IN_ATTACK))
		return;
	//Setup LocalPlayer viewangles
	m_pLocalPlayer->v_angle = m_pUserCmd->viewangles;

	//Select aimbot type
	switch (m_pLocalPlayer->m_iClass)
	{
	case TF_SOLDIER:
	case TF_DEMOMAN:
		ProjectileAimbot();
		break;
	default:
		BulletAimbot();
	}
}

void Hack::BulletAimbot()
{
	Player* pTarget;
	Vector vShoot;

	//Determine aimbot mode
	bool bAimHeadFirst = false;
	if (m_pLocalPlayer->m_iClass == TF_SNIPER
		|| m_pLocalPlayer->m_iClass == TF_SPY)
	{
		bAimHeadFirst = true;
	}

	//Find target
	pTarget = AimFindScreenTarget(vShoot, bAimHeadFirst);
	if (!pTarget)
		return;

	Vector vEye;
	GetEyePosition(vEye);
	AimTo(vEye, vShoot, m_pUserCmd->viewangles);
}

float Hack::GetWeaponProjectileSpeed()
{
	Entity* pWeapon = GetActiveWeapon();
	if (!pWeapon)
		return 0.0;
	const char* pName = pWeapon->m_pClassName;
	if (!strcmp(pName, "CTFRocketLauncher"))
		return 1100.0;
	else if (!strcmp(pName, "CTFGrenadeLauncher"))
		return 910.0;
	return 0.0;
}

static ConVarRef sv_gravity("sv_gravity");

void Hack::PredictPlayerPos(Player* player, float fDelta, Vector& vOut)
{
	vOut = player->m_vecOrigin
		+ player->m_vecVelocity * fDelta;
	if (!(player->m_fFlags & FL_ONGROUND))
	{
		float g = sv_gravity.GetFloat();
		vOut.z += (-g * fDelta * fDelta) / 2;
	}
}

void Hack::ProjectileAimbot()
{
	//aPos = aPos0 + aVel*t + (aAcc * t * t) / 2
	//bPos = bPos0 + bVel*t

	//bPos = aPos0 + aVel*t + (aAcc * t * t) / 2 - bPos0 - bVel*t
	//t = 1

	Player* pTarget;
	Vector vShoot;
	Vector vEye;
	Vector vForward;
	float fProjSpeed = GetWeaponProjectileSpeed();

	//Find target
	pTarget = AimFindScreenTarget(vShoot, false);
	if (!pTarget)
		return;

	//aPos = aPos0 + aVel*t
	//bPos = bPos0 + bVel*t

	//aPos0 + aVel*t = bPos0 + bVel*t
	//aVel*t - bVel*t = bPos0 -aPos0
	//t * (aVel - bVel) = bPos0 -aPos0
	//t = (bPos0 - aPos0) / (aVel - bVel)

	//aPos = aPos0 + aVel*t + (aAcc * t * t) / 2
	//bPos = bPos0 + bVel*t

	//aPos0 + aVel*t + (aAcc * t * t) / 2 = bPos0 + bVel*t
	//aVel*t - bVel*t + (aAcc * t * t) / 2 = bPos0 - aPos0
	//t * (aVel - bVel) + aAcc * t * t / 2 = bPos0 - aPos0

	GetEyePosition(vEye);
	AngleVectors(m_pLocalPlayer->v_angle, &vForward);

	Vector aPos0 = pTarget->m_vecOrigin + (pTarget->m_vecMins + pTarget->m_vecMaxs) / 2;
	Vector aVel = pTarget->m_vecVelocity;

	Vector bPos0 = vEye;
	Vector bVel = vForward * fProjSpeed;

	float t0 = (bPos0[0] - aPos0[0]) / (aVel[0] - bVel[0]);
	float t1 = (bPos0[1] - aPos0[1]) / (aVel[1] - bVel[1]);
	float t2 = (bPos0[2] - aPos0[2]) / (aVel[2] - bVel[2]);

	Msg("%f %f %f\n", t0, t1, t2);
	
	float aAcc = -sv_gravity.GetFloat();

	Vector vOut = Vector(
		aPos0[0] + aVel[0] * t0,
		aPos0[1] + aVel[1] * t1,
		aPos0[2] + aVel[2] * t2
		+ !(pTarget->m_fFlags & FL_ONGROUND)
		? (aAcc * t2 * t2) / 2
		: 0
		);
	AimTo(vEye, vOut, m_pUserCmd->viewangles);
}