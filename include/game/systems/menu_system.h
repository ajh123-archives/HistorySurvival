#pragma once

#include <entt/entt.hpp>
#include <glm/fwd.hpp>
#include "core/vs_camera.h"
#include "game/components/inputs.h"
#include "game/components/ui_context.h"
#include "game/components/world_context.h"
#include "world/generator/vs_terrain.h"
#include "core/vs_app.h"

// TODO cleanup this is a mess!
void updateMenuSystem(entt::registry& mainRegistry)
{
    auto& uiContext = mainRegistry.ctx<UIContext>();
    auto& worldContext = mainRegistry.ctx<WorldContext>();

    auto* app = VSApp::getInstance();

    if (uiContext.bShouldSetEditorActive)
    {
        app->setWorldActive(uiContext.editorWorldName);
        worldContext.world = app->getWorld();
        uiContext.bShouldSetEditorActive = false;
    }
    if (uiContext.bShouldSetGameActive)
    {
        app->setWorldActive(uiContext.gameWorldName);
        worldContext.world = app->getWorld();
        uiContext.bShouldSetGameActive = false;
    }
    auto* world = worldContext.world;

    if (app->getWorldName() == uiContext.menuWorldName)
    {
        if (uiContext.bIsMenuWorldInitialized &&
            !world->getChunkManager()->shouldReinitializeChunks())
        {
            // Rotate camera while in menu
            world->getCamera()->setPitchYaw(
                0.F, world->getCamera()->getYaw() + worldContext.deltaSeconds * 3.F);

            const auto groundTraceStart = glm::vec3(0.F, 200.F, 0.F);
            const auto groundTraceResult = world->getChunkManager()->lineTrace(
                groundTraceStart, groundTraceStart - glm::vec3(0.F, 500.F, 0.F));

            const auto collisionFrontStart =
                groundTraceResult.hitLocation + groundTraceResult.hitNormal + 4.F;

            const auto collisionFrontTraceResult = world->getChunkManager()->lineTrace(
                collisionFrontStart, collisionFrontStart + world->getCamera()->getFront() * 6.f);

            auto cameraTargetPos = collisionFrontStart;

            if (collisionFrontTraceResult.bHasHit)
            {
                const auto collisionBackStart = collisionFrontTraceResult.hitLocation;

                const auto collisionBackEnd =
                    collisionBackStart - world->getCamera()->getFront() * 12.F;

                const auto collisionBackTraceResult =
                    world->getChunkManager()->lineTrace(collisionBackStart, collisionBackEnd);

                if (collisionBackTraceResult.bHasHit)
                {
                    cameraTargetPos = collisionBackTraceResult.hitLocation +
                                      world->getCamera()->getFront() * 0.1F;
                }
                else
                {
                    cameraTargetPos = collisionBackEnd;
                }
            }

            world->getCamera()->setPosition(glm::mix(
                world->getCamera()->getPosition(),
                cameraTargetPos,
                0.25F * worldContext.deltaSeconds));
        }

        if (!uiContext.bIsMenuWorldInitialized)
        {
            uiContext.bIsMenuWorldInitialized = true;
            uiContext.bShouldUpdateChunks = true;
            uiContext.bShouldGenerateTerrain = true;
        }
    }

    // Update world state with ui state
    if (uiContext.bShouldUpdateChunks)
    {
        glm::ivec2 chunkCount = {4, 4};

        if (uiContext.worldSize == 0)
        {
            // Small
            chunkCount = {32, 32};
        }
        else if (uiContext.worldSize == 1)
        {
            // Medium
            chunkCount = {64, 64};
        }
        else if (uiContext.worldSize == 2)
        {
            // Large
            chunkCount = {128, 128};
        }
        else if (uiContext.worldSize == 3)
        {
            // Tiny for debug
            chunkCount = {2, 2};
        }

        constexpr glm::ivec3 chunkSize = {16, 128, 16};

        world->getChunkManager()->setChunkDimensions(chunkSize, chunkCount);
        uiContext.bShouldUpdateChunks = false;
    }

    if (uiContext.bShouldGenerateTerrain && !world->getChunkManager()->shouldReinitializeChunks())
    {
        uiContext.bShouldGenerateTerrain = false;

        if (uiContext.selectedBiomeType == 0)
        {
            VSTerrainGeneration::buildStandard(world);
        }
        else if (uiContext.selectedBiomeType == 1)
        {
            VSTerrainGeneration::buildMountains(world);
        }
        else if (uiContext.selectedBiomeType == 2)
        {
            VSTerrainGeneration::buildDesert(world);
        }

        if (!uiContext.bEditorActive)
        {
            uiContext.minimap.bShouldUpdate = true;
        }
    }

    if (uiContext.bShouldResetEditor && !world->getChunkManager()->shouldReinitializeChunks())
    {
        uiContext.bShouldResetEditor = false;
        VSTerrainGeneration::buildEditorPlane(world);
    }
}