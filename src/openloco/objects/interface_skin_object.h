#pragma once

#include "../graphics/Palette.h"
#include "../localisation/stringmgr.h"

using namespace openloco::graphics;

namespace openloco
{
#pragma pack(push, 1)
    struct interface_skin_object
    {
        string_id name;
        uint32_t img;
        Palette colour_06;
        Palette colour_07;
        Palette colour_08;
        Palette colour_09;
        Palette colour_0A;
        Palette colour_0B;
        Palette colour_0C;
        Palette colour_0D;
        Palette colour_0E;
        Palette colour_0F;
        Palette colour_10;
        Palette colour_11;
        Palette colour_12;
        Palette colour_13;
        Palette colour_14;
        Palette colour_15;
        Palette colour_16;
        Palette colour_17;
    };

    namespace interface_skin::image_ids
    {
        constexpr uint32_t phone = 188;
    }
#pragma pack(pop)
}
