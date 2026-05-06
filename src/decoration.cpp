#include <wayfire/per-output-plugin.hpp>
#include <wayfire/view.hpp>
#include <wayfire/workarea.hpp>
#include <wayfire/matcher.hpp>
#include <wayfire/workspace-set.hpp>
#include <wayfire/output.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/render-manager.hpp>

#include "deco-subsurface.hpp"
#include "wayfire/core.hpp"
#include "wayfire/plugin.hpp"
#include "wayfire/signal-provider.hpp"
#include "wayfire/toplevel-view.hpp"
#include "wayfire/toplevel.hpp"
#include <unistd.h>
#include <dlfcn.h>
#include <wayfire/bindings-repository.hpp>
#include <wayfire/plugins/ipc/ipc-activator.hpp>

namespace wf
{
namespace vkdecor
{
class wayfire_vkdecor : public wf::plugin_interface_t
{
    wf::option_wrapper_t<int> border_size{"vkdecor/border_size"};
    wf::option_wrapper_t<std::string> title_font{"vkdecor/title_font"};
    wf::option_wrapper_t<int> title_text_align{"vkdecor/title_text_align"};
    wf::option_wrapper_t<std::string> titlebar{"vkdecor/titlebar"};
    wf::option_wrapper_t<bool> maximized_borders{"vkdecor/maximized_borders"};
    wf::option_wrapper_t<bool> maximized_shadows{"vkdecor/maximized_shadows"};
    wf::option_wrapper_t<wf::color_t> fg_color{"vkdecor/fg_color"};
    wf::option_wrapper_t<wf::color_t> bg_color{"vkdecor/bg_color"};
    wf::option_wrapper_t<wf::color_t> fg_text_color{"vkdecor/fg_text_color"};
    wf::option_wrapper_t<wf::color_t> bg_text_color{"vkdecor/bg_text_color"};
    wf::option_wrapper_t<wf::color_t> button_color{"vkdecor/button_color"};
    wf::option_wrapper_t<double> button_line_thickness{"vkdecor/button_line_thickness"};
    wf::option_wrapper_t<double> button_size{"vkdecor/button_size"};
    wf::option_wrapper_t<int> left_button_spacing{"vkdecor/left_button_spacing"};
    wf::option_wrapper_t<int> right_button_spacing{"vkdecor/right_button_spacing"};
    wf::option_wrapper_t<int> left_button_x_offset{"vkdecor/left_button_x_offset"};
    wf::option_wrapper_t<int> right_button_x_offset{"vkdecor/right_button_x_offset"};
    wf::option_wrapper_t<int> button_y_offset{"vkdecor/button_y_offset"};
    wf::option_wrapper_t<std::string> button_minimize_image{"vkdecor/button_minimize_image"};
    wf::option_wrapper_t<std::string> button_maximize_image{"vkdecor/button_maximize_image"};
    wf::option_wrapper_t<std::string> button_restore_image{"vkdecor/button_restore_image"};
    wf::option_wrapper_t<std::string> button_close_image{"vkdecor/button_close_image"};
    wf::option_wrapper_t<std::string> button_minimize_hover_image{"vkdecor/button_minimize_hover_image"};
    wf::option_wrapper_t<std::string> button_maximize_hover_image{"vkdecor/button_maximize_hover_image"};
    wf::option_wrapper_t<std::string> button_restore_hover_image{"vkdecor/button_restore_hover_image"};
    wf::option_wrapper_t<std::string> button_close_hover_image{"vkdecor/button_close_hover_image"};
    wf::option_wrapper_t<std::string> button_layout{"vkdecor/button_layout"};
    wf::option_wrapper_t<std::string> ignore_views_string{"vkdecor/ignore_views"};
    wf::option_wrapper_t<std::string> always_decorate_string{"vkdecor/always_decorate"};
    wf::option_wrapper_t<std::string> overlay_engine{"vkdecor/overlay_engine"};
    wf::option_wrapper_t<int> rounded_corner_radius{"vkdecor/rounded_corner_radius"};
    wf::option_wrapper_t<int> shadow_radius{"vkdecor/shadow_radius"};
    wf::option_wrapper_t<wf::color_t> shadow_color{"vkdecor/shadow_color"};
    wf::view_matcher_t ignore_views{"vkdecor/ignore_views"};
    wf::view_matcher_t always_decorate{"vkdecor/always_decorate"};
    wf::wl_idle_call idle_update_views;
    std::function<void(void)> update_event;
    wf::effect_hook_t pre_hook;
    bool hook_set = false;
    bool initial_state = true;


