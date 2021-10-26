#pragma once
#include <wayfire/option-wrapper.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/region.hpp>

namespace wf {
namespace windecor {
/**
 * A  class that can render shadows.
 * It manages the shader and calculates the necessary padding.
 */
class decoration_shadow_t {
    public:
        decoration_shadow_t();
        void render(const framebuffer_t& fb, wf::point_t origin, const geometry_t& scissor);
        void resize(const int width, const int height);
        
        wf::region_t calculate_region() const;

        int get_radius() const;
        wf::geometry_t get_geometry() const;
    
    private:
        OpenGL::program_t program;
        wf::geometry_t geometry;
        wf::geometry_t inner_geometry;
        wlr_box calculate_padding(const wf::geometry_t window_geometry) const;

        wf::option_wrapper_t<wf::color_t> shadow_color { "winshadows/shadow_color" };
        wf::option_wrapper_t<int> shadow_radius { "winshadows/shadow_radius" };
        wf::option_wrapper_t<bool> clip_shadow_inside { "winshadows/clip_shadow_inside" };
};

}
}
