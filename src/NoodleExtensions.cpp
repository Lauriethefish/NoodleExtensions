#include "Animation/Events.h"
#include "NEHooks.h"

using namespace NoodleExtensions;

void NoodleExtensions::InstallHooks() {
    Logger& logger = NELogger::GetLogger();
    InstallBeatmapObjectCallbackControllerHooks(logger);
    InstallBeatmapObjectSpawnMovementDataHooks(logger);
    InstallBeatmapDataTransformHelperHooks(logger);
    InstallNoteCutSoundEffectManagerHooks(logger);
    InstallSpawnRotationProcessorHooks(logger);
    InstallGameplayCoreInstallerHooks(logger);
    InstallBeatmapObjectManagerHooks(logger);
    InstallObstacleControllerHooks(logger);
    InstallGameNoteControllerHooks(logger);
    InstallBombNoteControllerHooks(logger);
    InstallNoteFloorMovementHooks(logger);
    InstallBeatmapDataLoaderHooks(logger);
    InstallNoteControllerHooks(logger);
    InstallNoteJumpHooks(logger);
    InstallClampPatches(logger);

    Events::AddEventCallbacks(logger);
}