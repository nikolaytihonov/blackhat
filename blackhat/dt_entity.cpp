#include "dt_entity.h"
#include "cdll_int.h"
#include "cdll_client_int.h"
#include "client_class.h"
#include "icliententity.h"
#include "dt_recv.h"

//DT_BaseEntity
//Vector m_vecOrigin;
//QAngle m_angRotation;
//int m_nModelIndex;
//int m_iTeamNum;

//DT_CollisionProperty m_CollisionRecvTable
//Vector m_vecMins;
//Vector m_vecMaxs;

//DT_BaseAnimating
//int m_nSequence;
//int m_nHitboxSet;

static int s_vecOrigin_offset = 0;
static int s_angRotation_offset = 0;
static int s_nModelIndex_offset = 0;
static int s_iTeamNum_offset = 0;

static int s_vecMins_offset = 0;
static int s_vecMaxs_offset = 0;

static int s_nSequence_offset = 0;
static int s_nHitboxSet_offset = 0;
static int s_iHealth_offset = 0;
static int s_iClass_offset = 0;
static int s_vecViewOffset_offset = 0;
static int s_deadflag_offset = 0;
static int s_angle_offset = 0;
static int s_hActiveWeapon_offset = 0;
static int s_fFlags_offset = 0;

RecvTable* DT_FindClassTable(const char* pClassName)
{
	ClientClass* item = clientdll->GetAllClasses();
	if (item)
	{
		do {
			if (!strcmp(item->GetName(), pClassName))
				return item->m_pRecvTable;
		} while (item = item->m_pNext);
	}
	return NULL;
}

int DT_FindPropOffset(RecvTable* table, const char* pPropName)
{
	for (int i = 0; i < table->GetNumProps(); i++)
	{
		RecvProp* prop = table->GetProp(i);
		if (!strcmp(prop->GetName(), pPropName))
		{
			return prop->GetOffset();
		}

		if (prop->GetType() == DPT_DataTable)
		{
			int offset = DT_FindPropOffset(prop->GetDataTable(), pPropName);
			if (offset != -1)
				return prop->GetOffset() + offset;
		}
	}
	return -1;
}

void DT_InitOffsets()
{
	RecvTable* DT_BaseEntity = DT_FindClassTable("CBaseEntity");
	
	s_vecOrigin_offset = DT_FindPropOffset(DT_BaseEntity, "m_vecOrigin");
	s_angRotation_offset = DT_FindPropOffset(DT_BaseEntity, "m_angRotation");
	s_nModelIndex_offset = DT_FindPropOffset(DT_BaseEntity, "m_nModelIndex");
	s_iTeamNum_offset = DT_FindPropOffset(DT_BaseEntity, "m_iTeamNum");
	s_vecMins_offset = DT_FindPropOffset(DT_BaseEntity, "m_vecMins");
	s_vecMaxs_offset = DT_FindPropOffset(DT_BaseEntity, "m_vecMaxs");

	RecvTable* DT_TFPlayer = DT_FindClassTable("CTFPlayer");

	s_nSequence_offset = DT_FindPropOffset(DT_TFPlayer, "m_nSequence");
	s_nHitboxSet_offset = DT_FindPropOffset(DT_TFPlayer, "m_nHitboxSet");
	s_iHealth_offset = DT_FindPropOffset(DT_TFPlayer, "m_iHealth");
	s_iClass_offset = DT_FindPropOffset(DT_TFPlayer, "m_iClass");
	s_vecViewOffset_offset = DT_FindPropOffset(DT_TFPlayer, "m_vecViewOffset[0]");
	s_deadflag_offset = DT_FindPropOffset(DT_TFPlayer, "deadflag");
	s_angle_offset = s_deadflag_offset + 0x04; //PlayerState::v_angle predicted
	s_hActiveWeapon_offset = DT_FindPropOffset(DT_TFPlayer, "m_hActiveWeapon");
	s_fFlags_offset = DT_FindPropOffset(DT_TFPlayer, "m_fFlags");
	
	Msg("s_vecOrigin_offset 0x%04x\n", s_vecOrigin_offset);
	Msg("s_angRotation_offset 0x%04x\n", s_angRotation_offset);
	Msg("s_nModelIndex_offset 0x%04x\n", s_nModelIndex_offset);
	Msg("s_iTeamNum_offset 0x%04x\n", s_iTeamNum_offset);
	Msg("s_vecMins_offset 0x%04x\n", s_vecMins_offset);
	Msg("s_vecMaxs_offset 0x%04x\n", s_vecMaxs_offset);
	Msg("\n");
	Msg("s_nSequence_offset 0x%04x\n", s_nSequence_offset);
	Msg("s_nHitboxSet_offset 0x%04x\n", s_nHitboxSet_offset);
	Msg("s_vecViewOffset 0x%04x\n", s_vecViewOffset_offset);
	Msg("s_deadflag_offset 0x%04x\n", s_deadflag_offset);
	Msg("s_angle_offset 0x%04x\n", s_angle_offset);
	Msg("s_hActiveWeapon_offset 0x%04x\n", s_hActiveWeapon_offset);
	Msg("s_fFlags_offset 0x%04x\n", s_fFlags_offset);
}

CON_COMMAND(blackhat_init_offsets, "")
{
	DT_InitOffsets();
}

void DT_ReadEntity(Entity* entity, IClientEntity* clEntity)
{
	if (!strcmp(clEntity->GetClientClass()->GetName(), "CTFPlayer"))
	{
		((Player*)entity)->Read(clEntity);
	}
	else entity->Read(clEntity);
}

const char* Entity::GetName()
{
	return m_pClassName;
}

void Entity::Read(IClientEntity* entity)
{
	entindex = entity->entindex();
	m_pClassName = entity->GetClientClass()->GetName();
	m_pEntity = entity;
	m_bDormant = entity->IsDormant();

	char* base = (char*)entity->GetBaseEntity();

	m_vecOrigin = *(Vector*)(base + s_vecOrigin_offset);
	m_angRotation = *(QAngle*)(base + s_angRotation_offset);
	m_nModelIndex = *(int*)(base + s_nModelIndex_offset);
	m_iTeamNum = *(int*)(base + s_iTeamNum_offset);
	m_vecMins = *(Vector*)(base + s_vecMins_offset);
	m_vecMaxs = *(Vector*)(base + s_vecMaxs_offset);
}

bool Entity::IsPlayer()
{
	return !strcmp(m_pClassName, "CTFPlayer");
}

void Player::Read(IClientEntity* entity)
{
	Entity::Read(entity);

	char* base = (char*)entity->GetBaseEntity();

	m_nSequence = *(int*)(base + s_nSequence_offset);
	m_nHitboxSet = *(int*)(base + s_nHitboxSet_offset);
	m_iHealth = *(int*)(base + s_iHealth_offset);
	m_iClass = *(int*)(base + s_iClass_offset);
	m_vecViewOffset = *(Vector*)((char*)base + s_vecViewOffset_offset);
	deadflag = *(int*)((char*)base + s_deadflag_offset);
	v_angle = *(QAngle*)((char*)base + s_angle_offset);
	m_hActiveWeapon = *(CBaseHandle*)((char*)base + s_hActiveWeapon_offset);
	m_fFlags = *(int*)((char*)base + s_fFlags_offset);
}