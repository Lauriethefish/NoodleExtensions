#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"

#include "GlobalNamespace/BeatmapData.hpp"
#include "GlobalNamespace/BeatmapEventData.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData_NoteSpawnData.hpp"
#include "GlobalNamespace/BeatmapObjectSpawnMovementData_ObstacleSpawnData.hpp"
#include "GlobalNamespace/NoteCutDirection.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/NoteLineLayer.hpp"
#include "GlobalNamespace/ObstacleData.hpp"
#include "System/ValueType.hpp"

#include "AssociatedData.h"
#include "NEHooks.h"
#include "NELogger.h"
#include "SpawnDataHelper.h"
#include "custom-json-data/shared/CustomBeatmapData.h"

#include <cmath>

using namespace GlobalNamespace;
using namespace NEVector;

BeatmapObjectSpawnController * beatmapObjectSpawnController;

MAKE_HOOK_MATCH(BeatmapObjectSpawnController_Start, &BeatmapObjectSpawnController::Start,
                void,
                BeatmapObjectSpawnController *self) {
    beatmapObjectSpawnController = self;
    BeatmapObjectSpawnController_Start(self);
}

//  bool GetSliderSpawnData(SliderData sliderData, ref BeatmapObjectSpawnMovementData.SliderSpawnData result)
//        {
//            if (!_deserializedData.Resolve(sliderData, out NoodleBaseSliderData? noodleData))
//            {
//                return true;
//            }
//
//            float? njs = noodleData.NJS;
//            float? spawnoffset = noodleData.SpawnOffset;
//
//            bool gravityOverride = noodleData.DisableGravity;
//
//            float offset = _movementData.noteLinesCount / 2f;
//            float headLineIndex = noodleData.StartX + offset ?? sliderData.headLineIndex;
//            float headLineLayer = noodleData.StartY ?? (float)sliderData.headLineLayer;
//            float headStartlinelayer = noodleData.InternalStartNoteLineLayer + offset ?? (float)sliderData.headBeforeJumpLineLayer;
//            float tailLineIndex = noodleData.TailStartX + offset ?? sliderData.tailLineIndex;
//            float tailLineLayer = noodleData.TailStartY ?? (float)sliderData.tailLineLayer;
//            float tailStartlinelayer = noodleData.InternalTailStartNoteLineLayer + offset ?? (float)sliderData.tailBeforeJumpLineLayer;
//
//            Vector3 headOffset = GetNoteOffset(headLineIndex, headStartlinelayer);
//            Vector3 tailOffset = GetNoteOffset(tailLineIndex, tailStartlinelayer);
//            GetNoteJumpValues(
//                njs,
//                spawnoffset,
//                out float jumpDuration,
//                out float jumpDistance,
//                out Vector3 moveStartPos,
//                out Vector3 moveEndPos,
//                out Vector3 jumpEndPos);
//
//            NoteJumpGravityForLineLayer(
//                headLineLayer,
//                headStartlinelayer,
//                jumpDistance,
//                njs,
//                out float headJumpGravity,
//                out float headNoGravity);
//
//            NoteJumpGravityForLineLayer(
//                tailLineLayer,
//                tailStartlinelayer,
//                jumpDistance,
//                njs,
//                out float tailJumpGravity,
//                out float tailNoGravity);
//
//            result = new BeatmapObjectSpawnMovementData.SliderSpawnData(
//                moveStartPos + headOffset,
//                moveEndPos + headOffset,
//                jumpEndPos + headOffset,
//                gravityOverride ? headNoGravity : headJumpGravity,
//                moveStartPos + tailOffset,
//                moveEndPos + tailOffset,
//                jumpEndPos + tailOffset,
//                gravityOverride ? tailNoGravity : tailJumpGravity,
//                _movementData.moveDuration,
//                jumpDuration);
//
//            // IDK!!!!!!!
//            float num2 = jumpDuration * 0.5f;
//            float startVerticalVelocity = headJumpGravity * num2;
//            float yOffset = (startVerticalVelocity * num2) - (headJumpGravity * num2 * num2 * 0.5f);
//            noodleData.InternalNoteOffset = _movementData.centerPos + headOffset + new Vector3(0, yOffset, 0);
//
//            return false;
//        }

