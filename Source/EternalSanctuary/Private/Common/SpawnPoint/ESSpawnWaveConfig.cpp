#include "Common/SpawnPoint/ESSpawnWaveConfig.h"

float UESSpawnWaveConfig::GetDifficultyMultiplier() const
{
	switch (Difficulty)
	{
	case EESGameDifficulty::Easy:	return 1.0f;
	case EESGameDifficulty::Normal:	return 1.0f;
	case EESGameDifficulty::Hard:	return 1.3f;
	case EESGameDifficulty::Hell:	return 1.6f;
	default:				return 1.0f;
	}
}