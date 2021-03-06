/*
 * entitycache.h
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#ifndef ENTITYCACHE_H_
#define ENTITYCACHE_H_

#include "enums.h"
#include "itemtypes.h"
#include "fixsdk.h"
#include <mathlib/vector.h>

struct matrix3x4_t;

class IClientEntity;
struct player_info_s;
struct model_t;
struct mstudiohitboxset_t;
struct mstudiobbox_t;

#define MAX_STRINGS 16
#define MAX_ENTITIES 2048

#define PROXY_ENTITY true

#if PROXY_ENTITY == true
#define RAW_ENT(ce) ((ce) ? (ce)->InternalEntity() : nullptr)
#else
#define RAW_ENT(ce) ce->m_pEntity
#endif

#define CE_VAR(entity, offset, type) \
	NET_VAR(RAW_ENT(entity), offset, type)

#define CE_INT(entity, offset) CE_VAR(entity, offset, int)
#define CE_FLOAT(entity, offset) CE_VAR(entity, offset, float)
#define CE_BYTE(entity, offset) CE_VAR(entity, offset, unsigned char)
#define CE_VECTOR(entity, offset) CE_VAR(entity, offset, Vector)

#define CE_GOOD(entity) (!g_Settings.bInvalid && dynamic_cast<CachedEntity*>(entity) && RAW_ENT(entity) && !RAW_ENT(entity)->IsDormant())
#define CE_BAD(entity) (!CE_GOOD(entity))

#define IDX_GOOD(idx) (idx >= 0 && idx < HIGHEST_ENTITY && idx < MAX_ENTITIES)
#define IDX_BAD(idx) !IDX_GOOD(idx)

#define HIGHEST_ENTITY gEntityCache.m_nMax
#define ENTITY(idx) gEntityCache.GetEntity(idx)

struct CachedHitbox {
	Vector min;
	Vector max;
	Vector center;
	mstudiobbox_t* bbox;
};

#define CACHE_MAX_HITBOXES 64

class EntityHitboxCache {
public:
	EntityHitboxCache(CachedEntity* parent);
	~EntityHitboxCache();

	CachedHitbox* GetHitbox(int id);
	void Update();
	void InvalidateCache();
	bool VisibilityCheck(int id);
	void Init();
	int GetNumHitboxes();

	bool* m_VisCheckValidationFlags;
	bool* m_VisCheck;

	int m_nNumHitboxes;
	model_t* m_pLastModel;
	bool m_bModelSet;
	mstudiohitboxset_t* m_pHitboxSet;
	bool m_bInit;
	bool m_bSuccess;
	CachedEntity* m_pParentEntity;
	bool* m_CacheValidationFlags;
	CachedHitbox* m_CacheInternal;
};

class CachedEntity {
public:
	CachedEntity();
	~CachedEntity();

	void Update(int idx);

	// Entity fields start here.

	EntityType m_Type;

	int m_iClassID;
	float m_flDistance;

	bool m_bCritProjectile;
	bool m_bGrenadeProjectile;

	bool m_bAnyHitboxVisible;
	bool m_bVisCheckComplete;
	bool IsVisible();

	Vector m_vecOrigin;

	k_EItemType m_ItemType;
	int  m_iTeam;
	bool m_bAlivePlayer;
	bool m_bEnemy;
	int m_iMaxHealth;
	int m_iHealth;

	unsigned long m_lSeenTicks;
	unsigned long m_lLastSeen;

	player_info_s* m_pPlayerInfo;
	matrix3x4_t* m_Bones;
	bool m_bBonesSetup;
	matrix3x4_t* GetBones();

	// Players, Buildings, Stickies


	// Entity fields end here.

	EntityHitboxCache* m_pHitboxCache;
	int m_IDX;
	IClientEntity* InternalEntity();
	Vector m_vecVOrigin;
	Vector m_vecVelocity;
	Vector m_vecAcceleration;
	float m_fLastUpdate;
#if PROXY_ENTITY != true || 1 // FIXME??
	IClientEntity* m_pEntity;
#endif
};

class EntityCache {
public:
	EntityCache();
	~EntityCache();

	void Update();
	void Invalidate();
	CachedEntity* GetEntity(int idx);

	CachedEntity* m_pArray;
	int m_nMax;
};

extern EntityCache gEntityCache;

#endif /* ENTITYCACHE_H_ */
