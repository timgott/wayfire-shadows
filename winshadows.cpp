#include <wayfire/matcher.hpp>
#include <wayfire/object.hpp>
#include <wayfire/output.hpp>
#include <wayfire/per-output-plugin.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/scene-operations.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/signal-provider.hpp>
#include <wayfire/view.hpp>
#include <wayfire/workspace-set.hpp>

#include "node.hpp"

struct view_shadow_data : wf::custom_data_t {
    view_shadow_data(std::shared_ptr<winshadows::shadow_node_t> shadow_ptr) : shadow_ptr(shadow_ptr) {};

    std::shared_ptr<winshadows::shadow_node_t> shadow_ptr;
};

namespace wayfire_shadows_globals {
    // Global because focus has to be tracked across outputs, but there is an instance of the plugin per output
    wayfire_view last_focused_view = nullptr;
}

class wayfire_shadows : public wf::per_output_plugin_instance_t {
    const std::string surface_data_name = "shadow_surface";

    wf::view_matcher_t enabled_views{"winshadows/enabled_views"};
    wf::option_wrapper_t<bool> include_undecorated_views{"winshadows/include_undecorated_views"};

    wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped =
        [=](auto *data) { update_view_decoration(data->view); };
    wf::signal::connection_t<wf::view_decoration_state_updated_signal> on_view_updated =
        [=](auto *data) { update_view_decoration(data->view); };
    wf::signal::connection_t<wf::view_tiled_signal> on_view_tiled =
        [=](auto *data) { update_view_decoration(data->view); };

    wf::signal::connection_t<wf::focus_view_signal> on_focus_changed =
        [=](wf::focus_view_signal *data) {
            wayfire_view focused_view = data->view;
            wayfire_view last_focused = wayfire_shadows_globals::last_focused_view;
            if (last_focused != nullptr) {
                update_view_decoration(last_focused);
            }
            if (focused_view != nullptr) {
                update_view_decoration(focused_view);
            }
            wayfire_shadows_globals::last_focused_view = focused_view;
        };

    wf::signal::connection_t<wf::view_unmapped_signal> on_view_unmapped =
        [=](auto *data) {
            wayfire_view view = data->view;
            if (view == wayfire_shadows_globals::last_focused_view) {
                wayfire_shadows_globals::last_focused_view = nullptr;
            }
        };

public:
    void init() override {
        output->connect(&on_view_mapped);
        output->connect(&on_view_updated);
        output->connect(&on_view_tiled);
        output->connect(&on_focus_changed);
        output->connect(&on_view_unmapped);

        for (auto &view : output->wset()->get_views()) {
            update_view_decoration(view);
        }
    }

    /**
     * Checks whether the given view has server side decoration and is in
     * the white list.
     *
     * @param view The view to match
     * @return Whether the view should get a shadow.
     */
    bool is_view_shadow_enabled(wayfire_view view) {
        return enabled_views.matches(view) && (is_view_decorated(view) || include_undecorated_views);
    }

    bool is_view_decorated(wayfire_view view) {
        return view->should_be_decorated();
    }

    const wf::scene::floating_inner_ptr& get_shadow_root_node(wayfire_view view) const {
        return view->get_surface_root_node();
    }

    wf::wl_idle_call idle_deactivate;
    void update_view_decoration(wayfire_view view) {
        if (is_view_shadow_enabled(view)) {
            auto shadow_data = view->get_data<view_shadow_data>(surface_data_name);
            if (!shadow_data) {
                // No shadow yet, create it now.
                init_view(view);
            } else {
                // in some situations the shadow node might have been removed due to unmap, but the view is reused (including the custom data)
                auto shadow_root = get_shadow_root_node(view);
                if (shadow_data->shadow_ptr->parent() != shadow_root.get()) {
                        wf::scene::add_back(shadow_root, shadow_data->shadow_ptr);
                }
                // Shadow already exists, redraw if necessary,
                // e.g. view was focused and glow is enabled.
                if (shadow_data->shadow_ptr->needs_redraw()) {
                    view->damage();
                }
            }
        } else {
            deinit_view(view);
        }
    }

    bool is_view_initialized(wayfire_view view) {
        return view->has_data(surface_data_name);
    }

    void init_view(wayfire_view view) {
        // create the shadow node and add it to the view
        auto node = std::make_shared<winshadows::shadow_node_t>(view);
        wf::scene::add_back(get_shadow_root_node(view), node);

        // store the shadow node in the view so we can remove it later
        auto view_data = std::make_unique<view_shadow_data>(node);
        view->store_data(std::move(view_data), surface_data_name);

        view->damage();
    }

    void deinit_view(wayfire_view view) {
        auto view_data = view->get_data<view_shadow_data>(surface_data_name);
        if (view_data != nullptr) {
            wf::scene::remove_child(view_data->shadow_ptr);
            view->damage();
            view->erase_data(surface_data_name);
        }
    }

    void fini() override {
        output->disconnect(&on_view_mapped);
        output->disconnect(&on_view_updated);
        output->disconnect(&on_view_tiled);
        output->disconnect(&on_focus_changed);
        output->disconnect(&on_view_unmapped);

        for (auto &view : output->wset()->get_views()) {
            deinit_view(view);
        }
    }
};

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<wayfire_shadows>);
