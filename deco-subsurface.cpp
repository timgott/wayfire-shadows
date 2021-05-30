#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>

#include <linux/input-event-codes.h>

#include <wayfire/nonstd/wlroots.hpp>
#include <wayfire/compositor-surface.hpp>
#include <wayfire/output.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/core.hpp>
#include <wayfire/decorator.hpp>
#include <wayfire/view-transform.hpp>
#include <wayfire/signal-definitions.hpp>
#include "deco-subsurface.hpp"
#include "deco-layout.hpp"
#include "deco-theme.hpp"
#include "deco-shadow.hpp"

#include <wayfire/plugins/common/cairo-util.hpp>

#include <cairo.h>

class simple_decoration_surface : public wf::surface_interface_t, public wf::compositor_surface_t, public wf::decorator_frame_t_t {

    bool _mapped = true;
    
    int outer_offset_left;
    int outer_offset_top;
    int outer_offset_width;
    int outer_offset_height;

    wf::point_t shadow_offset;
    int shadow_padding_horizontal;
    int shadow_padding_vertical;

    wayfire_view view;
    wf::signal_callback_t title_set = [=] ( wf::signal_data_t *data ) {
        if ( get_signaled_view( data ) == view )
            view->damage(); // trigger re-render
    };

    void update_title( int width, int height, double scale ) {

        int target_width  = width * scale;
        int target_height = height * scale;

        if ( ( title_texture.tex.width != target_width ) ||
                ( title_texture.tex.height != target_height ) ||
                ( title_texture.current_text != view->get_title() ) ) {

            title_texture.current_text = view->get_title();
            auto surface = theme.render_text( title_texture.current_text, target_width, target_height );
            cairo_surface_upload_to_texture( surface, title_texture.tex );
            cairo_surface_destroy( surface );
        }
    }

    int width = 100, height = 100;
    int active = 1; // when views are mapped, they are usually activated

    struct {
        wf::simple_texture_t tex;
        std::string current_text = "";
    } title_texture;

    wf::windecor::decoration_theme_t theme;
    wf::windecor::decoration_layout_t layout;
    wf::windecor::decoration_shadow_t shadow;
    wf::region_t frame_region;
    wf::region_t outer_region;

  public:
    simple_decoration_surface( wayfire_view view ) :
            theme{ view->get_app_id() },
            layout{ theme, [=] ( wlr_box box ) { this->damage_surface_box( box ); } } {

        this->view = view;
        view->connect_signal( "title-changed", &title_set );
        view->connect_signal("subsurface-removed", &on_subsurface_removed);

        // make sure to hide frame if the view is fullscreen
        update_decoration_size();
    }

    virtual ~simple_decoration_surface() {

        view->disconnect_signal( "title-changed", &title_set );
    }

    /* wf::surface_interface_t implementation */
    virtual bool is_mapped() const final {

        return _mapped;
    }

    wf::point_t get_offset() final {

        return {outer_offset_left, outer_offset_top};
    }

    virtual wf::dimensions_t get_size() const final {

        return {width, height};
    }

    void render_title( const wf::framebuffer_t& fb, wf::geometry_t geometry ) {

        update_title( geometry.width, geometry.height, fb.scale );
        OpenGL::render_texture( title_texture.tex.tex, fb, geometry,
            glm::vec4( 1.0f ), OpenGL::TEXTURE_TRANSFORM_INVERT_Y );
    }

    void render_scissor_box( const wf::framebuffer_t& fb, wf::point_t origin, const wlr_box& scissor ) {

        /* Draw shadow */
        shadow.render(fb, origin, scissor);

        /* Clear background */
        wf::point_t frame_origin = origin + shadow_offset;
        wf::geometry_t frame_geometry = {
            frame_origin.x, frame_origin.y, 
            width - shadow_padding_horizontal, height - shadow_padding_vertical
        };
        theme.render_background( fb, frame_geometry, scissor, active );

        /* Draw title & buttons */
        auto renderables = layout.get_renderable_areas();
        for ( auto item : renderables ) {
            if ( item->get_type() == wf::windecor::DECORATION_AREA_TITLE ) {
                OpenGL::render_begin( fb );
                fb.logic_scissor( scissor );
                render_title( fb, item->get_geometry() + frame_origin );
                OpenGL::render_end();
            }

            else { // button
                item->as_button().render( fb, item->get_geometry() + frame_origin, scissor );
            }
        }
    }

    virtual void simple_render( const wf::framebuffer_t& fb, int x, int y, const wf::region_t& damage ) override {

        wf::region_t frame = this->outer_region + wf::point_t{x, y};
        frame &= damage;

        for ( const auto& box : frame )
        {
            render_scissor_box( fb, {x, y}, wlr_box_from_pixman_box( box ) );
        }
    }

    bool accepts_input( int32_t sx, int32_t sy ) override
    {
        return pixman_region32_contains_point( frame_region.to_pixman(),
            sx, sy, NULL );
    }

    /* wf::compositor_surface_t implementation */
    virtual void on_pointer_enter( int x, int y ) override
    {
        layout.handle_motion( x + shadow_offset.x, y + shadow_offset.y );
    }

    virtual void on_pointer_leave() override
    {
        layout.handle_focus_lost();
    }

    virtual void on_pointer_motion( int x, int y ) override
    {
        layout.handle_motion( x + shadow_offset.x, y + shadow_offset.y );
    }

    virtual void on_pointer_button( uint32_t button, uint32_t state ) override
    {
        if ( button != BTN_LEFT )
        {
            return;
        }

        handle_action( layout.handle_press_event( state == WLR_BUTTON_PRESSED ) );
    }

