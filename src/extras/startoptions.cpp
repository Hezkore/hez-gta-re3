#include "common.h"

#include <ctype.h>
#include <math.h>

#include "startoptions.h"
#include "Automobile.h"
#include "Bike.h"
#include "Boat.h"
#include "Camera.h"
#include "ModelInfo.h"
#include "Pad.h"
#include "PathFind.h"
#include "PlayerPed.h"
#include "Streaming.h"
#include "Timer.h"
#include "World.h"

#define CHEATS_SIZE 64
#define CAR_NAME_SIZE 32
#define NEAREST_ROAD_SEARCH_RANGE 100.0f
#define SPAWN_HEIGHT 4.0f
#define SPAWN_HEADING 3.49f
#define BOAT_SPAWN_DISTANCE 15.0f
#define TELEPORT_GROUND_VALUES 2
#define TELEPORT_ALL_VALUES 3

static char cheats[CHEATS_SIZE];
static char carName[CAR_NAME_SIZE];
static bool isTeleportWaiting;
static bool hasTeleportHeight;
static CVector teleportPosition;

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

void
StartOptions::SetTeleport(const char *coordinates)
{
	CVector position(0.0f, 0.0f, 0.0f);
	int count = sscanf(coordinates, "%f,%f,%f", &position.x, &position.y, &position.z);
	if ( count < TELEPORT_GROUND_VALUES || !isfinite(position.x) || !isfinite(position.y) || !isfinite(position.z) ) {
		printf("-teleport needs x,y or x,y,z, not %s\n", coordinates);
		return;
	}
	teleportPosition = position;
	hasTeleportHeight = count == TELEPORT_ALL_VALUES;
	isTeleportWaiting = true;
}

static bool
IsPlayerInControl(void)
{
	// Loading and fading in still change the player, so wait until both are done
	return FindPlayerPed() != nil && !CPad::GetPad(0)->ArePlayerControlsDisabled()
		&& TheCamera.GetScreenFadeStatus() == FADE_0;
}

