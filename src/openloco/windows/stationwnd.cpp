#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/cargo_object.h"
#include "../objects/objectmgr.h"
#include "../stationmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows
{
    static station_id_t get_station_id(const window& w)
    {
        return w.number;
    }

    static station& get_station(const window& w)
    {
        return *(stationmgr::get(get_station_id(w)));
    }

    // 0x0048EF02
    static void draw_rating_bar(window& w, gfx::drawpixelinfo_t& dpi, int16_t x, int16_t y, uint8_t amount, Palette colour)
    {
        registers regs;
        regs.al = amount;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)&w;
        regs.edi = (int32_t)&dpi;
        regs.ebp = colour;
        call(0x0048EF02, regs);
    }

    // 0x0048ED2F
    void station_2_scroll_paint(window& w, gfx::drawpixelinfo_t& dpi)
    {
        auto paletteId = colour::get_shade(w.palettes[1].getPalette(), 4);
        gfx::clear_single(dpi, paletteId);

        const auto& station = get_station(w);
        int16_t y = 0;
        for (int i = 0; i < 32; i++)
        {
            auto& cargo = station.cargo_stats[i];
            if (!cargo.empty())
            {
                auto cargoObj = objectmgr::get<cargo_object>(i);
                gfx::draw_string_494BBF(dpi, 1, y, 98, Palette::black, string_ids::wcolour2_stringid2, &cargoObj->name);

                auto rating = cargo.rating;
                auto colour = Palette::moss_green;
                if (rating < 100)
                {
                    colour = Palette::dark_olive_green;
                    if (rating < 50)
                    {
                        colour = Palette::saturated_red;
                    }
                }
                uint8_t amount = (rating * 327) / 256;
                draw_rating_bar(w, dpi, 100, y, amount, colour);

                uint16_t percent = rating / 2;
                gfx::draw_string_494B3F(dpi, 201, y, Palette::black, string_ids::station_cargo_rating_percent, &percent);
                y += 10;
            }
        }
    }
}
