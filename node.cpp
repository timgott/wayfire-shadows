#include "node.hpp"

#include <cmath>
#include <wayfire/output.hpp>

namespace winshadows {

shadow_node_t::shadow_node_t( wayfire_toplevel_view view ): wf::scene::node_t(false) {
    this->view = view;
    on_geometry_changed.set_callback([this] (auto) {
        update_geometry();
    });
    on_activated_changed.set_callback([this] (auto) {
        this->view->damage();
    });
    on_drag_focus_output.set_callback([this] (auto) {
        // drag_focus_output fires when a drag starts and whenever it crosses
        // between outputs; treat any of these as "drag in progress" if our
        // view is the one being dragged.
        bool dragging = (drag_helper->view == this->view);
        if (dragging != is_being_dragged) {
            is_being_dragged = dragging;
            update_geometry();
            this->view->damage();
        }
    });
    on_drag_done.set_callback([this] (wf::move_drag::drag_done_signal *ev) {
        if (ev->main_view == this->view && is_being_dragged) {
            is_being_dragged = false;
            update_geometry();
            this->view->damage();
        }
    });
    view->connect(&on_geometry_changed);
    view->connect(&on_activated_changed);
    drag_helper->connect(&on_drag_focus_output);
    drag_helper->connect(&on_drag_done);
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
        void render(const wf::scene::render_instruction_t& data ) override
        {
            // coordinates relative to view origin (not bounding box origin)
            wf::point_t frame_origin = self->frame_offset;
            wf::region_t paint_region = self->shadow_region + frame_origin;
            paint_region &= data.damage;

            for (const auto& box : paint_region)

            {
                self->shadow.render(data, frame_origin, wlr_box_from_pixman_box(box) , self->view->activated);
            }
            self->_was_activated = self->view->activated;
        }
    };

    instances.push_back(std::make_unique<shadow_render_instance_t>(this, push_damage, output));
}

void shadow_node_t::update_geometry() {
    wf::geometry_t frame_geometry = view->get_geometry();
    shadow.resize(frame_geometry.width, frame_geometry.height);

    // TODO: Check whether this can be done in a nicer/easier way
    wf::pointf_t view_origin_f = view->get_surface_root_node()->to_global({0, 0}); 
    wf::point_t view_origin {(int)view_origin_f.x, (int)view_origin_f.y};

    // Offset between view origin and frame top left corner
    frame_offset = wf::origin(frame_geometry) - view_origin;

    // Shadow geometry is relative to the top left corner of the frame (not the view)
    wf::geometry_t shadow_geometry = shadow.get_geometry();

    // move to view-relative coordinates
    geometry = shadow_geometry + frame_offset;

    this->shadow_region = shadow.calculate_region();

    // Clip the painted shadow to the workspace(s) the window's frame is on,
    // so an edge-tiled or maximized window's shadow does not leak past the
    // screen edge into adjacent workspaces. If the frame straddles two
    // workspaces, the clip is the union of those workspaces, leaving the
    // shadow free to extend across the workspace boundary the window itself
    // crosses.
    //
    // We deliberately leave the bounding box (geometry) unclipped: the
    // move-drag plugin captures the view's bbox at drag-start and positions
    // the dragged view as a fraction of that bbox, so changing the bbox when
    // a drag begins (which is when we'd want to unclip to follow the cursor)
    // would make the view visibly jump by the amount the clip was trimming.
    // Keeping the bbox stable avoids that, and the shadow_region clip alone
    // is enough to prevent the visual leakage the clip exists to address.
    auto output = view->get_output();
    if (output && !is_being_dragged) {
        auto og = output->get_relative_geometry();
        if (og.width > 0 && og.height > 0) {
            int x0 = (int)std::floor(1.0 * frame_geometry.x / og.width);
            int x1 = (int)std::floor(
                1.0 * (frame_geometry.x + frame_geometry.width - 1) / og.width);
            int y0 = (int)std::floor(1.0 * frame_geometry.y / og.height);
            int y1 = (int)std::floor(
                1.0 * (frame_geometry.y + frame_geometry.height - 1) / og.height);

            wf::geometry_t ws_bounds {
                x0 * og.width,
                y0 * og.height,
                (x1 - x0 + 1) * og.width,
                (y1 - y0 + 1) * og.height
            };

            wf::geometry_t ws_in_window {
                ws_bounds.x - frame_geometry.x,
                ws_bounds.y - frame_geometry.y,
                ws_bounds.width,
                ws_bounds.height
            };
            this->shadow_region &= ws_in_window;
        }
    }
}

}
