#include "shadow-surface.hpp"

namespace wf::winshadows {

shadow_decoration_surface::shadow_decoration_surface( wayfire_view view ) {
    this->view = view;
    view->connect_signal("subsurface-removed", &on_subsurface_removed);
    view->connect_signal("geometry-changed", &on_geometry_changed);

    update_geometry();
}

shadow_decoration_surface::~shadow_decoration_surface() {
    view->disconnect_signal(&on_subsurface_removed);
    view->disconnect_signal(&on_geometry_changed);
}

/* wf::surface_interface_t implementation */
bool shadow_decoration_surface::is_mapped() const {
    return _mapped;
}

wf::point_t shadow_decoration_surface::get_offset() {
    return surface_offset_to_view;
}

wf::dimensions_t shadow_decoration_surface::get_size() const {

    return surface_dimensions;
}

void shadow_decoration_surface::simple_render( const wf::render_target_t& fb, int x, int y, const wf::region_t& damage ) {
    wf::point_t frame_origin = wf::point_t{x, y} - surface_offset_to_frame;
    wf::region_t paint_region = this->shadow_region + frame_origin;
    paint_region &= damage;

    for (const auto& box : paint_region)
    {
        shadow.render(fb, frame_origin, wlr_box_from_pixman_box(box), view->activated);
    }
    _was_activated = view->activated;
}

bool shadow_decoration_surface::needs_redraw() {
    if (shadow.is_glow_enabled()) {
        return view->activated != _was_activated;
    }
    return false;
}

bool shadow_decoration_surface::accepts_input( int32_t, int32_t )
{
    return false;
}

void shadow_decoration_surface::update_geometry() {
    wf::geometry_t frame_geometry = view->get_wm_geometry();
    shadow.resize(frame_geometry.width, frame_geometry.height);

    // Offset between view origin and frame top left corner
    wf::point_t frame_offset = wf::origin(frame_geometry) - wf::origin(view->get_output_geometry());

    // compute size and offsets
    wf::geometry_t shadow_geometry = shadow.get_geometry();
    surface_dimensions = wf::dimensions(shadow_geometry);
    surface_offset_to_frame = wf::origin(shadow_geometry);
    surface_offset_to_view = surface_offset_to_frame + frame_offset;

    this->shadow_region = shadow.calculate_region();
}

void shadow_decoration_surface::unmap() {

    _mapped = false;
    wf::emit_map_state_change( this );
}

}