MAKE_HOOK_MATCH(GetObstacleSpawnData, &BeatmapObjectSpawnMovementData::GetObstacleSpawnData,
                BeatmapObjectSpawnMovementData::ObstacleSpawnData,
                BeatmapObjectSpawnMovementData *self, ObstacleData *normalObstacleData) {
    if (!Hooks::isNoodleHookEnabled())
        return GetObstacleSpawnData(self, normalObstacleData);

    auto *obstacleData = reinterpret_cast<CustomJSONData::CustomObstacleData *>(normalObstacleData);
    BeatmapObjectSpawnMovementData::ObstacleSpawnData result =
        GetObstacleSpawnData(self, obstacleData);

    // No need to create a custom ObstacleSpawnData if there is no custom data to begin with
    if (!obstacleData->customData->value) {
        return result;
    }
    BeatmapObjectAssociatedData &ad = getAD(obstacleData->customData);

    float lineIndex = ad.objectData.startX ? *ad.objectData.startX + self->noteLinesCount / 2 : obstacleData->lineIndex;
    float lineLayer = ad.objectData.startY.value_or(obstacleData->lineLayer);

    std::optional<float> const& njs = ad.objectData.noteJumpMovementSpeed;
    std::optional<float> const& spawnOffset = ad.objectData.noteJumpStartBeatOffset;

    auto const& scale = ad.objectData.scale;
    std::optional<float> height = scale && scale->at(1) ? scale->at(1) : std::nullopt;
    std::optional<float> width = scale && scale->at(0) ? scale->at(0) : std::nullopt;

    Vector3 obstacleOffset =
            SpawnDataHelper::GetObstacleOffset(self, lineIndex, lineLayer);
    obstacleOffset.y += self->get_jumpOffsetY();



    // original code has this line, not sure how important it is
    ////obstacleOffset.y = Mathf.Max(obstacleOffset.y, this._verticalObstaclePosY);

    float obstacleHeight;
    if (height.has_value()) {
        obstacleHeight = height.value() * 0.6f;
    } else {
        // _topObstaclePosY =/= _obstacleTopPosY
        obstacleHeight = std::min(
                obstacleData->height * 0.6f,
                self->obstacleTopPosY - obstacleOffset.y);
    }

    float jumpDuration;
    float jumpDistance;
    Vector3 localMoveStartPos;
    Vector3 localMoveEndPos;
    Vector3 localJumpEndPos;
    SpawnDataHelper::GetNoteJumpValues(beatmapObjectSpawnController->initData, self, njs, spawnOffset, jumpDuration,
                                       jumpDistance,
                                       localMoveStartPos, localMoveEndPos, localJumpEndPos);

    result = BeatmapObjectSpawnMovementData::ObstacleSpawnData(
        localMoveStartPos + obstacleOffset, localMoveEndPos + obstacleOffset, localJumpEndPos + obstacleOffset, obstacleHeight, result.moveDuration, jumpDuration,
        NECaches::get_noteLinesDistanceFast());

    float xOffset = ((width.value_or(obstacleData->lineIndex) / 2.0f) - 0.5f) * NECaches::get_noteLinesDistanceFast();
    Vector3 internalOffset = self->centerPos + obstacleOffset;
    internalOffset.x += xOffset;
    ad.noteOffset = internalOffset;

    return result;
}

