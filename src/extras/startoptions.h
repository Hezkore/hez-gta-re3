#pragma once

class CVehicle;

// Command line options such as -cheat and -spawncar, which act once the player can play
namespace StartOptions
{
	void AddCheat(const char *cheat);
	void SetCar(const char *modelName);
	void Apply(void);
}

CVehicle *SpawnCarOnNearestRoad(int32 modelId);
