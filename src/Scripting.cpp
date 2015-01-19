/*  
 *  Version: MPL 1.1
 *  
 *  The contents of this file are subject to the Mozilla Public License Version 
 *  1.1 (the "License"); you may not use this file except in compliance with 
 *  the License. You may obtain a copy of the License at 
 *  http://www.mozilla.org/MPL/
 *  
 *  Software distributed under the License is distributed on an "AS IS" basis,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 *  for the specific language governing rights and limitations under the
 *  License.
 *  
 *  The Original Code is the YSI 2.0 SA:MP plugin.
 *  
 *  The Initial Developer of the Original Code is Alex "Y_Less" Cole.
 *  Portions created by the Initial Developer are Copyright (C) 2008
 *  the Initial Developer. All Rights Reserved.
 *  
 *  Contributor(s):
 *  
 *  Peter Beverloo
 *  Marcus Bauer
 *  MaVe;
 *  Sammy91
 *  Incognito
 *  
 *  Special Thanks to:
 *  
 *  SA:MP Team past, present and future
 */

#include "Scripting.h"

#include <raknet/BitStream.h>
#include "RPCs.h"
#include "amxfunctions.h"
#include "Utils.h"
#include "Structs.h"
#include "Functions.h"
#include "Hooks.h"
#include <cmath>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	// Yes - BOTH string versions...
	#include <strsafe.h>
#else
	#include <sys/stat.h>
	#include <dirent.h>
	#include <fnmatch.h>
//	#include <sys/times.h>
	#include <algorithm>
#endif

#include <sdk/plugin.h>

#include <string.h>
#include <stdio.h>

// extern
typedef cell AMX_NATIVE_CALL (* AMX_Function_t)(AMX *amx, cell *params);

//----------------------------------------------------

// native SpawnPlayerForWorld(playerid, forplayerid);
static cell AMX_NATIVE_CALL Natives::SpawnPlayerForWorld( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "SpawnPlayerForWorld");

	int playerid = (int)params[1];
	
	CSAMPFunctions::SpawnPlayer_(playerid);

	return 1;
}

// native ApplyAnimationForPlayer(playerid, animplayerid, animlib[], animname[], Float:fDelta, loop, lockx, locky, freeze, time);
static cell AMX_NATIVE_CALL Natives::ApplyAnimationForPlayer(AMX *amx, cell *params)
{
	CHECK_PARAMS(10, "ApplyAnimationForPlayer");
	RakNet::BitStream bsSend;

	char *szAnimLib;
	char *szAnimName;
	BYTE byteAnimLibLen;
	BYTE byteAnimNameLen;
	float fS;
	bool opt1,opt2,opt3,opt4;
	int time;
	
	int playerid = (int)params[1];
	int animplayerid = (int)params[2];
	if (!IsPlayerConnected(playerid) || !IsPlayerConnected(animplayerid)) return 0;

	amx_StrParam(amx, params[3], szAnimLib);
	amx_StrParam(amx, params[4], szAnimName);

	if(!szAnimLib || !szAnimName) return 0;

	byteAnimLibLen = strlen(szAnimLib);
	byteAnimNameLen = strlen(szAnimName);

	fS = amx_ctof(params[5]);
	opt1 = !!params[6];
	opt2 = !!params[7];
	opt3 = !!params[8];
	opt4 = !!params[9];
	time = (int)params[10];

	bsSend.Write((WORD)animplayerid);
	bsSend.Write(byteAnimLibLen);
	bsSend.Write(szAnimLib,byteAnimLibLen);
	bsSend.Write(byteAnimNameLen);
	bsSend.Write(szAnimName,byteAnimNameLen);
	bsSend.Write(fS);
	bsSend.Write(opt1);
	bsSend.Write(opt2);
	bsSend.Write(opt3);
	bsSend.Write(opt4);
	bsSend.Write(time);

	pRakServer->RPC(&RPC_ScrApplyAnimation, &bsSend, HIGH_PRIORITY, UNRELIABLE, 0, pRakServer->GetPlayerIDFromIndex(playerid), false, false);
	return 1;
}

// native SetLastAnimationData(playerid, data)
static cell AMX_NATIVE_CALL Natives::SetLastAnimationData( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "SetLastAnimationData");

	int playerid = (int)params[1];
	int data = (int)params[2];

	if (playerid < 0 || playerid >= 1000)
		return 0;

	lastSyncData[playerid].iAnimationId = data;

	return 1;
}

