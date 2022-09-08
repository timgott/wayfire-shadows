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

namespace wf::winshadows {
class shadow_decoration_surface : public wf::surface_interface_t {

    bool _mapped = true;
    int _was_activated = 1; // used to check whether redrawing on focus is necessary

    wf::geometry_t surface_geometry;

    wayfire_view view;

    int width = 100, height = 100;
    wf::winshadows::decoration_shadow_t shadow;
    wf::region_t shadow_region;

    wf::signal_connection_t on_subsurface_removed = [&] (auto data) {

        auto ev = static_cast<wf::subsurface_removed_signal*>(data);
        if (ev->subsurface.get() == this) {
            unmap();
        }
    };

    wf::signal_connection_t on_geometry_changed = [&] (auto) {
        update_geometry();
    };

  public:
    shadow_decoration_surface( wayfire_view view );

    virtual ~shadow_decoration_surface();


    virtual bool is_mapped() const final;

    wf::point_t get_offset() final;

    virtual wf::dimensions_t get_size() const final;

    virtual void simple_render( const wf::framebuffer_t& fb, int x, int y, const wf::region_t& damage ) override;

    bool accepts_input( int32_t sx, int32_t sy ) override;

    void unmap();

    void update_geometry();

    bool needs_redraw();
};

}