    wf::signal::connection_t<wf::txn::new_transaction_signal> on_new_tx =
        [=] (wf::txn::new_transaction_signal *ev)
    {
        // For each transaction, we need to consider what happens with participating views
        for (const auto& obj : ev->tx->get_objects())
        {
            if (auto toplevel = std::dynamic_pointer_cast<wf::toplevel_t>(obj))
            {
                // First check whether the toplevel already has decoration
                // In that case, we should just set the correct margins
                if (auto deco = toplevel->get_data<simple_decorator_t>())
                {
                    deco->update_decoration_size();
                    toplevel->pending().margins = deco->get_margins(toplevel->pending());
                    continue;
                }

                // Second case: the view is already mapped, or the transaction does not map it.
                // The view is not being decorated, so nothing to do here.
                if (toplevel->current().mapped || !toplevel->pending().mapped)
                {
                    continue;
                }

                // Third case: the transaction will map the toplevel.
                auto view = wf::find_view_for_toplevel(toplevel);
                wf::dassert(view != nullptr, "Mapping a toplevel means there must be a corresponding view!");
                if (should_decorate_view(view))
                {
                    if (auto deco = toplevel->get_data<simple_decorator_t>())
                    {
                        deco->update_decoration_size();
                    }

                    adjust_new_decorations(view);
                }
            }
        }
    };

    wf::signal::connection_t<wf::view_decoration_state_updated_signal> on_decoration_state_changed =
        [=] (wf::view_decoration_state_updated_signal *ev)
    {
        auto toplevel = wf::toplevel_cast(ev->view);
        if (toplevel)
        {
            remove_decoration(toplevel);
        }

        update_view_decoration(ev->view);
    };

    // allows criteria containing maximized or floating check
    wf::signal::connection_t<wf::view_tiled_signal> on_view_tiled =
        [=] (wf::view_tiled_signal *ev)
    {
        update_view_decoration(ev->view);
    };

    wf::signal::connection_t<wf::view_app_id_changed_signal> on_app_id_changed =
        [=] (wf::view_app_id_changed_signal *ev)
    {
        update_view_decoration(ev->view);
    };

    wf::signal::connection_t<wf::view_title_changed_signal> on_title_changed =
        [=] (wf::view_title_changed_signal *ev)
    {
        update_view_decoration(ev->view);
    };

    wf::signal::connection_t<wf::output_added_signal> on_output_added =
        [=] (wf::output_added_signal *ev)
    {

    };

    wf::signal::connection_t<wf::output_removed_signal> on_output_removed =
        [=] (wf::output_removed_signal *ev)
    {
        ev->output->render->rem_effect(&pre_hook);
    };

  public:

    void init() override
    {
        auto& ol = wf::get_core().output_layout;
        ol->connect(&on_output_added);
        ol->connect(&on_output_removed);
        wf::get_core().connect(&on_decoration_state_changed);
        wf::get_core().tx_manager->connect(&on_new_tx);
        wf::get_core().connect(&on_app_id_changed);
        wf::get_core().connect(&on_title_changed);
        wf::get_core().connect(&on_view_tiled);

        for (auto& view : wf::get_core().get_all_views())
        {
            update_view_decoration(view);
        }

        initial_state = false;

        border_size.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
                {
                    continue;
                }

                remove_decoration(toplevel);
                adjust_new_decorations(toplevel);
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        });

        fg_color.set_callback([=] { update_colors(); });
        bg_color.set_callback([=] { update_colors(); });
        fg_text_color.set_callback([=] { update_colors(); });
        bg_text_color.set_callback([=] { update_colors(); });
        ignore_views_string.set_callback([=]
        {
            idle_update_views.run_once([=] ()
            {
                for (auto& view : wf::get_core().get_all_views())
                {
                    auto toplevel = wf::toplevel_cast(view);
                    if (!toplevel)
                    {
                        continue;
                    }

                    update_view_decoration(view);
                }
            });
        });
        always_decorate_string.set_callback([=]
        {
            idle_update_views.run_once([=] ()
            {
                for (auto& view : wf::get_core().get_all_views())
                {
                    auto toplevel = wf::toplevel_cast(view);
                    if (!toplevel)
                    {
                        continue;
                    }

                    update_view_decoration(view);
                }
            });
        });

