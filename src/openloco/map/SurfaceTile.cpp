#include "../industrymgr.h"
#include "../interop/interop.hpp"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include "../window.h"
#include "tile.h"
#include "tilemgr.h"
#include <cassert>

using namespace openloco;
using namespace openloco::interop;
using namespace openloco::ui;

#pragma pack(push, 1)
struct unk1
{
    uint16_t x; //0x00
    uint16_t y; // 0x02
    uint16_t var_4;
};
#pragma pack(pop)

static loco_global<utility::prng, 0x00525E20> _prng;
static loco_global<unk1[64], 0x009586DC> _9586DC;
static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;

static std::array<map_pos, 4> _offsets = { {
    map_pos(+32, 0),
    map_pos(-32, 0),
    map_pos(0, +32),
    map_pos(0, -32),
} };

static void sub_4CC20F(int16_t ax, int16_t cx, int16_t di, int16_t si)
{
    registers regs;
    regs.ax = ax;
    regs.cx = cx;
    regs.di = di;
    regs.si = si;
    call(0x004CC20F, regs);
}

static ui::window* wloc_46960C(map_pos position)
{
    for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
    {
        auto w = WindowManager::get(i);
        if (w->viewports[0] == nullptr)
            continue;

        auto viewport = w->viewports[0];
        if (viewport->zoom != 0)
            continue;

        if (!viewport->contains({ position.x, position.y }))
            continue;

        return w;
    }

    return nullptr;
}

void map::surface_element::loc_46959C(int16_t x, int16_t y, int ebx)
{
    auto coord2D = coordinate_3d_to_2d(x + 16, y + 16, this->water() * 16, gCurrentRotation);
    auto w = wloc_46960C(coord2D);
    if (w == nullptr)
        return;

    uint16_t dx2 = _prng->rand_next() & 0xFFFF;
    if (dx2 > 0x1745)
        return;

    for (auto offset : _offsets)
    {
        if (x + offset.x > 0x2FFF)
            return;
        if (y + offset.y > 0x2FFF)
            return;
        auto tile = map::tilemgr::get(x + offset.x, y + offset.y);
        if (tile.is_null())
            return;
        auto surface = tile.surface();
        if (surface->water() == 0)
            return;
    }

    _9586DC[ebx].x = x;
    _9586DC[ebx].y = y;
    _9586DC[ebx].var_4 = 0;
    this->set_flag_6();

    sub_4CC20F(x, y, this->water() * 16, this->water() * 16);
}
