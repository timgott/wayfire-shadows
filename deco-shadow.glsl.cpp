// GLSL as cpp string constant (.glsl extension for syntax highlighting)
#include "deco-shadow.hpp"


/* Vertex shader */

const std::string wf::winshadows::decoration_shadow_t::shadow_vert_shader = 
R"(
#version 100

attribute mediump vec2 position;
varying mediump vec2 uvpos;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(position.xy, 0.0, 1.0);
    uvpos = position.xy;
})";



/* Base fragment shader definitions */

const std::string box_shadow_fragment_header = 
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
)";


/* Rectangle shadow fragment shader */

const std::string wf::winshadows::decoration_shadow_t::shadow_frag_shader =
box_shadow_fragment_header + // include header and function definitions
R"(
void main()
{
    gl_FragColor = color * boxShadow(lower, upper, uvpos, sigma);
}
)";


/* Rectangle shadow+glow fragment shader */

const std::string wf::winshadows::decoration_shadow_t::shadow_glow_frag_shader =
box_shadow_fragment_header + // include header and function definitions
R"(
uniform vec4 glow_color;
uniform float glow_sigma;
uniform vec2 glow_lower;
uniform vec2 glow_upper;

void main()
{
    gl_FragColor =
        color * boxShadow(lower, upper, uvpos, sigma) +
        glow_color * boxShadow(glow_lower, glow_upper, uvpos, glow_sigma);
}
)";
