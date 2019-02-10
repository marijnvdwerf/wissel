#include "WindowManager.h"
#include "../audio/audio.h"
#include "../companymgr.h"
#include "../console.h"
#include "../graphics/colours.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../map/tile.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../tutorial.h"
#include "../ui.h"
#include "../viewportmgr.h"
#include "scrollview.h"
#include <algorithm>
#include <cinttypes>
#include <memory>

using namespace openloco::interop;

namespace openloco::ui::WindowManager
{
    namespace find_flag
    {
        constexpr uint16_t by_type = 1 << 7;
    }

    static loco_global<uint16_t, 0x00508F10> __508F10;
    static loco_global<gfx::drawpixelinfo_t, 0x0050B884> _screen_dpi;
    static loco_global<uint16_t, 0x0050C19C> time_since_last_tick;
    static loco_global<uint16_t, 0x0052334E> gWindowUpdateTicks;
    static loco_global<WindowType, 0x00523364> _callingWindowType;
    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523394> _toolWidgetIdx;
    static loco_global<uint8_t, 0x005233B6> _currentModalType;
    static loco_global<uint32_t, 0x00523508> _523508;
    static loco_global<int32_t, 0x00525330> _cursorWheel;
    static loco_global<uint32_t, 0x00525E28> _525E28;
    static loco_global<company_id_t, 0x009C68EB> _updating_company_id;
    static loco_global<uint32_t, 0x009DA3D4> _9DA3D4;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;

    static std::vector<std::unique_ptr<window>> _windows;

#define FOR_ALL_WINDOWS_FROM_BACK(w) for (auto& w : _windows)

    static void sub_4C6A40(ui::window* window, ui::viewport* viewport, int16_t dX, int16_t dY);

    static void sub_4C6A40(ui::window* window, ui::viewport* viewport, int16_t dX, int16_t dY);

    //    static std::vector<std::unique_ptr<window>>::iterator _find(window *w)
    //    {
    //        auto it = std::find_if(_windows.begin(), _windows.end(), [&w](std::unique_ptr<window> &ptr)
    //        {
    //            return w == ptr.get();
    //        });
    //
    //        return it;
    //    }

    void init()
    {
        // FIXME: dealloc
        _windows.clear();
        _523508 = 0;
    }

    void registerHooks()
    {
        register_hook(
            0x004C6A40,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4C6A40((ui::window*)regs.edi, (ui::viewport*)regs.esi, regs.dx, regs.bp);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045EFDB,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_out(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F015,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                window->viewport_zoom_in(false);
                regs = backup;
                return 0;
            });

        register_hook(
            0x0045F18B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                callViewportRotateEventOnAllWindows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004B93A5,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_4B93A5(regs.bx);
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9984,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                invalidateAllWindowsAfterInput();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004C9A95,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = findAt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = (uintptr_t)window;

                return 0;
            });

