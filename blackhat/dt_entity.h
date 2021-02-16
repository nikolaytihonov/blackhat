#ifndef __DT_ENTITY_H
#define __DT_ENTITY_H

#include "cdll_int.h"
#include "mathlib.h"
#include "edict.h"
#include <string>
#include "icliententity.h"
#include "ehandle.h"

class Entity
{
public:
	Entity()
		: m_pPrev(NULL),m_pNext(NULL)
	{}

	void Read(IClientEntity* entity);
	virtual const char* GetName();
	virtual bool IsPlayer();

	Entity* m_pPrev;
	Entity* m_pNext;

	int entindex;
	const char* m_pClassName;
	IClientEntity* m_pEntity;
	bool m_bDormant;
	
	Vector m_vecVelocity;

	Vector m_vecRenderOrigin;
	QAngle m_angRenderAngles;

	//DT_BaseEntity
	Vector m_vecOrigin;
	QAngle m_angRotation;
	int m_nModelIndex;
	int m_iTeamNum;

	//DT_CollisionProperty m_CollisionRecvTable
	Vector m_vecMins;
	Vector m_vecMaxs;
};

#define TF_SCOUT 1
#define TF_SOLDIER 3
#define TF_PYRO 7
#define TF_DEMOMAN 4
#define TF_HEAVY 6
#define TF_ENGINEER 9
#define TF_MEDIC 5
#define TF_SNIPER 2
#define TF_SPY 8

class Player : public Entity
{
public:
	Player() : Entity(),
		m_pModel(NULL)
	{}

	void Read(IClientEntity* entity);
	virtual bool IsPlayer() { return true; }

	//DT_BaseAnimating
	int m_nSequence;
	int m_nHitboxSet;

	//DT_BasePlayer
	int m_iHealth;

	int m_iClass;

	Vector m_vecViewOffset;

	int deadflag;
	QAngle v_angle; //View angle
	CBaseHandle m_hActiveWeapon;
	int m_fFlags;

	//Studiomodel
	studiohdr_t* m_pModel;
	matrix3x4_t m_Bones[MAXSTUDIOBONES];

	int m_iHead;
	int m_iBigHitbox;
};

void DT_InitOffsets();
void DT_ReadEntity(Entity* entity, IClientEntity* clEntity);

#endif