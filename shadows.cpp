#include <wayfire/object.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/view.hpp>
#include <wayfire/matcher.hpp>
#include <wayfire/workspace-manager.hpp>
#include <wayfire/output.hpp>
#include <wayfire/signal-definitions.hpp>

#include "shadow-surface.hpp"

struct view_shadow_data : wf::custom_data_t {
    view_shadow_data(nonstd::observer_ptr<shadow_decoration_surface> shadow_ptr) : shadow_ptr(shadow_ptr) {};

    nonstd::observer_ptr<shadow_decoration_surface> shadow_ptr;
};

class wayfire_shadows : public wf::plugin_interface_t
{
    const std::string surface_data_name = "shadow_surface";

    wf::view_matcher_t ignore_views{"winshadows/ignore_views"};

    wf::signal_connection_t view_updated{
        [=] (wf::signal_data_t *data)
        {
            update_view_decoration(get_signaled_view(data));
        }
    };

  public:
    void init() override
    {
        grab_interface->name = "window-shadows";
        grab_interface->capabilities = 0;

        output->connect_signal("view-mapped", &view_updated);
        output->connect_signal("view-decoration-state-updated", &view_updated);
        for (auto& view :
             output->workspace->get_views_in_layer(wf::ALL_LAYERS))
        {
            update_view_decoration(view);
        }
    }

    /**
     * Uses view_matcher_t to match whether the given view needs to be
     * ignored for decoration
     *
     * @param view The view to match
     * @return Whether the given view should be ignored?
     */
    bool is_view_ignored(wayfire_view view)
    {
        return ignore_views.matches(view);
    }

    wf::wl_idle_call idle_deactivate;
    void update_view_decoration(wayfire_view view)
    {
        if (!is_view_ignored(view))
        {
            if (!is_view_initialized(view)) {
                if (output->activate_plugin(grab_interface))
                {
                    init_view(view);
                    idle_deactivate.run_once([this] ()
                    {
                        output->deactivate_plugin(grab_interface);
                    });
                }
            }
        } else
        {
            deinit_view(view);
        }
    }

    bool is_view_initialized( wayfire_view view ) {
        return view->has_data(surface_data_name);
    }

    void init_view( wayfire_view view )
    {
        auto surf = std::make_unique<shadow_decoration_surface>( view );
        
        auto view_data = std::make_unique<view_shadow_data>(surf.get());

        view->store_data( 
            std::move(view_data), 
            surface_data_name
        );

        view->add_subsurface(std::move( surf ), true );
        view->damage();
    }

    void deinit_view( wayfire_view view )
    {
        auto view_data = view->get_data<view_shadow_data>(surface_data_name);
        if (view_data != nullptr) {
            view->remove_subsurface(view_data->shadow_ptr);
            view->erase_data(surface_data_name);
            view->damage();
        }
    }

    void fini() override
    {
        output->disconnect_signal(&view_updated);

        for (auto& view : output->workspace->get_views_in_layer(wf::ALL_LAYERS))
        {
            deinit_view(view);
        }
    }

    // std::string icon_for_app
};

DECLARE_WAYFIRE_PLUGIN(wayfire_shadows);
