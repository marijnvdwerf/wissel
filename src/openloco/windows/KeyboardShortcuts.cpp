#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::KeyboardShortcuts
{

    static const int kRowHeight = 10;

    static window_event_list _events;
    static loco_global<char[16], 0x0112C826> _commonFormatArgs;
    static loco_global<uint8_t, 0x011364A4> _11364A4;

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 360, 238 }, widget_type::frame, 0, 0xFFFFFFFF),                                               // 0,
        make_widget({ 1, 1 }, { 358, 13 }, widget_type::caption_25, 0, string_ids::str_702),                                  // 1,
        make_widget({ 345, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), // 2,
        make_widget({ 0, 15 }, { 360, 223 }, widget_type::panel, 1, 0xFFFFFFFF),                                              // 3,
        make_widget({ 4, 19 }, { 352, 202 }, widget_type::scrollview, 1, vertical, string_ids::str_1001),                     // 4,
        make_widget({ 4, 223 }, { 150, 12 }, widget_type::wt_11, 1, string_ids::str_703, string_ids::str_704),                // 5,
        widget_end(),
    };

    //    {
    //        SDL_SCANCODE_ESCAPE, "⎋" Backspace, "⌫" Delete, "⌦",
    //            SDL_SCANCODE_CAPSLOCK, "⇪",
    //    ⇞ Page up
    //    ⇟ Page down
    //    ⇥ Tab
    //    ↘ End
    //    ↖ Home
    //    }

    namespace widx
    {
        enum
        {
            w2,
            w5,
        };
    }

    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);
    static void on_mouse_up(window* self, widget_index i);
    static void loc_4BE832(window* self);
    static void tooltip(window*, widget_index);
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index);

    void init_events()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.get_scroll_size = get_scroll_size;
        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;
    }

    // 0x004BE6C7
    window* open()
    {
        window* window;

        window = WindowManager::bringToFront(WindowType::keyboardShortcuts, 0);
        if (window != nullptr)
            return window;

        init_events();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(WindowType::keyboardShortcuts, { 360, 238 }, 0, &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::w2) | (1 << widx::w5);
        window->init_scroll_widgets();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_10;

        window->row_count = 35;
        window->row_hover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);
    }
    // 0x004BE72C
    static void draw_scroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        auto colour = window->colours[1];
        auto shade = colour::get_shade(colour, 4);
        gfx::clear_single(*dpi, shade);

        loco_global<string_id, 0x112C826> common_format_args;
        auto dx = 0;
        for (auto i = 0; i < window->row_count; i++)
        {
            string_id ax = string_ids::white_stringid2;
            if (i == window->row_hover)
            {
                gfx::draw_rect(dpi, 0, dx, 800, kRowHeight, 0x2000030);
                ax = string_ids::wcolour2_stringid2;
            }

            addr<0x112C826 + 2, string_id>() = string_ids::str_705 + i; //string id
            addr<0x112C826 + 4, string_id>() = 0;
            addr<0x112C826 + 6, string_id>() = 0;

            if (config::get().keyboard_shortcuts[i].var_0 != 0xFF)
            {
                addr<0x112C826 + 6, string_id>() = 740 + config::get().keyboard_shortcuts[i].var_0;

                if (config::get().keyboard_shortcuts[i].var_1 != 0)
                {
                    addr<0x112C826 + 4, string_id>() = string_ids::str_997;
                    if (config::get().keyboard_shortcuts[i].var_1 != 1)
                    {
                        addr<0x112C826 + 4, string_id>() = string_ids::str_998;
                    }
                }
            }

            addr<0x112C826 + 0, string_id>() = string_ids::str_996;

            gfx::draw_string_494B3F(*dpi, 0, dx - 1, colour::black, ax, _commonFormatArgs);
            dx += kRowHeight;
        }
    }

    // 0x004BE821
    static void on_mouse_up(window* self, widget_index i)
    {
        switch (i)
        {
            case widx::w2:
                WindowManager::close(self);
                return;

            case widx::w5:
                loc_4BE832(self);
                return;
        }
    }

    // 0x004BE832
    static void loc_4BE832(window* self)
    {
        call(0x004BE3F3);
        openloco::config::write();
        self->invalidate();
    }

    // 0x004BE844
    static void tooltip(window*, widget_index)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = string_ids::tooltip_scroll_list;
    }

    // 0x004BE84E
    static void get_scroll_size(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = 35 * kRowHeight;
    }

    // 0x004BE853
    static void on_scroll_mouse_over(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / kRowHeight;

        if (row >= window->row_count)
            return;

        if (row != window->row_hover)
        {
            window->row_hover = row;
            window->invalidate();
        }
    }

    // 0x004BE87B
    static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / kRowHeight;

        if (row >= self->row_count)
            return;

        EditKeyboardShortcut::open(row);
    }
}
