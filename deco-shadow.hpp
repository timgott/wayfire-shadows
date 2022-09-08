#pragma once
#include <wayfire/option-wrapper.hpp>
#include <wayfire/opengl.hpp>

namespace wf::winshadows {
/**
 * A  class that can render shadows.
 * It manages the shader and calculates the necessary padding.
 */
class decoration_shadow_t {
    public:
        decoration_shadow_t();
        ~decoration_shadow_t();

        void render(const framebuffer_t& fb, wf::point_t origin, const geometry_t& scissor, const bool glow);
        void resize(const int width, const int height);
        wf::region_t calculate_region() const;
        wf::geometry_t get_geometry() const;
        bool is_glow_enabled() const;

    private:
        OpenGL::program_t shadow_program;
        OpenGL::program_t shadow_glow_program;
        wf::geometry_t glow_geometry;
        wf::geometry_t shadow_geometry;
        wf::geometry_t outer_geometry;
        wf::geometry_t window_geometry;
        wlr_box calculate_padding(const wf::geometry_t window_geometry) const;

        wf::option_wrapper_t<wf::color_t> shadow_color_option { "winshadows/shadow_color" };
        wf::option_wrapper_t<int> shadow_radius_option { "winshadows/shadow_radius" };
        wf::option_wrapper_t<bool> clip_shadow_inside { "winshadows/clip_shadow_inside" };
        wf::option_wrapper_t<int> vertical_offset { "winshadows/vertical_offset" };
        wf::option_wrapper_t<int> horizontal_offset { "winshadows/horizontal_offset" };

        wf::option_wrapper_t<wf::color_t> glow_color_option { "winshadows/glow_color" };
        wf::option_wrapper_t<int> glow_radius_option { "winshadows/glow_radius" };
        wf::option_wrapper_t<double> glow_emissivity_option { "winshadows/glow_emissivity" };

        static const std::string shadow_vert_shader;
        static const std::string shadow_frag_shader;
        static const std::string shadow_glow_frag_shader;
};

}

