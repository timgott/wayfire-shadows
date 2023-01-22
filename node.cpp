#include "node.hpp"

namespace winshadows {

shadow_node_t::shadow_node_t( wayfire_view view ): wf::scene::node_t(false) {
    this->view = view;
    on_geometry_changed.set_callback([this] (auto) {
        update_geometry();
    });
    view->connect(&on_geometry_changed);
    update_geometry();
}

shadow_node_t::~shadow_node_t() {
    view->disconnect(&on_geometry_changed);
}

wf::geometry_t shadow_node_t::get_bounding_box()  {
    return geometry;
}

void shadow_node_t::gen_render_instances(std::vector<wf::scene::render_instance_uptr> &instances, wf::scene::damage_callback push_damage, wf::output_t *output) {
    // define renderer
    class shadow_render_instance_t : public wf::scene::simple_render_instance_t<shadow_node_t> {
      public:
        using simple_render_instance_t::simple_render_instance_t;
        void render(const wf::render_target_t& target, const wf::region_t& region) override
        {
            // coordinates relative to view origin (not bounding box origin)
            wf::point_t frame_origin = self->frame_offset;
            wf::region_t paint_region = self->shadow_region + frame_origin;
            paint_region &= region;

            for (const auto& box : paint_region)
            {
                self->shadow.render(target, frame_origin, wlr_box_from_pixman_box(box), self->view->activated);
            }
            self->_was_activated = self->view->activated;
        }
    };

    instances.push_back(std::make_unique<shadow_render_instance_t>(this, push_damage, output));
}

bool shadow_node_t::needs_redraw() {
    if (shadow.is_glow_enabled()) {
        return view->activated != _was_activated;
    }
    return false;
}

void shadow_node_t::update_geometry() {
    wf::geometry_t frame_geometry = view->get_wm_geometry();
    shadow.resize(frame_geometry.width, frame_geometry.height);

    // Offset between view origin and frame top left corner
    frame_offset = wf::origin(frame_geometry) - wf::origin(view->get_output_geometry());

    // Shadow geometry is relative to the top left corner of the frame (not the view)
    wf::geometry_t shadow_geometry = shadow.get_geometry();

    // move to view-relative coordinates
    geometry = shadow_geometry + frame_offset;

    this->shadow_region = shadow.calculate_region();
}

}
