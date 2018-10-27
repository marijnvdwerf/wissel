#pragma once

#include "../graphics/Palette.h"
#include "../localisation/stringmgr.h"
#include <cstdint>

using namespace openloco::graphics;

namespace openloco::graphics
{
#pragma pack(push, 1)
    struct TextPalette
    {
        uint8_t value;

        TextPalette() = default;

        TextPalette(const Palette& p)
        {
            value = p;
        }
        explicit TextPalette(int8_t i)
        {
            value = i;
        }
        Palette getPalette()
        {
            return (Palette)(this->value & 0x1F);
        }
        void setFlag5(bool set = true)
        {
            if (set)
            {
                this->value |= format_flags::textflag_5;
            }
            else
            {
                this->value &= ~format_flags::textflag_5;
            }
        }
        bool hasFlag5()
        {
            return (this->value & format_flags::textflag_5) != 0;
        }
        void setFlag6(bool set = true)
        {
            if (set)
            {
                this->value |= format_flags::textflag_6;
            }
            else
            {
                this->value &= ~format_flags::textflag_6;
            }
        }
        bool hasFlag6()
        {
            return (this->value & format_flags::textflag_6) != 0;
        }
    };
#pragma pack(pop)
}