// native SendLastSyncData(playerid, toplayerid, animation = 0)
static cell AMX_NATIVE_CALL Natives::SendLastSyncData( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(3, "SendLastSyncData");

	int playerid = (int)params[1];
	int toplayerid = (int)params[2];
	int animation = (int)params[3];
	BYTE ps = ID_PLAYER_SYNC;
	CSyncData* d = &lastSyncData[playerid];

	RakNet::BitStream bs;
	bs.Write((BYTE)ID_PLAYER_SYNC);
	bs.Write((WORD)playerid);

	if (d->wUDAnalog) {
		bs.Write(true);
		bs.Write((WORD)d->wUDAnalog);
	} else {
		bs.Write(false);
	}

	if (d->wLRAnalog) {
		bs.Write(true);
		bs.Write((WORD)d->wLRAnalog);
	} else {
		bs.Write(false);
	}

	bs.Write((WORD)d->wKeys);

	bs.Write(d->vecPosition.fX);
	bs.Write(d->vecPosition.fY);
	bs.Write(d->vecPosition.fZ);

	if (fakeQuat[playerid] != NULL) {
		bs.Write((bool)(fakeQuat[playerid]->w<0.0f));
		bs.Write((bool)(fakeQuat[playerid]->x<0.0f));
		bs.Write((bool)(fakeQuat[playerid]->y<0.0f));
		bs.Write((bool)(fakeQuat[playerid]->z<0.0f));
		bs.Write((unsigned short)(fabs(fakeQuat[playerid]->x)*65535.0));
		bs.Write((unsigned short)(fabs(fakeQuat[playerid]->y)*65535.0));
		bs.Write((unsigned short)(fabs(fakeQuat[playerid]->z)*65535.0));
	} else {
		bs.Write((bool)(d->fQuaternionAngle<0.0f));
		bs.Write((bool)(d->vecQuaternion.fX<0.0f));
		bs.Write((bool)(d->vecQuaternion.fY<0.0f));
		bs.Write((bool)(d->vecQuaternion.fZ<0.0f));
		bs.Write((unsigned short)(fabs(d->vecQuaternion.fX)*65535.0));
		bs.Write((unsigned short)(fabs(d->vecQuaternion.fY)*65535.0));
		bs.Write((unsigned short)(fabs(d->vecQuaternion.fZ)*65535.0));
	}

	BYTE health, armour;

	if (fakeHealth[playerid] != 255) {
		health = fakeHealth[playerid];
	} else {
		health = d->byteHealth;
	}

	if (fakeArmour[playerid] != 255) {
		armour = fakeArmour[playerid];
	} else {
		armour = d->byteArmour;
	}

	if (health >= 100) {
		health = 0xF;
	} else {
		health /= 7;
	}

	if (armour >= 100) {
		armour = 0xF;
	} else {
		armour /= 7;
	}

	bs.Write((BYTE)((health << 4) | (armour)));

	bs.Write(d->byteWeapon);
	bs.Write(d->byteSpecialAction);

	// Make them appear standing still if paused
	if (GetTickCount() - lastUpdateTick[playerid] > 2000) {
		bs.WriteVector(0.0f, 0.0f, 0.0f);
	} else {
		bs.WriteVector(d->vecVelocity.fX, d->vecVelocity.fY, d->vecVelocity.fZ);
	}

	if (d->wSurfingInfo) {
		bs.Write(true);

		bs.Write(d->wSurfingInfo);
		bs.Write(d->vecSurfing.fX);
		bs.Write(d->vecSurfing.fY);
		bs.Write(d->vecSurfing.fZ);
	} else {
		bs.Write(false);
	}

	// Animations are only sent when they are changed
	if (animation) {
		bs.Write(true);
		bs.Write(animation);
	} else {
		bs.Write(false);
	}
	
	pRakServer->Send(&bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, pRakServer->GetPlayerIDFromIndex(toplayerid), false);

	return 1;
}

// native SetFakeArmour(playerid, health);
static cell AMX_NATIVE_CALL Natives::SetFakeArmour( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "SetFakeArmour");

	int playerid = (int)params[1];
	BYTE armour = (BYTE)params[2];

	fakeArmour[playerid] = armour;

	return 1;
}


