#pragma once

#include "IndexedColour.h"
#include "Palette.h"
#include <cstdint>

using namespace openloco::graphics;

namespace openloco
{
    namespace colour
    {
        constexpr uint8_t translucent_flag = 1 << 7;

        constexpr Palette translucent(graphics::Palette c)
        {
            return (graphics::Palette)(c | translucent_flag);
        }

        constexpr Palette opaque(Palette c)
        {
            return (graphics::Palette)(c & ~translucent_flag);
        }

        IndexedColour get_shade(Palette palette, uint8_t shade);
    }
}
