#ifndef HOOKS_H
#define HOOKS_H

#include <sdk/plugin.h>
#include "CTypes.h"
#include "Structs.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Hopefully this will be optimised out
// Unfortunately I can't get ANY direct cast to work
// pointer_to_member seems like a very special structure
// Edit: Seems like it is optimised out, good, so this
// just bypasses the compilers checks to do what I want
#define POINTER_TO_MEMBER(m,n,o) temp = n, m = *(o *)&temp

// Definitions
bool Unlock(void *address, size_t len);
//void AssemblySwap(char * addr, char * dat, int len);
//void AssemblyRedirect(void * from, void * to, char * ret);

bool memory_compare(const BYTE *data, const BYTE *pattern, const char *mask);
DWORD FindPattern(char *pattern, char *mask);

void InstallPreHooks();

extern BYTE lastWeapon[1000];
extern CSyncData lastSyncData[1000];
extern BOOL syncDataFrozen[1000];
extern BYTE fakeHealth[1000];
extern BYTE fakeArmour[1000];
extern BOOL knifeSync;
extern glm::quat* fakeQuat[1000];
extern BOOL distanceBasedStreamRate;
extern DWORD lastUpdateTick[1000];
extern BOOL disableSyncBugs;


#endif
