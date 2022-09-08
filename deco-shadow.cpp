#include "deco-shadow.hpp"

wf::winshadows::decoration_shadow_t::decoration_shadow_t() {
    OpenGL::render_begin();
    shadow_program.set_simple(
        OpenGL::compile_program(shadow_vert_shader, shadow_frag_shader)
    );
    shadow_glow_program.set_simple(
        OpenGL::compile_program(shadow_vert_shader, shadow_glow_frag_shader)
    );
    OpenGL::render_end();
}

wf::winshadows::decoration_shadow_t::~decoration_shadow_t() {
    OpenGL::render_begin();
    shadow_program.free_resources();
    shadow_glow_program.free_resources();
    OpenGL::render_end();
}

void wf::winshadows::decoration_shadow_t::render(const framebuffer_t& fb, wf::point_t window_origin, const geometry_t& scissor, const bool glow) {
    float radius = shadow_radius_option;

    wf::color_t color = shadow_color_option;

    // Premultiply alpha for shader
    glm::vec4 premultiplied = {
        color.r * color.a,
        color.g * color.a,
        color.b * color.a,
        color.a
    };

    // Glow color, alpha=0 => additive blending (exploiting premultiplied alpha)
    wf::color_t glow_color = glow_color_option;
    glm::vec4 glow_premultiplied = {
        glow_color.r * glow_color.a,
        glow_color.g * glow_color.a,
        glow_color.b * glow_color.a,
        glow_color.a * (1.0 - glow_emissivity_option)
    };

    // Enable glow shader only when glow radius > 0 and view is focused
    bool use_glow = (glow && is_glow_enabled());
    OpenGL::program_t &program = 
        use_glow ? shadow_glow_program : shadow_program;

    OpenGL::render_begin(fb);
    fb.logic_scissor(scissor);

    program.use(wf::TEXTURE_TYPE_RGBA);

    // Compute vertex rectangle geometry
    wf::geometry_t bounds = outer_geometry + window_origin;
    float left = bounds.x;
    float right = bounds.x + bounds.width;
    float top = bounds.y;
    float bottom = bounds.y + bounds.height;

    GLfloat vertexData[] = {
        left, bottom,
        right, bottom,
        right, top,
        left, top
    };

    glm::mat4 matrix = fb.get_orthographic_projection();

    program.attrib_pointer("position", 2, 0, vertexData);
    program.uniformMatrix4f("MVP", matrix);
    program.uniform1f("sigma", radius / 3.0f);
    program.uniform4f("color", premultiplied);

    float inner_x = window_geometry.x + window_origin.x;
    float inner_y = window_geometry.y + window_origin.y;
    float inner_w = window_geometry.width;
    float inner_h = window_geometry.height;
    float shadow_x = inner_x + horizontal_offset;
    float shadow_y = inner_y + vertical_offset;
    program.uniform2f("lower", shadow_x, shadow_y);
    program.uniform2f("upper", shadow_x + inner_w, shadow_y + inner_h);

    if (use_glow) {
        program.uniform1f("glow_sigma", glow_radius_option / 3.0f);
        program.uniform4f("glow_color", glow_premultiplied);
        program.uniform2f("glow_lower", inner_x, inner_y);
        program.uniform2f("glow_upper", inner_x + inner_w, inner_y + inner_h);
    }

    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

    program.deactivate();
    OpenGL::render_end();
}

wf::region_t wf::winshadows::decoration_shadow_t::calculate_region() const {
    // TODO: geometry and region depending on whether glow is active or not
    wf::region_t region = wf::region_t(shadow_geometry) | wf::region_t(glow_geometry);

    if (clip_shadow_inside) {
        region ^= window_geometry;
    }

    return region;
}

wf::geometry_t wf::winshadows::decoration_shadow_t::get_geometry() const {
    return outer_geometry;
}

void wf::winshadows::decoration_shadow_t::resize(const int window_width, const int window_height) {
    window_geometry =  {
        0,
        0,
        window_width,
        window_height
    };

    shadow_geometry = {
        -shadow_radius_option + horizontal_offset, -shadow_radius_option + vertical_offset,
        window_width + shadow_radius_option * 2, window_height + shadow_radius_option * 2
    };

    glow_geometry = {
        -glow_radius_option, -glow_radius_option,
        window_width + glow_radius_option * 2, window_height + glow_radius_option * 2
    };

    int left = std::min(shadow_geometry.x, glow_geometry.x);
    int top = std::min(shadow_geometry.y, glow_geometry.y);
    int right = std::max(shadow_geometry.x + shadow_geometry.width, glow_geometry.x + glow_geometry.width);
    int bottom = std::max(shadow_geometry.y + shadow_geometry.height, glow_geometry.y + glow_geometry.height);
    outer_geometry = {
        left,
        top,
        right - left,
        bottom - top
    };
}

bool wf::winshadows::decoration_shadow_t::is_glow_enabled() const {
    return glow_radius_option > 0;
}
