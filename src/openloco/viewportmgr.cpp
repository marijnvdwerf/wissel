#include "viewportmgr.h"
#include "config.h"
#include "interop/interop.hpp"
#include "things/thing.h"
#include "things/thingmgr.h"
#include "ui.h"
#include "window.h"
#include <algorithm>

using namespace openloco::ui;
using namespace openloco::interop;

namespace openloco::ui::viewportmgr
{
    loco_global<viewport[max_viewports], 0x0113D758> _viewports;
    loco_global<viewport * [max_viewports], 0x0113D820> _viewportPointers;

    static void create(registers regs, int index);

    void init()
    {
        for (size_t i = 0; i < max_viewports; i++)
        {
            _viewports[i].width = 0;
        }

        _viewportPointers[0] = nullptr;
    }

    void registerHooks()
    {
        register_hook(
            0x004CA2D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                create(regs, 0);
                regs = backup;
                return 0;
            });
        register_hook(
            0x004CA38A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                create(regs, 1);
                regs = backup;
                return 0;
            });
    }

    std::array<viewport*, max_viewports> viewports()
    {
        auto arr = (std::array<viewport*, max_viewports>*)_viewportPointers.get();
        return *arr;
    }

    // 0x004CEC25
    void updatePointers()
    {
        call(0x004CEC25);
    }

    static viewport* init_viewport(gfx::point_t origin, gfx::ui_size_t size, uint8_t zoom)
    {
        viewport* vp = nullptr;
        for (size_t i = 0; i < max_viewports; i++)
        {
            if (_viewports[i].width != 0)
                continue;
            vp = &_viewports[i];
            break;
        }

        if (vp == nullptr)
        {
            return nullptr;
        }

        vp->x = origin.x;
        vp->y = origin.y;
        vp->width = size.width;
        vp->height = size.height;

        vp->view_width = size.width << zoom;
        vp->view_height = size.height << zoom;
        vp->zoom = zoom;
        vp->var_12 = 0;

        auto& cfg = openloco::config::get();
        if (cfg.flags & config::flags::gridlines_on_landscape)
        {
            vp->var_12 |= 0x20;
        }

        return vp;
    }

    static void focusViewportOn(window* w, int index, thing_id_t dx)
    {
        viewport* viewport = w->viewports[index];

        w->viewport_configurations[index].viewport_target_sprite = dx;

        auto t = thingmgr::get<thing>(dx);

        int16_t dest_x, dest_y;
        viewport->centre_2d_coordinates(t->x, t->y, t->z, &dest_x, &dest_y);
        w->viewport_configurations[index].saved_view_x = dest_x;
        w->viewport_configurations[index].saved_view_y = dest_y;
        viewport->view_x = dest_x;
        viewport->view_y = dest_y;
    }

    static void focusViewportOn(window* w, int index, uint16_t tile_x, uint16_t tile_y, uint16_t tile_z)
    {
        viewport* viewport = w->viewports[index];

        w->viewport_configurations[index].viewport_target_sprite = 0xFFFF;

        int16_t dest_x, dest_y;
        viewport->centre_2d_coordinates(tile_x, tile_y, tile_z, &dest_x, &dest_y);
        w->viewport_configurations[index].saved_view_x = dest_x;
        w->viewport_configurations[index].saved_view_y = dest_y;
        viewport->view_x = dest_x;
        viewport->view_y = dest_y;
    }

    static void create(registers regs, int index)
    {
        ui::window* window = (ui::window*)regs.esi;
        uint8_t zoom = 0;
        if (regs.edx & (1 << 30))
        {
            regs.edx &= (1 << 30);
            zoom = regs.cl;
        }

        int16_t x = regs.ax;
        int16_t y = regs.eax >> 16;
        uint16_t width = regs.bx;
        uint16_t height = regs.ebx >> 16;

        if (regs.edx & (1 << 31))
        {
            thing_id_t id = regs.dx;
            create(window, index, { x, y }, { width, height }, zoom, id);
        }
        else
        {
            uint16_t tile_x = regs.dx;
            uint16_t tile_y = regs.edx >> 16;
            uint16_t tile_z = regs.ecx >> 16;
            create(window, index, { x, y }, { width, height }, zoom, tile_x, tile_y, tile_z);
        }
    }

    void updatePointers();
    /* 0x004CA2D0
         * ax : x
         * eax >> 16 : y
         * bx : width
         * ebx >> 16 : height
         * cl : zoom
         * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
         * Optional one of 2
         * 1.
         * ecx >> 16 : tile_z
         * dx : tile_x
         * edx >> 16 : tile_y
         * 2.
         * dx : thing_id
         */
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, uint8_t zoom, thing_id_t thing_id)
    {
        viewport* viewport = init_viewport(origin, size, zoom);

        if (viewport == nullptr)
            return;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, thing_id);

        updatePointers();
    }

    /* 0x004CA2D0
     * ax : x
     * eax >> 16 : y
     * bx : width
     * ebx >> 16 : height
     * cl : zoom
     * edx >> 14 : flags (bit 30 zoom related, bit 31 set if thing_id used)
     * Optional one of 2
     * 1.
     * ecx >> 16 : tile_z
     * dx : tile_x
     * edx >> 16 : tile_y
     * 2.
     * dx : thing_id
     */
    void create(window* window, int viewportIndex, gfx::point_t origin, gfx::ui_size_t size, uint8_t zoom, uint16_t tile_x, uint16_t tile_y, uint16_t tile_z)
    {
        viewport* viewport = init_viewport(origin, size, zoom);

        if (viewport == nullptr)
            return;

        window->viewports[viewportIndex] = viewport;
        focusViewportOn(window, viewportIndex, tile_x, tile_y, tile_z);

        updatePointers();
    }
}
