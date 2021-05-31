#pragma once

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <linux/input-event-codes.h>

#include <wayfire/view.hpp>
#include <wayfire/output.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/core.hpp>
#include <wayfire/signal-definitions.hpp>
#include "deco-shadow.hpp"

#include <wayfire/plugins/common/cairo-util.hpp>

#include <cairo.h>

class shadow_decoration_surface : public wf::surface_interface_t {

    bool _mapped = true;
    
    wf::geometry_t surface_geometry;

    wayfire_view view;

    int width = 100, height = 100;
    int active = 1; // when views are mapped, they are usually activated
    wf::windecor::decoration_shadow_t shadow;
    wf::region_t shadow_region;

  public:
    shadow_decoration_surface( wayfire_view view ) {

        this->view = view;
        view->connect_signal("subsurface-removed", &on_subsurface_removed);
        view->connect_signal("geometry-changed", &on_geometry_changed);

        // make sure to hide frame if the view is fullscreen
        update_geometry();
    }

    virtual ~shadow_decoration_surface() {
        view->disconnect_signal(&on_subsurface_removed);
        view->disconnect_signal(&on_geometry_changed);
    }

    wf::signal_connection_t on_subsurface_removed = [&] (auto data) {

        auto ev = static_cast<wf::subsurface_removed_signal*>(data);
        if (ev->subsurface.get() == this) {
            unmap();
        }
    };

    wf::signal_connection_t on_geometry_changed = [&] (auto) {
        update_geometry();
    };

    virtual bool is_mapped() const final;

    wf::point_t get_offset() final;

    virtual wf::dimensions_t get_size() const final;

    virtual void simple_render( const wf::framebuffer_t& fb, int x, int y, const wf::region_t& damage ) override;

    bool accepts_input( int32_t sx, int32_t sy ) override;

    void unmap();

    void update_geometry();
};
