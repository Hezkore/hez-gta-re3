#pragma once

class CVehicle;
class CVector;

// Command line options such as -cheat, -spawncar and -teleport, which act once the player can play
namespace StartOptions
{
	void AddCheat(const char *cheat);
	void SetCar(const char *modelName);
	void SetTeleport(const char *coordinates);
	void Apply(void);
#ifdef DEBUG
	// -pad control=value@milliseconds,... plays pad presses through the standard pad, for tests
	void SetPadFeed(const char *events);
	void FeedPad(void);
#endif
}

CVehicle *SpawnCarOnNearestRoad(int32 modelId);
// Without a height, the player lands on the ground at x and y
void TeleportPlayer(CVector position, bool hasHeight);