// native SetFakeHealth(playerid, health);
static cell AMX_NATIVE_CALL Natives::SetFakeHealth( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "SetFakeHealth");

	int playerid = (int)params[1];
	BYTE health = (BYTE)params[2];

	fakeHealth[playerid] = health;

	return 1;
}

const float MATH_PI = 3.14159265359f;
const float RAD_TO_DEG = 180.0f/MATH_PI;
const float DEG_TO_RAD = 1.0f/RAD_TO_DEG;

// native SetFakeFacingAngle(playerid, Float:angle)
static cell AMX_NATIVE_CALL Natives::SetFakeFacingAngle( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "SetFakeFacingAngle");

	int playerid = (int)params[1];

	if ((int)params[2] == 0x7FFFFFFF) {
		fakeQuat[playerid] = NULL;
	} else {
		glm::vec3 vec = glm::vec3(0.0f, 0.0f, 360.0f - amx_ctof(params[2]));

		fakeQuat[playerid] = new glm::quat(vec * DEG_TO_RAD);
	}


	return 1;
}

// native SetKnifeSync(toggle);
static cell AMX_NATIVE_CALL Natives::SetKnifeSync( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(1, "SetKnifeSync");

	knifeSync = (BOOL)params[1];

	return 1;
}

// native SetDistanceBasedStreamRate(toggle);
static cell AMX_NATIVE_CALL Natives::SetDistanceBasedStreamRate( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(1, "SetDistanceBasedStreamRate");

	distanceBasedStreamRate = (BOOL)params[1];

	return 1;
}

// native SetDisableSyncBugs(toggle);
static cell AMX_NATIVE_CALL Natives::SetDisableSyncBugs( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(1, "SetDisableSyncBugs");

	disableSyncBugs = (BOOL)params[1];

	return 1;
}

// native SendDeath(playerid);
static cell AMX_NATIVE_CALL Natives::SendDeath( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(1, "SendDeath");

	int playerid = (int)params[1];

	CPlayer *pPlayer = pNetGame->pPlayerPool->pPlayer[playerid];

	pPlayer->byteState = PLAYER_STATE_WASTED;


	RakNet::BitStream bs;
	bs.Write((WORD)playerid);

	pRakServer->RPC(&RPC_DeathBroadcast, &bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, pRakServer->GetPlayerIDFromIndex(playerid), true, false);

	return 1;
}

// native FreezeSyncData(playerid, bool:toggle)
static cell AMX_NATIVE_CALL Natives::FreezeSyncData( AMX* amx, cell* params )
{
	if (!serverVersion)
		return 0;

	CHECK_PARAMS(2, "FreezeSyncData");

	int playerid = (int)params[1];
	BOOL toggle = (BOOL)params[2];

	lastSyncData[playerid].vecVelocity = CVector();
	lastSyncData[playerid].byteSpecialAction = 0;
	lastSyncData[playerid].wKeys = 0;
	lastSyncData[playerid].wUDAnalog = 0;
	lastSyncData[playerid].wLRAnalog = 0;

	syncDataFrozen[playerid] = toggle;

	return 1;
}


// And an array containing the native function-names and the functions specified with them
AMX_NATIVE_INFO YSINatives [] =
{
	{ "SpawnPlayerForWorld",		Natives::SpawnPlayerForWorld },
	{ "SetFakeHealth",				Natives::SetFakeHealth },
	{ "ApplyAnimationForPlayer",	Natives::ApplyAnimationForPlayer },
	{ "SetFakeArmour",				Natives::SetFakeArmour },
	{ "SetFakeFacingAngle",			Natives::SetFakeFacingAngle },
	{ "FreezeSyncData",				Natives::FreezeSyncData },
	{ "SetKnifeSync",				Natives::SetKnifeSync },
	{ "SendDeath",					Natives::SendDeath },
	{ "SetLastAnimationData",		Natives::SetLastAnimationData },
	{ "SendLastSyncData",			Natives::SendLastSyncData },
	{ "SetDistanceBasedStreamRate",	Natives::SetDistanceBasedStreamRate },
	{ "SetDisableSyncBugs",			Natives::SetDisableSyncBugs },
	{ 0,							0 }
};

AMX_NATIVE_INFO RedirecedtNatives[] =
{
	{ 0,								0 }
};

int InitScripting(AMX *amx)
{
	return amx_Register(amx, YSINatives, -1);
}
