#include <wayfire/core.hpp>
#include <wayfire/matcher.hpp>
#include <wayfire/object.hpp>
#include <wayfire/output.hpp>
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

class wayfire_shadows : public wf::plugin_interface_t {
    const std::string surface_data_name = "shadow_surface";

    wf::view_matcher_t enabled_views{"winshadows/enabled_views"};
    wf::option_wrapper_t<bool> include_undecorated_views{"winshadows/include_undecorated_views"};

    // update new views
    wf::signal::connection_t<wf::view_mapped_signal> on_view_mapped =
        [=](auto *data) { update_view_decoration(data->view); };
    // update when view enables or disables server side decoration
    wf::signal::connection_t<wf::view_decoration_state_updated_signal> on_view_updated =
        [=](auto *data) { update_view_decoration(data->view); };
    // update on tile state change, such that it is possible to exclude tiled windows
    wf::signal::connection_t<wf::view_tiled_signal> on_view_tiled =
        [=](auto *data) { update_view_decoration(data->view); };

public:
    void init() override {
        wf::get_core().connect(&on_view_mapped);
        wf::get_core().connect(&on_view_updated);
        wf::get_core().connect(&on_view_tiled);

        for (auto &view : wf::get_core().get_all_views()) {
            update_view_decoration(view);
        }
    }

    void fini() override {
        wf::get_core().disconnect(&on_view_mapped);
        wf::get_core().disconnect(&on_view_updated);
        wf::get_core().disconnect(&on_view_tiled);

        for (auto &view : wf::get_core().get_all_views()) {
            deinit_view(view);
        }
    }

    /**
     * Checks whether the given view has server side decoration and is in
     * the white list.
     *
     * @param view The view to match
     * @return Whether the view should get a shadow.
     */
    bool is_view_shadow_enabled(wayfire_toplevel_view view) {
        return enabled_views.matches(view) && (is_view_decorated(view) || include_undecorated_views);
    }

    bool is_view_decorated(wayfire_toplevel_view view) {
        return view->should_be_decorated();
    }

    const wf::scene::floating_inner_ptr& get_shadow_root_node(wayfire_view view) const {
        return view->get_surface_root_node();
    }

    wf::wl_idle_call idle_deactivate;
    void update_view_decoration(wayfire_view view) {
        auto toplevel = wf::toplevel_cast(view);
        if (toplevel) {
            if (is_view_shadow_enabled(toplevel)) {
                auto shadow_data = view->get_data<view_shadow_data>(surface_data_name);
                if (!shadow_data) {
                    // No shadow yet, create it now.
                    init_view(toplevel);
                } else {
                    // in some situations the shadow node might have been removed due to unmap, but the view is reused (including the custom data)
                    auto shadow_root = get_shadow_root_node(view);
                    if (shadow_data->shadow_ptr->parent() != shadow_root.get()) {
                        wf::scene::add_back(shadow_root, shadow_data->shadow_ptr);
                    }
                }
            } else {
                deinit_view(view);
            }
        }
    }

    bool is_view_initialized(wayfire_view view) {
        return view->has_data(surface_data_name);
    }

    void init_view(wayfire_toplevel_view view) {
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
};

DECLARE_WAYFIRE_PLUGIN(wayfire_shadows);