    void handle_action( wf::windecor::decoration_layout_t::action_response_t action )
    {
        switch ( action.action )
        {
          case wf::windecor::DECORATION_ACTION_MOVE:
            return view->move_request();

          case wf::windecor::DECORATION_ACTION_RESIZE:
            return view->resize_request( action.edges );

          case wf::windecor::DECORATION_ACTION_CLOSE:
            return view->close();

          case wf::windecor::DECORATION_ACTION_TOGGLE_MAXIMIZE:
            if ( view->tiled_edges )
            {
                view->tile_request( 0 );
            } else
            {
                view->tile_request( wf::TILED_EDGES_ALL );
            }

            break;

          case wf::windecor::DECORATION_ACTION_MINIMIZE:
            view->minimize_request( true );
            break;

          default:
            break;
        }
    }

    virtual void on_touch_down( int x, int y ) override
    {
        layout.handle_motion( x + shadow_offset.x, y + shadow_offset.y );
        handle_action( layout.handle_press_event() );
    }

    virtual void on_touch_motion( int x, int y ) override {

        layout.handle_motion( x + shadow_offset.x, y + shadow_offset.y );
    }

    virtual void on_touch_up() override {

        handle_action( layout.handle_press_event( false ) );
        layout.handle_focus_lost();
    }

    /* frame implementation */
    virtual wf::geometry_t expand_wm_geometry( wf::geometry_t contained_wm_geometry ) override {

        contained_wm_geometry.x     += outer_offset_left;
        contained_wm_geometry.y     += outer_offset_top;
        contained_wm_geometry.width += outer_offset_width;
        contained_wm_geometry.height += outer_offset_height;

        return contained_wm_geometry;
    }

    virtual void calculate_resize_size( int& target_width, int& target_height ) override {

        target_width  -= outer_offset_width;
        target_height -= outer_offset_height;

        target_width  = std::max( target_width, 1 );
        target_height = std::max( target_height, 1 );
    }

    wf::signal_connection_t on_subsurface_removed = [&] (auto data) {

        auto ev = static_cast<wf::subsurface_removed_signal*>(data);
        if (ev->subsurface.get() == this) {
            unmap();
        }
    };

    void unmap() {

        _mapped = false;
        wf::emit_map_state_change( this );
    }

    virtual void notify_view_activated( bool active ) override {
        /* Stored state active/attention; new state not active */
        if ( this->active ) {
            if ( not active )
                view->damage();
        }

        /* Stored value is not active; new state is active */
        else {
            if ( active )
                view->damage();
        }

        if ( view->has_data( "view-demands-attention" ) )
            this->active = 2;

        this->active = ( active ? 1 : 0 );
    }

    virtual void notify_view_resized( wf::geometry_t view_geometry ) override {

        view->damage();
        width  = view_geometry.width;
        height = view_geometry.height;

        layout.resize( width - shadow_padding_horizontal, height - shadow_padding_vertical );
        shadow.resize( width, height );
        if ( !view->fullscreen ) {
            update_frame_region();
        }

        view->damage();
    }

    virtual void notify_view_tiled() override
    {
        update_decoration_size();
    }

    void update_decoration_size()
    {
        if ( view->fullscreen )
        {
            outer_offset_left = 0;
            outer_offset_top  = 0;
            this->frame_region.clear();
        } else
        {
            int current_thickness = theme.get_border_size();
            int current_titlebar  =
                theme.get_title_height() + theme.get_border_size();
            
            int shadow_radius = shadow.get_radius();

            int tiled_edges = view->tiled_edges;
            int shadow_top = (tiled_edges & WLR_EDGE_TOP) ? 0 : shadow_radius;
            int shadow_left = (tiled_edges & WLR_EDGE_LEFT) ? 0 : shadow_radius;
            int shadow_right = (tiled_edges & WLR_EDGE_RIGHT) ? 0 : shadow_radius;
            int shadow_bottom = (tiled_edges & WLR_EDGE_BOTTOM) ? 0 : shadow_radius;

            int margin_top = current_titlebar + current_thickness + shadow_top;
            int margin_left = current_thickness + shadow_left;
            int margin_right = current_thickness + shadow_right;
            int margin_bottom = current_thickness + shadow_bottom;

            outer_offset_left = -margin_left;
            outer_offset_top = -margin_top;
            outer_offset_width = margin_left + margin_right;
            outer_offset_height = margin_top + margin_bottom;

            shadow_padding_horizontal = shadow_left + shadow_right;
            shadow_padding_vertical = shadow_top + shadow_bottom;

            shadow_offset = {shadow_left, shadow_top};

            update_frame_region();
        }
    }

    void update_frame_region()
    {
        wf::region_t inner_region = layout.calculate_region() + shadow_offset;
        this->frame_region = inner_region;
        this->outer_region = inner_region | shadow.calculate_region();
    }

    virtual void notify_view_fullscreen() override
    {
        update_decoration_size();

        if ( !view->fullscreen )
        {
            notify_view_resized( view->get_wm_geometry() );
        }
    }
};

void init_view( wayfire_view view )
{
    auto surf = std::make_unique<simple_decoration_surface>( view );
    auto ptr  = surf.get();

    view->add_subsurface( std::move( surf ), true );
    view->set_decoration( ptr );
    view->damage();
}

void deinit_view( wayfire_view view )
{
    auto decor = dynamic_cast<simple_decoration_surface*>(
        view->get_decoration().get() );
    if ( !decor )
    {
        return;
    }

    decor->unmap();
    view->set_decoration( nullptr );
}
