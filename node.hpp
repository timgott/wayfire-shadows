#pragma once

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <linux/input-event-codes.h>

#include <wayfire/scene.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/core.hpp>
#include <wayfire/signal-definitions.hpp>
#include "renderer.hpp"

namespace winshadows {

class shadow_node_t : public wf::scene::node_t {
  private:
    int _was_activated = 1; // used to check whether redrawing on focus is necessary

    wf::dimensions_t surface_dimensions;
    wf::point_t surface_offset_to_view;
    wf::point_t surface_offset_to_frame;

    wayfire_view view;

    int width = 100, height = 100;
    wf::region_t shadow_region;
    shadow_renderer_t shadow;

    wf::signal_connection_t on_geometry_changed = [&] (auto) {
        update_geometry();
    };

    void update_geometry();

  public:
    shadow_node_t(wayfire_view view);

    virtual ~shadow_node_t();

    void gen_render_instances(std::vector<wf::scene::render_instance_uptr> &instances, wf::scene::damage_callback push_damage, wf::output_t *output = nullptr) override;

    wf::geometry_t get_bounding_box() override;

    bool needs_redraw();

};

}