static bool
HasWaitingOptions(void)
{
	return cheats[0] != '\0' || carName[0] != '\0' || isTeleportWaiting;
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
	if ( !HasWaitingOptions() || !IsPlayerInControl() )
		return;

	if ( cheats[0] != '\0' )
		TypeCheats();
	if ( isTeleportWaiting ) {
		debug("teleporting to %f,%f,%f\n", teleportPosition.x, teleportPosition.y, teleportPosition.z);
		TeleportPlayer(teleportPosition, hasTeleportHeight);
		isTeleportWaiting = false;
	}
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
		if ( CModelInfo::IsBikeModel(modelId) )
			vehicle = new CBike(modelId, RANDOM_VEHICLE);
		else
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

void
TeleportPlayer(CVector position, bool hasHeight)
{
	CEntity *player = FindPlayerEntity();
	CStreaming::LoadScene(position);
	CStreaming::LoadSceneCollision(position);
	if ( !hasHeight )
		position.z = CWorld::FindGroundZForCoord(position.x, position.y) + player->GetDistanceFromCentreOfMassToBaseOfModel();
	player->Teleport(position);
}

#ifdef DEBUG
#define PAD_FEED_EVENTS 64
#define PAD_FEED_CONTROL_SIZE 8

// One timed change of a pad control, counted from the moment the player can play
struct PadFeedEvent
{
	uint32 atMilliseconds;
	char control[PAD_FEED_CONTROL_SIZE];
	float value;
};

struct PadFeedControl
{
	const char *name;
	bool CStandardPadState::*button;
	float CStandardPadState::*axis;
};

static const PadFeedControl padFeedControls[] = {
	{ "a", &CStandardPadState::A, nil }, { "b", &CStandardPadState::B, nil },
	{ "x", &CStandardPadState::X, nil }, { "y", &CStandardPadState::Y, nil },
	{ "lb", &CStandardPadState::LeftBumper, nil }, { "rb", &CStandardPadState::RightBumper, nil },
	{ "back", &CStandardPadState::Back, nil }, { "start", &CStandardPadState::Start, nil },
	{ "ls", &CStandardPadState::LeftThumb, nil }, { "rs", &CStandardPadState::RightThumb, nil },
	{ "up", &CStandardPadState::DPadUp, nil }, { "down", &CStandardPadState::DPadDown, nil },
	{ "left", &CStandardPadState::DPadLeft, nil }, { "right", &CStandardPadState::DPadRight, nil },
	{ "lt", nil, &CStandardPadState::LeftTrigger }, { "rt", nil, &CStandardPadState::RightTrigger },
	{ "lx", nil, &CStandardPadState::LeftX }, { "ly", nil, &CStandardPadState::LeftY },
	{ "rx", nil, &CStandardPadState::RightX }, { "ry", nil, &CStandardPadState::RightY },
};

static PadFeedEvent padFeedEvents[PAD_FEED_EVENTS];
static int padFeedEventCount;
static int padFeedNextEvent;
static CStandardPadState padFeedState;
static bool isPadFeedRunning;
static uint32 padFeedStartMilliseconds;

static const PadFeedControl *
FindPadFeedControl(const char *name)
{
	for ( int i = 0; i < ARRAY_SIZE(padFeedControls); i++ )
		if ( !strcmp(padFeedControls[i].name, name) )
			return &padFeedControls[i];
	return nil;
}

void
StartOptions::SetPadFeed(const char *events)
{
	const char *next = events;
	while ( *next && padFeedEventCount < PAD_FEED_EVENTS ) {
		PadFeedEvent &event = padFeedEvents[padFeedEventCount];
		int length = 0;
		if ( sscanf(next, "%7[a-z]=%f@%u%n", event.control, &event.value, &event.atMilliseconds, &length) != 3
		    || !FindPadFeedControl(event.control) ) {
			printf("-pad needs control=value@milliseconds,..., not %s\n", next);
			padFeedEventCount = 0;
			return;
		}
		padFeedEventCount++;
		next += length;
		if ( *next == ',' )
			next++;
	}
}

static void
PrintPadFeedState(const PadFeedEvent &event)
{
	CPlayerPed *player = FindPlayerPed();
	CVehicle *vehicle = FindPlayerVehicle();
	CPhysical *mover = vehicle ? (CPhysical *)vehicle : (CPhysical *)player;
	float forwardSpeed = DotProduct(mover->GetMoveSpeed(), mover->GetForward());
	printf("pad feed %s=%.2f at %u ms: ped state %d, weapon %d, camera mode %d, forward speed %.3f, setup %d\n",
		event.control, event.value, event.atMilliseconds, player->GetPedState(), player->GetWeapon()->m_eWeaponType,
		TheCamera.Cams[TheCamera.ActiveCam].Mode, forwardSpeed, CPad::GetPad(0)->Mode);
}

void
StartOptions::FeedPad(void)
{
	if ( padFeedEventCount == 0 )
		return;
	if ( !isPadFeedRunning ) {
		// the other options first, so the presses land on the start car or the teleported player
		if ( HasWaitingOptions() || !IsPlayerInControl() )
			return;
		isPadFeedRunning = true;
		padFeedStartMilliseconds = CTimer::GetTimeInMilliseconds();
	}

	uint32 elapsed = CTimer::GetTimeInMilliseconds() - padFeedStartMilliseconds;
	for ( ; padFeedNextEvent < padFeedEventCount && padFeedEvents[padFeedNextEvent].atMilliseconds <= elapsed; padFeedNextEvent++ ) {
		const PadFeedEvent &event = padFeedEvents[padFeedNextEvent];
		const PadFeedControl *control = FindPadFeedControl(event.control);
		if ( control->button )
			padFeedState.*control->button = event.value != 0.0f;
		else
			padFeedState.*control->axis = event.value;
		PrintPadFeedState(event);
	}
	CPad::GetPad(0)->AffectFromStandardPad(padFeedState);
}
#endif
