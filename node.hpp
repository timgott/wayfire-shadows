#pragma once

#include <wayfire/geometry.hpp>
#include <wayfire/scene-render.hpp>
#include <wayfire/core.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/toplevel-view.hpp>
#include "renderer.hpp"

namespace winshadows {

class shadow_node_t : public wf::scene::node_t {
  private:
    int _was_activated = 1; // used to check whether redrawing on focus is necessary

    // geometry of the node relative to the view origin
    wf::geometry_t geometry;

    // offset between the node origin and the frame origin (i.e. top-left borders)
    wf::point_t frame_offset;

    wayfire_toplevel_view view;

    int width = 100, height = 100;
    wf::region_t shadow_region;
    shadow_renderer_t shadow;

    wf::signal::connection_t<wf::view_geometry_changed_signal> on_geometry_changed;
    wf::signal::connection_t<wf::view_activated_state_signal> on_activated_changed;

    void update_geometry();

  public:
    shadow_node_t(wayfire_toplevel_view view);

    virtual ~shadow_node_t();

    void gen_render_instances(std::vector<wf::scene::render_instance_uptr> &instances, wf::scene::damage_callback push_damage, wf::output_t *output = nullptr) override;

    wf::geometry_t get_bounding_box() override;

};

}

