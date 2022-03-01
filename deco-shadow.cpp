#include "deco-shadow.hpp"

static const char *box_shadow_vertex_shader =
R"(#version 100

attribute mediump vec2 position;
varying mediump vec2 uvpos;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(position.xy, 0.0, 1.0);
    uvpos = position.xy;
})";

static const char *box_shadow_frag_shader = 
R"(
#version 100
precision mediump float;
varying vec2 uvpos;
uniform vec2 lower;
uniform vec2 upper;
uniform vec4 color;
uniform float sigma;

// Adapted from http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/
// License: CC0 (http://creativecommons.org/publicdomain/zero/1.0/)

// This approximates the error function, needed for the gaussian integral
vec4 erf(vec4 x) {
  vec4 s = sign(x), a = abs(x);
  x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
  x *= x;
  return s - s / (x * x);
}

// Return the mask for the shadow of a box from lower to upper
float boxShadow(vec2 lower, vec2 upper, vec2 point, float sigma) {
  vec4 query = vec4(lower - point, upper - point);
  vec4 integral = 0.5 + 0.5 * erf(query * (sqrt(0.5) / sigma));
  return (integral.z - integral.x) * (integral.w - integral.y);
}

void main()
{
    gl_FragColor = color * boxShadow(lower, upper, uvpos, sigma);
})";

wf::windecor::decoration_shadow_t::decoration_shadow_t() {
    OpenGL::render_begin();
    program.set_simple(
        OpenGL::compile_program(box_shadow_vertex_shader, box_shadow_frag_shader)
    );
    OpenGL::render_end();
}

void wf::windecor::decoration_shadow_t::render(const framebuffer_t& fb, wf::point_t window_origin, const geometry_t& scissor) {
    float radius = shadow_radius;

    wf::color_t color = shadow_color;

    // Emissiveness makes color additive instead of opaque
    // (exploiting premultiplied alpha)
    double alpha = color.a * (1.0 - shadow_emissiveness);

    // Premultiply alpha for shader
    glm::vec4 premultiplied = {
        color.r * color.a,
        color.g * color.a,
        color.b * color.a,
        alpha
    };

    OpenGL::render_begin( fb );
    fb.logic_scissor( scissor );

    program.use(wf::TEXTURE_TYPE_RGBA);

    float x = window_origin.x + geometry.x;
    float y = window_origin.y + geometry.y;
    float w = geometry.width;
    float h = geometry.height;

    GLfloat vertexData[] = {
        x, y + h,
        x + w, y + h,
        x + w, y,
        x, y,
    };

    glm::mat4 matrix = fb.get_orthographic_projection();

    program.attrib_pointer("position", 2, 0, vertexData);
    program.uniformMatrix4f("MVP", matrix);
    program.uniform1f("sigma", radius / 3.0f);
    program.uniform4f("color", premultiplied);

    float inner_x = window_geometry.x + window_origin.x + horizontal_offset;
    float inner_y = window_geometry.y + window_origin.y + vertical_offset;
    float inner_w = window_geometry.width;
    float inner_h = window_geometry.height;
    program.uniform2f("lower", inner_x, inner_y);
    program.uniform2f("upper", inner_x + inner_w, inner_y + inner_h);

    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

    program.deactivate();
    OpenGL::render_end();
}

int wf::windecor::decoration_shadow_t::get_radius() const {
    return shadow_radius;
}

wf::region_t wf::windecor::decoration_shadow_t::calculate_region() const {
    wf::region_t region(geometry);

    if (clip_shadow_inside) {
        region ^= window_geometry;
    }

    return region;
} 

wf::geometry_t wf::windecor::decoration_shadow_t::get_geometry() const {
    return geometry;
}

void wf::windecor::decoration_shadow_t::resize(const int window_width, const int window_height) {
    int radius = get_radius();

    window_geometry =  {
        0,
        0,
        window_width,
        window_height
    };

    geometry = {
        -radius + horizontal_offset, -radius + vertical_offset,
        window_width + radius * 2, window_height + radius * 2
    };
}
