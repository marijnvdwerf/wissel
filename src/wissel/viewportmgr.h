#pragma once

#include "map/tile.h"
#include "station.h"
#include "things/thing.h"
#include "window.h"
#include <array>

namespace wissel::ui::viewportmgr
{
    enum class ZoomLevel : uint8_t
    {
        full = 0,
        half = 1,
        quarter = 2,
        eight = 3,
    };

    constexpr int16_t viewportsPerWindow = 2;

    void init();
    void registerHooks();
    void updatePointers();
    void create(window* window, int viewportIndex, gfx::Point origin, gfx::Size size, ZoomLevel zoom, thing_id_t thing_id);
    void create(window* window, int viewportIndex, gfx::Point origin, gfx::Size size, ZoomLevel zoom, map::map_pos3 tile);
    void invalidate(station* station);
    void invalidate(thing_base* t, ZoomLevel zoom);
    void invalidate(map::map_pos pos, map::coord_t di, map::coord_t si, ZoomLevel zoom = ZoomLevel::eight, int range = 32);
}
