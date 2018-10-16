#include "../interop/interop.hpp"
#include "colours.h"
#include <cassert>

using namespace openloco::interop;
using namespace openloco::graphics;

namespace openloco::colour
{

    loco_global<IndexedColour[32][8], 0x01136BA0> _colour_map_a;
    loco_global<IndexedColour[32][8], 0x01136C98> _colour_map_b;

    IndexedColour get_shade(Palette palette, uint8_t shade)
    {
        assert(palette <= 31);

        if (shade < 8)
        {
            return _colour_map_a[palette][shade];
        }

        return _colour_map_b[palette][shade - 8];
    }
}
