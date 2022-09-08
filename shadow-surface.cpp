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

    return wf::origin(surface_geometry);
}

wf::dimensions_t shadow_decoration_surface::get_size() const {

    return wf::dimensions(surface_geometry);
}

void shadow_decoration_surface::simple_render( const wf::framebuffer_t& fb, int, int, const wf::region_t& damage ) {
    wf::point_t window_origin = wf::origin(view->get_wm_geometry());
    wf::region_t frame = this->shadow_region + window_origin;
    frame &= damage;

    for (const auto& box : frame)
    {
        shadow.render(fb, window_origin, wlr_box_from_pixman_box(box), view->activated);
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
    wf::geometry_t view_geometry = view->get_wm_geometry();
    shadow.resize(view_geometry.width, view_geometry.height);

    wf::point_t frame_offset = wf::origin(view->get_wm_geometry()) - wf::origin(view->get_output_geometry());

    surface_geometry = shadow.get_geometry() + frame_offset;
    this->shadow_region = shadow.calculate_region();
}

void shadow_decoration_surface::unmap() {

    _mapped = false;
    wf::emit_map_state_change( this );
}

}
