#pragma once

#include "../graphics/Palette.h"
#include <cstdint>

using namespace openloco::graphics;

namespace openloco::ui
{
    constexpr uint8_t translucent_flag = 1 << 7;

#pragma pack(push, 1)
    struct WindowPalette
    {
        uint8_t value;

        constexpr Palette getPalette();
        constexpr bool isTranslucent();

        WindowPalette() = default;

        WindowPalette(uint8_t i)
        {
            value = i;
        }

        WindowPalette(const Palette& p)
        {
            value = p;
        }

        static constexpr WindowPalette translucent(Palette palette)
        {
            WindowPalette out = {};
            out.value = palette | translucent_flag;

            return out;
        }

        static constexpr WindowPalette translucent(WindowPalette palette)
        {
            return translucent(palette.getPalette());
        }
    };
#pragma pack(pop)

    constexpr Palette WindowPalette::getPalette()
    {
        return (Palette)(value & ~translucent_flag);
    }

    constexpr bool WindowPalette::isTranslucent()
    {
        return (value & translucent_flag) != 0;
    }
}
