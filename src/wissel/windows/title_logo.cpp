#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../ui/WindowManager.h"
#include "../wissel.h"

using namespace wissel::interop;

namespace wissel::ui::windows
{
    static const gfx::Size window_size = { 298, 170 };

    namespace widx
    {
        enum
        {
            logo
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, window_size, widget_type::wt_3, 0),
        widget_end(),
    };

    static window_event_list _events;

    static void on_mouse_up(window* window, widget_index widgetIndex);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi);

    ui::window* open_title_logo()
    {
        _events.on_mouse_up = on_mouse_up;
        _events.draw = draw;

        auto window = wissel::ui::WindowManager::createWindow(
            WindowType::title_logo,
            { 0, 0 },
            window_size,
            window_flags::stick_to_front | window_flags::transparent,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = 1 << widx::logo;

        window->init_scroll_widgets();

        window->colours[0] = colour::translucent(colour::grey);
        window->colours[1] = colour::translucent(colour::grey);

        return window;
    }

    // 0x00439298
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        gfx::draw_image(dpi, window->x, window->y, image_ids::locomotion_logo);
    }

    // 0x004392AD
    static void on_mouse_up(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::logo:
                about::open();
                break;
        }
    }
}