        register_hook(
            0x004C9AFA,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                auto window = findAtAlt(regs.ax, regs.bx);
                regs = backup;
                regs.esi = (uintptr_t)window;

                return 0;
            });

        register_hook(
            0x004C9B56,
            [](registers& regs) -> uint8_t {
                ui::window* w;
                if (regs.cx & find_flag::by_type)
                {
                    w = find((WindowType)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    w = find((WindowType)regs.cx, regs.dx);
                }

                regs.esi = (uintptr_t)w;
                if (w == nullptr)
                {
                    return X86_FLAG_ZERO;
                }

                return 0;
            });

        register_hook(
            0x004CB966,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                if (regs.al < 0)
                {
                    invalidateWidget((WindowType)(regs.al & 0x7F), regs.bx, regs.ah);
                }
                else if ((regs.al & 1 << 6) != 0)
                {
                    invalidate((WindowType)(regs.al & 0xBF));
                }
                else
                {
                    invalidate((WindowType)regs.al, regs.bx);
                }
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CC692,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                if ((regs.cx & find_flag::by_type) != 0)
                {
                    close((WindowType)(regs.cx & ~find_flag::by_type));
                }
                else
                {
                    close((WindowType)regs.cx, regs.dx);
                }
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CC6EA,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = (ui::window*)regs.esi;
                close(window);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004CD296,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                relocateWindows();
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CD3D0,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                dispatchUpdateAll();
                return 0;
            });

        register_hook(
            0x004CE438,
            [](registers& regs) -> uint8_t {
                auto w = getMainWindow();

                regs.esi = (uintptr_t)w;
                if (w == nullptr)
                {
                    return X86_FLAG_CARRY;
                }

                return 0;
            });

        register_hook(
            0x004CEE0B,
            [](registers& regs) -> uint8_t {
                registers backup = regs;
                sub_4CEE0B((ui::window*)regs.esi);
                regs = backup;

                return 0;
            });

        register_hook(
            0x004CF456,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                closeAllFloatingWindows();
                regs = backup;
                return 0;
            });

        register_hook(
            0x004C6118,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                update();
                regs = backup;
                return 0;
            });

        register_hook(
            0x004C96E7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                handle_input();
                regs = backup;
                return 0;
            });

        register_hook(
            0x004C9F5D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                console::log("create_window 0x%08X", regs.edx);

                auto w = createWindow((WindowType)regs.cl, gfx::Point(regs.ax, regs.eax >> 16), gfx::Size(regs.bx, regs.ebx >> 16), regs.ecx >> 8, (window_event_list*)regs.edx);
                regs = backup;

                regs.esi = (uintptr_t)w;
                return 0;
            });

        register_hook(
            0x004C9C68,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                console::log("create window alt");
                auto w = createWindow((WindowType)regs.cl, gfx::Size(regs.bx, (((uint32_t)regs.ebx) >> 16)), regs.ecx >> 8, (window_event_list*)regs.edx);
                regs = backup;

                regs.esi = (uintptr_t)w;

                return 0;
            });

        register_hook(
            0x004CD3A9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                console::log("bringToFront(0x%02X, %d)", regs.cx, regs.dx);
                auto w = bringToFront((WindowType)regs.cx, regs.dx);
                regs = backup;

                regs.esi = (uintptr_t)w;
                if (w == nullptr)
                {
                    return X86_FLAG_ZERO;
                }

                return 0;
            });

        register_hook(
            0x004C5E55,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;

                gfx::redraw_screen_rect(regs.ax, regs.bx, regs.dx, regs.bp);
                regs = backup;
                return 0;
            });
    }

    window* get(size_t index)
    {
        return _windows[index].get();
    }

    size_t count()
    {
        return _windows.size();
    }

    int indexOf(window* window)
    {
        int i = 0;
        for (auto& w : _windows)
        {
            if (w.get() == window)
            {
                return i;
            }
            i++;
        }

        return -1;
    }

    WindowType getCurrentModalType()
    {
        return (WindowType)*_currentModalType;
    }

    void setCurrentModalType(WindowType type)
    {
        _currentModalType = (uint8_t)type;
    }

    // 0x004C6118
    void update()
    {
        _tooltipNotShownTicks = _tooltipNotShownTicks + time_since_last_tick;
        _9DA3D4 = _9DA3D4 + 1;
        if (_9DA3D4 == 224 && ui::dirty_blocks_initialised())
        {
            if (_callingWindowType != WindowType::undefined)
            {
                _9DA3D4 = _9DA3D4 - 1;
            }
            else
            {
                // set_window_pos_wrapper();
            }
        }

        if (!ui::dirty_blocks_initialised())
        {
            return;
        }

        if (!intro::is_active())
        {
            gfx::draw_dirty_blocks();
        }

        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            w->viewports_update_position();
        }

        // 1000 tick update
        gWindowUpdateTicks = gWindowUpdateTicks + time_since_last_tick;
        if (gWindowUpdateTicks >= 1000)
        {
            gWindowUpdateTicks = 0;
            for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
            {
                auto& w = *it;
                w->call_6();
            }
        }

        // Border flash invalidation
        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;

            if ((w->flags & window_flags::white_border_mask) != 0)
            {
                // TODO: Replace with countdown
                w->flags -= window_flags::white_border_one;
                if ((w->flags & window_flags::white_border_mask) != 0)
                {
                    w->invalidate();
                }
            }
        }

        allWheelInput();
    }

    // 0x004CE438
    window* getMainWindow()
    {
        return find(WindowType::main);
    }

    // 0x004C9B56
    window* find(WindowType type)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type == type)
            {
                return w.get();
            }
        }

        return nullptr;
    }

    // 0x004C9B56
    window* find(WindowType type, window_number number)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type == type && w->number == number)
            {
                return w.get();
            }
        }

        return nullptr;
    }

    // 0x004C9A95
    window* findAt(int16_t x, int16_t y)
    {
        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;

            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & window_flags::flag_7) != 0)
                continue;

            if ((w->flags & window_flags::no_background) != 0)
            {
                auto index = w->find_widget_at(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->call_on_resize() == nullptr)
            {
                return findAt(x, y);
            }

            return &(*w);
        }

        return nullptr;
    }

    // 0x004C9AFA
    window* findAtAlt(int16_t x, int16_t y)
    {
        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;

            if (x < w->x)
                continue;

            if (x >= (w->x + w->width))
                continue;

            if (y < w->y)
                continue;
            if (y >= (w->y + w->height))
                continue;

            if ((w->flags & window_flags::no_background) != 0)
            {
                auto index = w->find_widget_at(x, y);
                if (index == -1)
                {
                    continue;
                }
            }

            if (w->call_on_resize() == nullptr)
            {
                return findAtAlt(x, y);
            }

            return &(*w);
        }

        return nullptr;
    }

    // 0x004CB966
    void invalidate(WindowType type)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type != type)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidate(WindowType type, window_number number)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            w->invalidate();
        }
    }

    // 0x004CB966
    void invalidateWidget(WindowType type, window_number number, uint8_t widget_index)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type != type)
                continue;

            if (w->number != number)
                continue;

            auto widget = w->widgets[widget_index];

            if (widget.left != -2)
            {
                gfx::set_dirty_blocks(
                    w->x + widget.left,
                    w->y + widget.top,
                    w->x + widget.right + 1,
                    w->y + widget.bottom + 1);
            }
        }
    }

    // 0x004C9984
    void invalidateAllWindowsAfterInput()
    {
        if (is_paused())
        {
            _523508++;
        }

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;

            w->update_scroll_widgets();
            w->invalidate_pressed_image_buttons();
            w->call_on_resize();
        }
    }

    // 0x004CC692
    void close(WindowType type)
    {
        bool repeat = true;
        while (repeat)
        {
            repeat = false;
            FOR_ALL_WINDOWS_FROM_BACK (w)
            {
                if (w->type != type)
                    continue;

                close(w.get());
                repeat = true;
                break;
            }
        }
    }

    // 0x004CC692
    void close(WindowType type, window_number id)
    {
        auto window = find(type, id);
        if (window != nullptr)
        {
            close(window);
        }
    }

    // 0x004CC750
    window* bringToFront(window* w)
    {
        return w;
        //        registers regs;
        //        regs.esi = (uint32_t)w;
        //        call(0x004CC750, regs);
        //
        //        return (window*)regs.esi;
    }

    // 0x004CD3A9
    window* bringToFront(WindowType type, uint16_t id)
    {
        auto window = find(type, id);
        if (window == nullptr)
            return nullptr;

        window->flags |= 0x60000;
        window->invalidate();

        return bringToFront(window);
    }

    /**
     * 0x004C9BEA
     *
     * @param x @<dx>
     * @param y @<ax>
     * @param width @<bx>
     * @param height @<cx>
     */
    static bool window_fits_within_space(int16_t x, int16_t y, uint16_t width, uint16_t height)
    {
        if (x < 0)
            return false;

        if (y < 28)
            return false;

        if (x + width > ui::width())
            return false;

        if (y + height > ui::width())
            return false;

        for (auto& w : _windows)
        {
            if ((w->flags & window_flags::stick_to_back) != 0)
                continue;
            if (x + width <= w->x)
                continue;
            if (x > w->x + w->width)
                continue;
            if (y + height <= w->y)
                continue;
            if (y >= w->y + w->height)
                continue;

            return false;
        }

        return true;
    }

    static window* loc_4C9F27(
        WindowType type,
        gfx::Point origin,
        gfx::Size size,
        uint32_t flags,
        window_event_list* events)
    {
        origin.x = std::clamp<typeof(origin.x)>(origin.x, 0, ui::width() - size.width);

        return createWindow(type, { origin.x, origin.y }, { size.width, size.height }, flags, events);
    }

    // sub_4C9BA2
    static bool window_fits_on_screen(gfx::Point origin, gfx::Size size)
    {
        if (origin.x < -(size.width / 4))
            return false;
        if (origin.x > ui::width() - (size.width / 2))
            return false;

        if (origin.y < 28)
            return false;
        if (origin.y > ui::height() - (size.height / 4))
            return false;

        return window_fits_within_space(origin.x, origin.y, size.width, size.height);
    }

    /**
     *
     * @param type @<cl>
     * @param size.width @<bx>
     * @param size.height @<ebx>
     * @param flags @<ecx << 8>
     * @param events @<edx>
     * @return
     */
    window* createWindow(
        WindowType type,
        gfx::Size size,
        uint32_t flags,
        window_event_list* events)
    {
        uint16_t width = size.width;
        uint16_t height = size.height;

        int16_t y = 30; // ax
        int16_t x = 0;  // dx
        if (window_fits_within_space(x, y, width, height))
            return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

        x = ui::width() - width;
        if (window_fits_within_space(x, y, width, height))
            return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

        x = 0;
        y = ui::height() - height - 29;
        if (window_fits_within_space(x, y, width, height))
            return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

        x = ui::width() - width;
        y = ui::height() - height - 29;
        if (window_fits_within_space(x, y, width, height))
            return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

        for (auto& w : _windows)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            x = w->x + w->width + 2;
            y = w->y;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

            x = w->x - width - 2;
            y = w->y;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

            x = w->x;
            y = w->y - height - 2;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

            x = w->x + w->width + 2;
            y = w->y + w->height - height;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

            x = w->x - width - 2;
            y = w->y + w->height - height;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);

            x = w->x + w->width - width;
            y = w->y - height - 2;
            if (window_fits_within_space(x, y, width, height))
                return loc_4C9F27(type, { x, y }, { width, height }, flags, events);
        }

        for (auto& w : _windows)
        {
            if (w->flags & window_flags::stick_to_back)
                continue;

            x = w->x + w->width + 2;
            y = w->y;
            window_fits_on_screen({ x, y }, { width, height });

            x = w->x - width - 2;
            y = w->y;
            window_fits_on_screen({ x, y }, { width, height });

            x = w->x;
            y = w->y + w->height + 2;
            window_fits_on_screen({ x, y }, { width, height });

            x = w->x;
            y = w->y - height - 2;
            window_fits_on_screen({ x, y }, { width, height });
        }

        x = 0;
        y = 30;
        for (auto& w : _windows)
        {
            if (w->x == x && w->y == y)
            {
                x += 5;
                y += 5;
                // restart loop
            }
        }

        return loc_4C9F27(type, { x, y }, { width, height }, flags, events);
    }
    /**
     * 0x004C9F5D
     *
     * @param type @<cl>
     * @param origin @<eax>
     * @param size @<ebx>
     * @param flags @<ecx << 8>
     * @param events @<edx>
     * @return
     */

    window* createWindow(
        WindowType type,
        gfx::Point origin,
        gfx::Size size,
        uint32_t flags,
        window_event_list* events)
    {
        console::log("Creating Window (%d, 0x%08X)", type, (uintptr_t)events);

        // Find right position to insert new window
        size_t dstIndex = _windows.size();
        if ((flags & window_flags::stick_to_back) != 0)
        {
            for (size_t i = 0; i < _windows.size(); i++)
            {
                if ((_windows[i]->flags & window_flags::stick_to_back) != 0)
                {
                    dstIndex = i;
                }
            }
        }
        else if ((flags & window_flags::stick_to_front) == 0)
        {
            for (auto i = (int)_windows.size(); i > 0; i--)
            {
                if ((_windows[i - 1]->flags & window_flags::stick_to_front) == 0)
                {
                    dstIndex = i;
                    break;
                }
            }
        }

        _windows.insert(_windows.begin() + dstIndex, std::make_unique<window>());
        auto window = _windows[dstIndex].get();
        console::log("Count: %d", _windows.size());

        window->type = type;
        window->var_884 = -1;
        window->var_885 = 0xFF;
        window->flags = flags;
        if ((flags & window_flags::flag_12) != 0)
        {
            window->flags |= window_flags::white_border_mask;
            audio::play_sound(audio::sound_id::open_window, 0);
        }
        else if (((flags & window_flags::stick_to_back) == 0) && ((flags & window_flags::stick_to_front) == 0) && ((flags & window_flags::flag_13) == 0))
        {
            // FIXME: Same as other case, but need a nice way to make the if clear
            window->flags |= window_flags::white_border_mask;
            audio::play_sound(audio::sound_id::open_window, 0);
        }

        window->number = 0;
        window->x = origin.x;
        window->y = origin.y;
        window->width = size.width;
        window->height = size.height;
        window->viewports[0] = nullptr;
        window->viewports[1] = nullptr;
        window->event_handlers = events;

        window->enabled_widgets = 0;
        window->disabled_widgets = 0;
        window->activated_widgets = 0;
        window->holdable_widgets = 0;

        window->var_846 = 0;
        window->var_848 = 0;
        window->var_84A = 0;
        window->var_84C = 0;
        window->var_84E = 0;
        window->var_850 = 0;
        window->var_852 = 0;
        window->var_854 = 0;
        window->var_856 = 0;
        window->var_858 = 0;
        window->current_tab = 0;
        window->frame_no = 0;

        window->invalidate();

        return window;
    }

    window* createWindowCentred(WindowType type, gfx::Size size, uint32_t flags, window_event_list* events)
    {
        auto x = (ui::width() / 2) - (size.width / 2);
        auto y = std::max(28, (ui::height() / 2) - (size.height / 2));
        return createWindow(type, gfx::Point(x, y), size, flags, events);
    }

    // 0x004C5FC8
    void drawSingle(gfx::drawpixelinfo_t* _dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom)
    {
        // Copy dpi so we can crop it
        auto dpi = *_dpi;

        // Clamp left to 0
        int32_t overflow = left - dpi.x;
        if (overflow > 0)
        {
            dpi.x += overflow;
            dpi.width -= overflow;
            if (dpi.width <= 0)
                return;
            dpi.pitch += overflow;
            dpi.bits += overflow;
        }

        // Clamp width to right
        overflow = dpi.x + dpi.width - right;
        if (overflow > 0)
        {
            dpi.width -= overflow;
            if (dpi.width <= 0)
                return;
            dpi.pitch += overflow;
        }

        // Clamp top to 0
        overflow = top - dpi.y;
        if (overflow > 0)
        {
            dpi.y += overflow;
            dpi.height -= overflow;
            if (dpi.height <= 0)
                return;
            dpi.bits += (dpi.width + dpi.pitch) * overflow;
        }

        // Clamp height to bottom
        overflow = dpi.y + dpi.height - bottom;
        if (overflow > 0)
        {
            dpi.height -= overflow;
            if (dpi.height <= 0)
                return;
        }

        if (is_unknown_4_mode() && w->type != WindowType::wt_47)
        {
            return;
        }

        loco_global<uint8_t[32], 0x9C645C> byte9C645C;

        // Company colour?
        if (w->var_884 != -1)
        {
            w->colours[0] = byte9C645C[w->var_884];
        }

        addr<0x1136F9C, int16_t>() = w->x;
        addr<0x1136F9E, int16_t>() = w->y;

        loco_global<uint8_t[4], 0x1136594> windowColours;
        // Text colouring
        windowColours[0] = colour::opaque(w->colours[0]);
        windowColours[1] = colour::opaque(w->colours[1]);
        windowColours[2] = colour::opaque(w->colours[2]);
        windowColours[3] = colour::opaque(w->colours[3]);

        w->call_prepare_draw();
        w->call_draw(&dpi);
    }

    // 0x004C6EE6
    static input::mouse_button game_get_next_input(uint32_t* x, int16_t* y)
    {
        registers regs;
        call(0x004c6ee6, regs);

        *x = regs.eax;
        *y = regs.bx;

        return (input::mouse_button)regs.cx;
    }

    // 0x004CD422
    static void process_mouse_tool(int16_t x, int16_t y)
    {
        if (!input::has_flag(input::input_flags::tool_active))
        {
            return;
        }

        auto window = find(_toolWindowType, _toolWindowNumber);
        if (window != nullptr)
        {
            window->call_tool_update(_toolWidgetIdx, x, y);
        }
        else
        {
            input::cancel_tool();
        }
    }

    static bool has_508F10(int i)
    {
        return (((uint16_t)__508F10) & (1 << i)) != 0;
    }

    bool set_508F10(int i)
    {
        bool val = (((uint16_t)__508F10) & (1 << i)) != 0;

        __508F10 = __508F10 | ~(1 << i);
        return val;
    }

    static bool reset_508F10(int i)
    {
        bool val = (((uint16_t)__508F10) & (1 << i)) != 0;

        __508F10 = __508F10 & ~(1 << i);
        return val;
    }

    // 0x004C96E7
    void handle_input()
    {
        bool set;

        if (reset_508F10(10))
        {
            call(0x00435ACC);
        }

        set = _525E28 & (1 << 2);
        *_525E28 &= ~(1 << 2);
        if (set)
        {
            if ((get_screen_flags() & 3) == 0)
            {
                if (tutorial::state() == tutorial::tutorial_state::none)
                {
                    call(0x4C95A6);
                }
            }
        }

        if (reset_508F10(5))
        {
            registers regs;
            regs.bl = 1;
            regs.dl = 2;
            regs.di = 1;
            do_game_command(21, regs);
        }

        if (has_508F10(0) && has_508F10(4))
        {
            if (reset_508F10(2))
            {
                call(0x004A0AB0);
                closeAllFloatingWindows();
                registers regs;
                regs.bl = 1;
                do_game_command(69, regs);
            }

            if (reset_508F10(3))
            {
                call(0x004A0AB0);
                closeAllFloatingWindows();
                registers regs;
                regs.bl = 1;
                do_game_command(70, regs);
            }
        }

        if (reset_508F10(4))
        {
            registers regs;
            regs.bl = 1;
            do_game_command(72, regs);
        }

        if (reset_508F10(0))
        {
            closeAllFloatingWindows();
        }

        if (reset_508F10(1))
        {
            registers regs;
            regs.bl = 1;
            regs.dl = 0;
            regs.di = 2;
            do_game_command(21, regs);
        }

        if (ui::dirty_blocks_initialised())
        {
            for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
            {
                auto& w = *it;
                w->call_8();
            }

            invalidateAllWindowsAfterInput();
            call(0x004c6e65); // update_cursor_position

            uint32_t x;
            int16_t y;
            input::mouse_button state;
            while ((state = game_get_next_input(&x, &y)) != input::mouse_button::released)
            {
                if (is_title_mode() && intro::is_active() && state == input::mouse_button::left_pressed)
                {
                    if (intro::state() == (intro::intro_state)9)
                    {
                        intro::state(intro::intro_state::end);
                        continue;
                    }
                    else
                    {
                        intro::state((intro::intro_state)8);
                    }
                }
                input::handle_mouse(x, y, state);
            }

            if (input::has_flag(input::input_flags::flag5))
            {
                input::handle_mouse(x, y, state);
            }
            else if (x != 0x80000000)
            {
                x = std::clamp<int16_t>(x, 0, ui::width() - 1);
                y = std::clamp<int16_t>(y, 0, ui::height() - 1);

                input::handle_mouse(x, y, state);
                input::process_mouse_over(x, y);
                process_mouse_tool(x, y);
            }
        }

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            w->call_9();
        }
    }

    // 0x004C98CF
    void sub_4C98CF()
    {
        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            w->call_8();
        }

        invalidateAllWindowsAfterInput();
        call(0x004c6e65); // update_cursor_position

        uint32_t x;
        int16_t y;
        input::mouse_button state;
        while ((state = game_get_next_input(&x, &y)) != input::mouse_button::released)
        {
            input::handle_mouse(x, y, state);
        }

        if (input::has_flag(input::input_flags::flag5))
        {
            input::handle_mouse(x, y, state);
        }
        else if (x != 0x80000000)
        {
            x = std::clamp<int16_t>(x, 0, ui::width() - 1);
            y = std::clamp<int16_t>(y, 0, ui::height() - 1);

            input::handle_mouse(x, y, state);
            input::process_mouse_over(x, y);
            process_mouse_tool(x, y);
        }

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            w->call_9();
        }
    }

    // 0x004CD3D0
    void dispatchUpdateAll()
    {
        _523508++;
        companymgr::updating_company_id(companymgr::get_controlling_id());

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            w->call_update();
        }

        ui::textinput::sub_4CE6FF();
        call(0x4CEEA7);
    }

    // 0x004CC6EA
    void close(window* window)
    {
        if (window == nullptr)
        {
            return;
        }

        // Make a copy of the window class and number in case
        // the window order is changed by the close event.
        auto type = window->type;
        uint16_t number = window->number;

        window->call_close();

        window = find(type, number);
        if (window == nullptr)
            return;

        if (window->viewports[0] != nullptr)
        {
            window->viewports[0]->width = 0;
            window->viewports[0] = nullptr;
        }

        if (window->viewports[1] != nullptr)
        {
            window->viewports[1]->width = 0;
            window->viewports[1] = nullptr;
        }

        window->invalidate();

        console::log("Close window");
        _windows.erase(
            std::remove_if(
                _windows.begin(),
                _windows.end(),
                [&window](std::unique_ptr<ui::window> const& w) {
                    return w.get() == window;
                }),
            _windows.end());

        viewportmgr::updatePointers();
    }

    // 0x0045F18B
    void callViewportRotateEventOnAllWindows()
    {
        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            w->call_viewport_rotate();
        }
    }

    // 0x004CD296
    void relocateWindows()
    {
        int16_t newLocation = 8;
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            // Work out if the window requires moving
            bool extendsX = (w->x + 10) >= ui::width();
            bool extendsY = (w->y + 10) >= ui::height();
            if ((w->flags & window_flags::stick_to_back) != 0 || (w->flags & window_flags::stick_to_front) != 0)
            {
                // toolbars are 27px high
                extendsY = (w->y + 10 - 27) >= ui::height();
            }

            if (extendsX || extendsY)
            {
                // Calculate the new locations
                int16_t oldX = w->x;
                int16_t oldY = w->y;
                w->x = newLocation;
                w->y = newLocation + 28;

                // Move the next new location so windows are not directly on top
                newLocation += 8;

                // Adjust the viewports if required.
                if (w->viewports[0] != nullptr)
                {
                    w->viewports[0]->x -= oldX - w->x;
                    w->viewports[0]->y -= oldY - w->y;
                }

                if (w->viewports[1] != nullptr)
                {
                    w->viewports[1]->x -= oldX - w->x;
                    w->viewports[1]->y -= oldY - w->y;
                }
            }
        }
    }

    // 0x004CEE0B
    void sub_4CEE0B(window* self)
    {
        int left = self->x;
        int right = self->x + self->width;
        int top = self->y;
        int bottom = self->y + self->height;

        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w.get() == self)
                continue;

            if (w->flags & window_flags::stick_to_back)
                continue;

            if (w->flags & window_flags::stick_to_front)
                continue;

            if (w->x >= right)
                continue;

            if (w->x + w->width <= left)
                continue;

            if (w->y >= bottom)
                continue;

            if (w->y + w->height <= top)
                continue;

            w->invalidate();

            if (bottom < ui::height() - 80)
            {
                int dY = bottom + 3 - w->y;
                w->y += dY;
                w->invalidate();

                if (w->viewports[0] != nullptr)
                {
                    w->viewports[0]->y += dY;
                }

                if (w->viewports[1] != nullptr)
                {
                    w->viewports[1]->y += dY;
                }
            }
        }
    }

    // 0x004B93A5
    void sub_4B93A5(window_number number)
    {
        FOR_ALL_WINDOWS_FROM_BACK (w)
        {
            if (w->type != WindowType::vehicle)
                continue;

            if (w->number != number)
                continue;

            if (w->current_tab != 4)
                continue;

            w->invalidate();
        }
    }

    // 0x004BF089
    void closeTopmost()
    {
        close(WindowType::dropdown, 0);

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;

            if (w->flags & window_flags::stick_to_back)
                continue;

            if (w->flags & window_flags::stick_to_front)
                continue;

            close(w.get());
            break;
        }
    }

    static void windowScrollWheelInput(ui::window* window, widget_index widgetIndex, int wheel)
    {
        int scrollIndex = window->get_scroll_data_index(widgetIndex);
        scroll_area_t* scroll = &window->scroll_areas[scrollIndex];
        ui::widget_t* widget = &window->widgets[widgetIndex];

        if (window->scroll_areas[scrollIndex].flags & 0b10000)
        {
            int size = widget->bottom - widget->top - 1;
            if (scroll->flags & 0b1)
                size -= 11;
            size = std::max(0, scroll->v_bottom - size);
            scroll->v_top = std::clamp(scroll->v_top + wheel, 0, size);
        }
        else if (window->scroll_areas[scrollIndex].flags & 0b1)
        {
            int size = widget->right - widget->left - 1;
            if (scroll->flags & 0b10000)
                size -= 11;
            size = std::max(0, scroll->h_right - size);
            scroll->h_left = std::clamp(scroll->h_left + wheel, 0, size);
        }

        ui::scrollview::update_thumbs(window, widgetIndex);
        invalidateWidget(window->type, window->number, widgetIndex);
    }

    // 0x004C628E
    static bool windowWheelInput(window* window, int wheel)
    {
        int widgetIndex = -1;
        int scrollIndex = -1;
        for (widget_t* widget = window->widgets; widget->type != widget_type::end; widget++)
        {
            widgetIndex++;

            if (widget->type != widget_type::scrollview)
                continue;

            scrollIndex++;
            if (window->scroll_areas[scrollIndex].flags & 0b10001)
            {
                windowScrollWheelInput(window, widgetIndex, wheel);
                return true;
            }
        }

        return false;
    }

    // TODO: Move
    // 0x0049771C
    static void sub_49771C()
    {
        // Might have something to do with town labels
        call(0x0049771C);
    }

    // TODO: Move
    // 0x0048DDC3
    static void sub_48DDC3()
    {
        // Might have something to do with station labels
        call(0x0048DDC3);
    }

    // 0x004C6202
    void allWheelInput()
    {
        int wheel = 0;

        while (true)
        {
            _cursorWheel -= 120;

            if (_cursorWheel < 0)
            {
                _cursorWheel += 120;
                break;
            }

            wheel -= 17;
        }

        while (true)
        {
            _cursorWheel += 120;

            if (_cursorWheel > 0)
            {
                _cursorWheel -= 120;
                break;
            }

            wheel += 17;
        }

        if (tutorial::state() != tutorial::tutorial_state::none)
            return;

        if (input::has_flag(input::input_flags::flag5))
        {
            if (openloco::is_title_mode())
                return;

            auto main = WindowManager::getMainWindow();
            if (main != nullptr)
            {
                if (wheel > 0)
                {
                    main->viewport_rotate_right();
                }
                else if (wheel < 0)
                {
                    main->viewport_rotate_left();
                }
                sub_49771C();
                sub_48DDC3();
                windows::map_center_on_view_point();
            }

            return;
        }

        int32_t x = addr<0x0113E72C, int32_t>();
        int32_t y = addr<0x0113E730, int32_t>();
        auto window = findAt(x, y);

        if (window != nullptr)
        {
            if (window->type == WindowType::main)
            {
                if (openloco::is_title_mode())
                    return;

                if (wheel > 0)
                {
                    window->viewport_zoom_in(true);
                }
                else if (wheel < 0)
                {
                    window->viewport_zoom_out(true);
                }
                sub_49771C();
                sub_48DDC3();

                return;
            }
            else
            {
                auto widgetIndex = window->find_widget_at(x, y);
                if (widgetIndex != -1)
                {
                    if (window->widgets[widgetIndex].type == widget_type::scrollview)
                    {
                        auto scrollIndex = window->get_scroll_data_index(widgetIndex);
                        if (window->scroll_areas[scrollIndex].flags & 0b10001)
                        {
                            windowScrollWheelInput(window, widgetIndex, wheel);
                            return;
                        }
                    }

                    if (windowWheelInput(window, wheel))
                    {
                        return;
                    }
                }
            }
        }

        for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
        {
            auto& w = *it;
            if (windowWheelInput(w.get(), wheel))
            {
                return;
            }
        }
    }

    bool isInFront(ui::window* window)
    {
        auto it = std::find_if(_windows.begin(), _windows.end(), [&window](std::unique_ptr<ui::window> const& w) {
            return w.get() == window;
        });

        for (it++; it != _windows.end(); it++)
        {
            auto& w = *it;

            if ((w->flags & window_flags::stick_to_front) != 0)
                continue;

            return false;
        }

        return true;
    }

    bool isInFrontAlt(ui::window* window)
    {
        auto it = std::find_if(_windows.begin(), _windows.end(), [&window](std::unique_ptr<ui::window> const& w) {
            return w.get() == window;
        });

        for (it++; it != _windows.end(); it++)
        {
            auto& w = *it;

            if ((w->flags & window_flags::stick_to_front) != 0)
                continue;

            if (w->type == WindowType::buildVehicle)
                continue;

            return false;
        }

        return true;
    }

    void sub_4C6B09(window* window, viewport* viewport, int16_t x, int16_t y);

    /**
     * 0x004C6A40
     * openrct2: viewport_shift_pixels
     *
     * @param window @<edi>
     * @param viewport @<esi>
     */
    static void sub_4C6A40(ui::window* window, ui::viewport* viewport, int16_t dX, int16_t dY)
    {
        auto it = std::find_if(_windows.begin(), _windows.end(), [&window](std::unique_ptr<ui::window> const& w) {
            return w.get() == window;
        });
        for (; it != _windows.end(); it++)
        {
            auto& w = *it;

            if ((w->flags & window_flags::transparent) == 0)
                continue;

            if (viewport == w->viewports[0])
                continue;

            if (viewport == w->viewports[1])
                continue;

            if (viewport->x + viewport->width <= w->x)
                continue;

            if (w->x + w->width <= viewport->x)
                continue;

            if (viewport->y + viewport->height <= w->y)
                continue;

            if (w->y + w->height <= viewport->y)
                continue;

            int16_t left, top, right, bottom, cx;

            left = w->x;
            top = w->y;
            right = w->x + w->width;
            bottom = w->y + w->height;

            // TODO: replace these with min/max
            cx = viewport->x;
            if (left < cx)
                left = cx;

            cx = viewport->x + viewport->width;
            if (right > cx)
                right = cx;

            cx = viewport->y;
            if (top < cx)
                top = cx;

            cx = viewport->y + viewport->height;
            if (bottom > cx)
                bottom = cx;

            if (left < right && top < bottom)
            {
                gfx::redraw_screen_rect(left, top, right, bottom); // openrct2: window_draw_all
            }
        }

        sub_4C6B09(window, viewport, dX, dY);
    }

    static void copy_rect(int16_t ax, int16_t bx, int16_t cx, int16_t dx, int16_t di, int16_t si)
    {
        console::log("copy_rect(%d, %d, %d, %d, %d, %d);", ax, bx, cx, dx, di, si);
        console::log("  dpi.bits:       0x%" PRIXPTR, _screen_dpi->bits);
        console::log("  dpi.x:          %d", _screen_dpi->x);
        console::log("  dpi.y:          %d", _screen_dpi->y);
        console::log("  dpi.width:      %d", _screen_dpi->width);
        console::log("  dpi.height:     %d", _screen_dpi->height);
        console::log("  dpi.pitch:      %d", _screen_dpi->pitch);
        console::log("  dpi.zoom_level: %d", _screen_dpi->zoom_level);
        registers regs;
        regs.ax = ax;
        regs.bx = bx;
        regs.cx = cx;
        regs.dx = dx;
        regs.di = di;
        regs.si = si;
        call(0x00451DCB, regs);
    }

    /**
     * 0x004C6B09
     * rct2: viewport_redraw_after_shift
     *
     * @param edi @<edi>
     * @param x @<dx>
     * @param y @<bp>
     * @param viewport @<esi>
     */
    void sub_4C6B09(window* window, viewport* viewport, int16_t x, int16_t y)
    {
        if (window != nullptr)
        {
            // skip current window and non-intersecting windows
            if (viewport == window->viewports[0] || viewport == window->viewports[1] || viewport->x + viewport->width <= window->x || viewport->x >= window->x + window->width || viewport->y + viewport->height <= window->y || viewport->y >= window->y + window->height)
            {
                size_t nextWindowIndex = WindowManager::indexOf(window) + 1;
                auto nextWindow = nextWindowIndex >= count() ? nullptr : get(nextWindowIndex);
                sub_4C6B09(nextWindow, viewport, x, y);
                return;
            }

            // save viewport
            ui::viewport view_copy = *viewport;

            if (viewport->x < window->x)
            {
                viewport->width = window->x - viewport->x;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);

                viewport->x += viewport->width;
                viewport->view_x += viewport->width << viewport->zoom;
                viewport->width = view_copy.width - viewport->width;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);
            }
            else if (viewport->x + viewport->width > window->x + window->width)
            {
                viewport->width = window->x + window->width - viewport->x;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);

                viewport->x += viewport->width;
                viewport->view_x += viewport->width << viewport->zoom;
                viewport->width = view_copy.width - viewport->width;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);
            }
            else if (viewport->y < window->y)
            {
                viewport->height = window->y - viewport->y;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);

                viewport->y += viewport->height;
                viewport->view_y += viewport->height << viewport->zoom;
                viewport->height = view_copy.height - viewport->height;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);
            }
            else if (viewport->y + viewport->height > window->y + window->height)
            {
                viewport->height = window->y + window->height - viewport->y;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);

                viewport->y += viewport->height;
                viewport->view_y += viewport->height << viewport->zoom;
                viewport->height = view_copy.height - viewport->height;
                viewport->view_width = viewport->width << viewport->zoom;
                sub_4C6B09(window, viewport, x, y);
            }

            // restore viewport
            *viewport = view_copy;
        }
        else
        {
            int16_t left = viewport->x;
            int16_t top = viewport->y;
            int16_t right = left + viewport->width;
            int16_t bottom = top + viewport->height;

            // if moved more than the viewport size
            if (std::abs(x) >= viewport->width || std::abs(y) >= viewport->width)
            {
                // redraw whole viewport
                gfx::redraw_screen_rect(left, top, right, bottom);
            }
            else
            {
                // update whole block ?
                copy_rect(left, top, viewport->width, viewport->height, x, y);

                if (x > 0)
                {
                    // draw left
                    int16_t _right = left + x;
                    gfx::redraw_screen_rect(left, top, _right, bottom);
                    left += x;
                }
                else if (x < 0)
                {
                    // draw right
                    int16_t _left = right + x;
                    gfx::redraw_screen_rect(_left, top, right, bottom);
                    right += x;
                }

                if (y > 0)
                {
                    // draw top
                    bottom = top + y;
                    gfx::redraw_screen_rect(left, top, right, bottom);
                }
                else if (y < 0)
                {
                    // draw bottom
                    top = bottom + y;
                    gfx::redraw_screen_rect(left, top, right, bottom);
                }
            }
        }
    }

    // 0x004CF456
    void closeAllFloatingWindows()
    {
        close(WindowType::dropdown, 0);

        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto it = _windows.rbegin(); it != _windows.rend(); it++)
            {
                auto& w = *it;

                if ((w->flags & window_flags::stick_to_back) != 0)
                    continue;

                if ((w->flags & window_flags::stick_to_front) != 0)
                    continue;

                close(w.get());

                // restart loop
                changed = true;
                break;
            }
        }
    }
}
