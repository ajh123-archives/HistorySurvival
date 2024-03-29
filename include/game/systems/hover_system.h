#pragma once

#include <entt/entity/entity.hpp>
#include <entt/entt.hpp>
#include "core/vs_app.h"
#include "core/vs_debug_draw.h"
#include "game/components/inputs.h"
#include "game/components/hoverable.h"
#include "game/components/bounds.h"
#include "game/components/world_context.h"
#include "world/vs_world.h"

void updateHoverSystem(entt::registry& registry)
{
    auto& inputs = registry.ctx().get<Inputs>();

    const auto& worldContext = registry.ctx().get<WorldContext>();

    inputs.hoverEntity = entt::null;

    if (inputs.mouseTrace.bHasHit)
    {
        registry.view<Hoverable, Location, Bounds>().each([&inputs, &worldContext](
                                                              const auto entity,
                                                              const Hoverable& hoverable,
                                                              const Location& location,
                                                              const Bounds& bounds) {
            // Move mouse trace along normal to ensure we are inside the bounds
            const auto mouseLocationLocal =
                inputs.mouseTrace.hitLocation - inputs.mouseTrace.hitNormal * 0.1F - location;
            if (bounds.isLocationInside(mouseLocationLocal))
            {
                inputs.hoverEntity = entity;
                // Add visual indicator
                worldContext.world->getDebugDraw()->drawBox(
                    {location + bounds.min, location + bounds.max}, hoverable.hoverColor);
                // For now stop on the first intersection
                return;

            }
        });
    }
}
