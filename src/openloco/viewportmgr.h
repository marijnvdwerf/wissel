#pragma once

#include "things/thing.h"
#include "window.h"
#include <array>

namespace openloco::ui::viewportmgr
{
    constexpr size_t max_viewports = 10;
    void init();
    void registerHooks();
    std::array<viewport*, max_viewports> viewports();
    void updatePointers();
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, uint8_t zoom, thing_id_t thing_id);
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, uint8_t zoom, uint16_t tile_x, uint16_t tile_y, uint16_t tile_z);
}
