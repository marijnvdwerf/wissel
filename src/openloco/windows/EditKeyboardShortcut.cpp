#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::EditKeyboardShortcut
{

    static window_event_list _events;
    static loco_global<uint8_t, 0x011364A4> _11364A4;
    static loco_global<char[16], 0x0112C826> _commonFormatArgs;

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 280, 72 }, widget_type::frame, 0, 0xFFFFFFFF),                                                // 0,
        make_widget({ 1, 1 }, { 278, 13 }, widget_type::caption_25, 0, string_ids::str_999),                                  // 1,
        make_widget({ 265, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), // 2,
        make_widget({ 0, 15 }, { 280, 57 }, widget_type::panel, 1, 0xFFFFFFFF),                                               // 3,
        widget_end(),
    };

    static void on_mouse_up(window* self, widget_index i);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    void init_events()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.draw = draw;
    }

    namespace widx
    {
        enum
        {
            w2,
            w5,
        };
    }

    // 0x004BF7B9
    window* open(uint8_t row)
    {
        WindowManager::close(WindowType::editKeyboardShortcut);
        _11364A4 = row;

        // TODO: only needs to be called once
        init_events();

        auto child = WindowManager::createWindow(WindowType::editKeyboardShortcut, { 280, 72 }, 0, &_events);

        child->widgets = _widgets;
        child->enabled_widgets = 1 << widx::w2;
        child->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        child->colours[0] = skin->colour_0B;
        child->colours[1] = skin->colour_10;

        return child;
    }

    // 0x004BE8DF
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);

        addr<0x112C826, string_id>() = string_ids::str_705 + _11364A4;
        auto point = gfx::Point(window->x + 140, window->y + 32);
        gfx::draw_string_centred_wrapped(dpi, &point, 272, 0, string_ids::str_1000, _commonFormatArgs);
    }

    // 0x004BE821
    static void on_mouse_up(window* self, widget_index i)
    {
        switch (i)
        {
            case widx::w2:
                WindowManager::close(self);
                return;
        }
    }

}
