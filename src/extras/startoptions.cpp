#include "common.h"

#include <ctype.h>

#include "startoptions.h"
#include "Automobile.h"
#include "Boat.h"
#include "Camera.h"
#include "ModelInfo.h"
#include "Pad.h"
#include "PathFind.h"
#include "PlayerPed.h"
#include "Streaming.h"
#include "World.h"

#define CHEATS_SIZE 64
#define CAR_NAME_SIZE 32
#define NEAREST_ROAD_SEARCH_RANGE 100.0f
#define SPAWN_HEIGHT 4.0f
#define SPAWN_HEADING 3.49f
#define BOAT_SPAWN_DISTANCE 15.0f

static char cheats[CHEATS_SIZE];
static char carName[CAR_NAME_SIZE];

void
StartOptions::AddCheat(const char *cheat)
{
	size_t length = strlen(cheats);
	for ( ; *cheat && length < CHEATS_SIZE - 1; cheat++ )
		cheats[length++] = toupper((unsigned char)*cheat);
	cheats[length] = '\0';
}

void
StartOptions::SetCar(const char *modelName)
{
	strncpy(carName, modelName, CAR_NAME_SIZE - 1);
	carName[CAR_NAME_SIZE - 1] = '\0';
}

static bool
IsPlayerInControl(void)
{
	// Loading and fading in still change the player, so wait until both are done
	return FindPlayerPed() != nil && !CPad::GetPad(0)->ArePlayerControlsDisabled()
		&& TheCamera.GetScreenFadeStatus() == FADE_0;
}

static void
TypeCheats(void)
{
	debug("typing the start cheats %s\n", cheats);
	for ( char *c = cheats; *c; c++ )
		CPad::GetPad(0)->AddToPCCheatString(*c);
	cheats[0] = '\0';
}

static void
SpawnCarForPlayer(void)
{
	int32 modelId;
	CBaseModelInfo *modelInfo = CModelInfo::GetModelInfo(carName, &modelId);
	CVehicle *vehicle = nil;
	if ( modelInfo != nil && modelInfo->GetModelType() == MITYPE_VEHICLE )
		vehicle = SpawnCarOnNearestRoad(modelId);

	if ( vehicle == nil )
		debug("could not spawn the start car %s\n", carName);
	else {
		debug("spawned the start car %s\n", carName);
		CPlayerPed *player = FindPlayerPed();
		player->SetObjective(OBJECTIVE_ENTER_CAR_AS_DRIVER, vehicle);
		player->WarpPedIntoCar(vehicle);
	}
	carName[0] = '\0';
}

void
StartOptions::Apply(void)
{
	if ( (cheats[0] == '\0' && carName[0] == '\0') || !IsPlayerInControl() )
		return;

	if ( cheats[0] != '\0' )
		TypeCheats();
	if ( carName[0] != '\0' )
		SpawnCarForPlayer();
}

CVehicle *
SpawnCarOnNearestRoad(int32 modelId)
{
	CStreaming::RequestModel(modelId, 0);
	CStreaming::LoadAllRequestedModels(false);
	if ( !CStreaming::HasModelLoaded(modelId) )
		return nil;

	bool isBoat = CModelInfo::IsBoatModel(modelId);
	int node = -1;
	if ( !isBoat ) {
		node = ThePaths.FindNodeClosestToCoors(FindPlayerCoors(), 0, NEAREST_ROAD_SEARCH_RANGE, false, false);
		if ( node < 0 )
			return nil;
	}

	CVehicle *vehicle;
	if ( isBoat ) {
		vehicle = new CBoat(modelId, RANDOM_VEHICLE);
		vehicle->SetPosition(TheCamera.GetPosition() + TheCamera.GetForward() * BOAT_SPAWN_DISTANCE);
	} else {
		vehicle = new CAutomobile(modelId, RANDOM_VEHICLE);
		vehicle->SetPosition(ThePaths.m_pathNodes[node].GetPosition());
	}

	vehicle->bHasBeenOwnedByPlayer = true;
	vehicle->GetMatrix().GetPosition().z += SPAWN_HEIGHT;
	vehicle->SetOrientation(0.0f, 0.0f, SPAWN_HEADING);
	vehicle->SetStatus(STATUS_ABANDONED);
	vehicle->m_nDoorLock = CARLOCK_UNLOCKED;
	CWorld::Add(vehicle);
	return vehicle;
}