        pre_hook = [=] ()
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
                {
                    continue;
                }

            }
        };


        titlebar.set_callback([=] {recreate_frames();});
        overlay_engine.set_callback([=] {option_changed_cb(true, false);recreate_frames();});
        button_color.set_callback([=] {recreate_frames();});
        button_line_thickness.set_callback([=] {recreate_frames();});
        button_size.set_callback([=] {recreate_frames();});
        left_button_spacing.set_callback([=] {recreate_frames();});
        right_button_spacing.set_callback([=] {recreate_frames();});
        left_button_x_offset.set_callback([=] {recreate_frames();});
        right_button_x_offset.set_callback([=] {recreate_frames();});
        button_y_offset.set_callback([=] {recreate_frames();});
        button_minimize_image.set_callback([=] {recreate_frames();});
        button_maximize_image.set_callback([=] {recreate_frames();});
        button_restore_image.set_callback([=] {recreate_frames();});
        button_close_image.set_callback([=] {recreate_frames();});
        button_minimize_hover_image.set_callback([=] {recreate_frames();});
        button_maximize_hover_image.set_callback([=] {recreate_frames();});
        button_restore_hover_image.set_callback([=] {recreate_frames();});
        button_close_hover_image.set_callback([=] {recreate_frames();});
        button_layout.set_callback([=] {recreate_frames();});
        title_text_align.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                view->damage();
            }
        });
        title_font.set_callback([=] {recreate_frames();});
        shadow_radius.set_callback([=]
        {
            option_changed_cb(false, (std::string(overlay_engine) == "rounded_corners"));
        });
        shadow_color.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
                {
                    continue;
                }

                view->damage();
            }
        });
        rounded_corner_radius.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
                {
                    continue;
                }

                view->damage();
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        });
        maximized_borders.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>() ||
                    !toplevel->pending_tiled_edges())
                {
                    continue;
                }

                view->damage();
                remove_decoration(toplevel);
                adjust_new_decorations(toplevel);
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        });
        maximized_shadows.set_callback([=]
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>() ||
                    !toplevel->pending_tiled_edges())
                {
                    continue;
                }

                view->damage();
                remove_decoration(toplevel);
                adjust_new_decorations(toplevel);
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        });

        update_event = [=] (void)
        {
            update_colors();
        };

        dlopen("libpangocairo-1.0.so", RTLD_LAZY);
    }

    void fini() override
    {
        for (auto view : wf::get_core().get_all_views())
        {
            if (auto toplevel = wf::toplevel_cast(view))
            {
                remove_decoration(toplevel);
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        }

        if (hook_set)
        {
            for (auto& o : wf::get_core().output_layout->get_outputs())
            {
                o->render->rem_effect(&pre_hook);
            }
        }

        on_output_added.disconnect();
        on_output_removed.disconnect();
        on_decoration_state_changed.disconnect();
        on_new_tx.disconnect();
        on_app_id_changed.disconnect();
        on_title_changed.disconnect();

    }

    void recreate_frame(wayfire_toplevel_view toplevel)
    {
        auto deco = toplevel->toplevel()->get_data<simple_decorator_t>();
        if (!deco)
        {
            return;
        }

        deco->recreate_frame();
    }

    void recreate_frames()
    {
        for (auto& view : wf::get_core().get_all_views())
        {
            auto toplevel = wf::toplevel_cast(view);
            if (!toplevel)
            {
                continue;
            }

            recreate_frame(toplevel);
        }
    }

    void option_changed_cb(bool resize_decorations, bool recreate_decorations)
    {

            if (hook_set)
            {
                for (auto& o : wf::get_core().output_layout->get_outputs())
                {
                    o->render->rem_effect(&pre_hook);
                }

                hook_set = false;
            }


        if (recreate_decorations)
        {
            for (auto& view : wf::get_core().get_all_views())
            {
                auto toplevel = wf::toplevel_cast(view);
                if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
                {
                    continue;
                }

                remove_decoration(toplevel);
                adjust_new_decorations(toplevel);
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }

            return;
        }

        for (auto& view : wf::get_core().get_all_views())
        {
            auto toplevel = wf::toplevel_cast(view);
            if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
            {
                continue;
            }

            view->damage();
            toplevel->toplevel()->get_data<simple_decorator_t>()->effect_updated();

            auto& pending = toplevel->toplevel()->pending();
            if (!resize_decorations || (pending.tiled_edges != 0))
            {
                wf::vkdecor::schedule_transaction(toplevel->toplevel());
                continue;
            }

            if (std::string(overlay_engine) == "rounded_corners")
            {
                pending.margins =
                {int(shadow_radius) * 2, int(shadow_radius) * 2,
                    int(shadow_radius) * 2, int(shadow_radius) * 2};
                pending.geometry = wf::expand_geometry_by_margins(pending.geometry, pending.margins);
            } else
            {
                pending.geometry = wf::shrink_geometry_by_margins(pending.geometry, pending.margins);
                pending.margins  = toplevel->toplevel()->get_data<simple_decorator_t>()->get_margins(
                    toplevel->toplevel()->pending());
                pending.geometry = wf::expand_geometry_by_margins(pending.geometry, pending.margins);
            }

            wf::vkdecor::schedule_transaction(toplevel->toplevel());
        }
    }

    void update_colors()
    {
        for (auto& view : wf::get_core().get_all_views())
        {
            auto toplevel = wf::toplevel_cast(view);
            if (!toplevel || !toplevel->toplevel()->get_data<simple_decorator_t>())
            {
                continue;
            }

            auto deco = toplevel->toplevel()->get_data<simple_decorator_t>();
            deco->update_colors();
            view->damage();
        }
    }

    /**
     * Uses view_matcher_t to match whether the given view needs to be
     * ignored for decoration
     *
     * @param view The view to match
     * @return Whether the given view should be decorated?
     */
    bool ignore_decoration_of_view(wayfire_view view)
    {
        return ignore_views.matches(view);
    }

    bool should_decorate_view(wayfire_toplevel_view view)
    {
        return (view->should_be_decorated() && !ignore_decoration_of_view(view)) || always_decorate.matches(
            view);
    }

    void adjust_new_decorations(wayfire_toplevel_view view)
    {
        auto toplevel = view->toplevel();

        bool was_decorated = false;
        if (!toplevel->get_data<simple_decorator_t>())
        {
            toplevel->store_data(std::make_unique<simple_decorator_t>(view));
        } else
        {
            was_decorated = true;
        }

        auto deco     = toplevel->get_data<simple_decorator_t>();
        auto& pending = toplevel->pending();
        pending.margins = deco->get_margins(pending);

        if (!pending.fullscreen && !pending.tiled_edges && (was_decorated || initial_state))
        {
            pending.geometry = wf::expand_geometry_by_margins(pending.geometry, pending.margins);
        }
    }

    void remove_decoration(wayfire_toplevel_view view)
    {
        view->toplevel()->erase_data<simple_decorator_t>();
        auto& pending = view->toplevel()->pending();
        if (!pending.fullscreen && !pending.tiled_edges)
        {
            pending.geometry = wf::shrink_geometry_by_margins(pending.geometry, pending.margins);
        }

        pending.margins = {0, 0, 0, 0};

        std::string custom_data_name = "wf-decoration-shadow-margin";
        if (view->has_data(custom_data_name))
        {
            view->erase_data(custom_data_name);
        }
    }

    bool is_toplevel_decorated(const std::shared_ptr<wf::toplevel_t>& toplevel)
    {
        return toplevel->has_data<wf::vkdecor::simple_decorator_t>();
    }

    void update_view_decoration(wayfire_view view)
    {
        if (auto toplevel = wf::toplevel_cast(view))
        {
            const bool wants_decoration = should_decorate_view(toplevel);
            if (wants_decoration != is_toplevel_decorated(toplevel->toplevel()))
            {
                if (wants_decoration)
                {
                    adjust_new_decorations(toplevel);
                } else
                {
                    remove_decoration(toplevel);
                }

                wf::vkdecor::schedule_transaction(toplevel->toplevel());
            }
        }
    }
};
}
}

DECLARE_WAYFIRE_PLUGIN(wf::vkdecor::wayfire_vkdecor);