MAKE_HOOK_MATCH(GetJumpingNoteSpawnData, &BeatmapObjectSpawnMovementData::GetJumpingNoteSpawnData,
                BeatmapObjectSpawnMovementData::NoteSpawnData, BeatmapObjectSpawnMovementData *self,
                NoteData *normalNoteData) {
    if (!Hooks::isNoodleHookEnabled())
        return GetJumpingNoteSpawnData(self, normalNoteData);

    auto noteDataCast = il2cpp_utils::try_cast<CustomJSONData::CustomNoteData>(normalNoteData);
    if (!noteDataCast)
        return GetJumpingNoteSpawnData(self, normalNoteData);

    auto noteData = *noteDataCast;
    if (!noteData->customData) {
        return GetJumpingNoteSpawnData(self, normalNoteData);
    }

    BeatmapObjectAssociatedData &ad = getAD(noteData->customData);

    auto const njs = ad.objectData.noteJumpMovementSpeed;
    std::optional<float> const spawnOffset = ad.objectData.noteJumpStartBeatOffset;
    std::optional<float> const flipLineIndex =
            ad.flip ? std::optional{ad.flip->x} : std::nullopt;

    float offset = self->noteLinesCount / 2.0f;

    bool const gravityOverride = ad.objectData.disableNoteGravity.value_or(false);

    float lineIndex = ad.objectData.startX ? *ad.objectData.startX + offset : noteData->lineIndex;
    float lineLayer = ad.objectData.startY.value_or(noteData->noteLineLayer);
    float const startLineLayer = ad.startNoteLineLayer ? *ad.startNoteLineLayer + offset : (float) noteData->beforeJumpNoteLineLayer;

    float jumpDuration = self->jumpDuration;

//    Vector3 moveStartPos = result.moveStartPos;
//    Vector3 moveEndPos = result.moveEndPos;
//    Vector3 jumpEndPos = result.jumpEndPos;
//    float jumpGravity = result.jumpGravity;

    Vector3 const noteOffset = SpawnDataHelper::GetNoteOffset(self, lineIndex, startLineLayer);


    float jumpDistance;
    Vector3 moveStartPos;
    Vector3 moveEndPos;
    Vector3 jumpEndPos;
    SpawnDataHelper::GetNoteJumpValues(beatmapObjectSpawnController->initData, self, njs, spawnOffset, jumpDuration,
                                       jumpDistance, moveStartPos, moveEndPos,
                                       jumpEndPos);

    float jumpGravity;
    float noGravity;

    SpawnDataHelper::NoteJumpGravityForLineLayer(self,

                                                 lineLayer,

                                                 startLineLayer,
                                                 jumpDistance,
                                                 njs,

                                                 jumpGravity,
                                                 noGravity
                                                 );

    float offsetStartRow = flipLineIndex.value_or(lineIndex);
    float offsetStartHeight =
            gravityOverride ? lineLayer
                            : startLineLayer;

    Vector3 const noteOffset2 =
            SpawnDataHelper::GetNoteOffset(self, offsetStartRow, offsetStartHeight);

    auto result = BeatmapObjectSpawnMovementData::NoteSpawnData(
            moveStartPos + noteOffset2, moveEndPos + noteOffset2, jumpEndPos + noteOffset, gravityOverride ? noGravity : jumpGravity, self->moveDuration, jumpDuration);



    // DEFINITE POSITION IS WEIRD, OK?
    // fuck
    float num2 = jumpDuration * 0.5f;
    float startVerticalVelocity = jumpGravity * num2;
    float yOffset = (startVerticalVelocity * num2) - (jumpGravity * num2 * num2 * 0.5f);
    ad.noteOffset = Vector3(self->centerPos) + noteOffset + Vector3(0, yOffset, 0);


    return result;
}

void InstallBeatmapObjectSpawnMovementDataHooks(Logger &logger) {
    INSTALL_HOOK(logger, GetObstacleSpawnData);
    INSTALL_HOOK(logger, GetJumpingNoteSpawnData);
    INSTALL_HOOK(logger, BeatmapObjectSpawnController_Start)
}

NEInstallHooks(InstallBeatmapObjectSpawnMovementDataHooks);