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
precision highp float;
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

// Antiderivative of sqrt(1-x^2)
float circleIntegral(float x) {
  return (sqrt(1.0-x*x)*x+asin(x)) / 2.0;
}

#define M_PI 3.14159265358

float circleSegment(float dist) {
  return sqrt(1.0-dist*dist);
}

float circleMinusWall(float top, float bottom, float right, float fullArea) {
  if (right <= -1.0) {
    return fullArea; // entire circle
  } else if (right >= 1.0) {
    return 0.0; // nothing
  } else {
    // compute circle segment half width
    float w = circleSegment(right);
    // circle segment area
    float segmentTop = max(top, -w);
    float segmentBottom = min(bottom, w);
    float area = circleIntegral(segmentBottom) - circleIntegral(segmentTop) - (segmentBottom - segmentTop) * abs(right);
    if (right < 0.0) {
      return fullArea - area;
    } else {
      return area;
    }
  }
}

// Circle / rectangle overlap
// circle is at (0,0) radius 1
// only one top-left corner of rectangle is considered (assume rectangle >> circle)
float circleOverlap(vec2 lower, vec2 upper) {
  // left/right half integral with vertical bounds
  float top = max(lower.y, -1.0);
  float bottom = min(upper.y, 1.0);
  float inner = 2.0 * (circleIntegral(bottom) - circleIntegral(top));
  // left/right outer integrals for horizontal bounds
  float outerLeft = circleMinusWall(top, bottom, -lower.x, inner);
  float outerRight = circleMinusWall(top, bottom, upper.x, inner);
  return (inner - outerLeft - outerRight) / M_PI;
}

float circlularLightShadow(vec2 lower, vec2 upper, vec2 point, float radius) {
  vec4 query = vec4(lower - point, upper - point) / radius;
  return circleOverlap(query.st, query.pq);
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

vec2 invQrtIntegralPartial(vec2 y, float xmin, float xmax) {
  // Rectangle integral over 1/(x^2+y^2+1)^2
  // Computed using FriCAS:
  // formatExpression(integrate(integrate(1/((x^2+y^2+1)^2), x=a..b, "noPole"), y))$Format1D
  float a = xmin;
  float b = xmax;
  return (y*sqrt(a*a+1.0)*sqrt(b*b+1.0)*sqrt(y*y+1.0)*atan((b*sqrt(y*y+1.0))/(y*y+1.0))+(
    -y)*sqrt(a*a+1.0)*sqrt(b*b+1.0)*sqrt(y*y+1.0)*atan((a*sqrt(y*y+1.0))/(y*y+1.0))+(
    b*y*y+b)*sqrt(a*a+1.0)*atan((y*sqrt(b*b+1.0))/(b*b+1.0))+((-a)*y*y-a)*sqrt(b*b+1.0)*atan((y*sqrt(a*a+1.0))/(a*a+1.0)))/((2.0*y*y+2.0)*sqrt(a*a+1.0)*sqrt(b*b+1.0));
}

float boxInvQrtFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  vec4 query = vec4(lower - point, upper - point) / scale;
  vec2 integralBounds = invQrtIntegralPartial(query.yw, query.x, query.z);
  return (integralBounds.y - integralBounds.x);
}

float orthoInvSqrFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  // f = (x^2+1)^(-1)
  // F = arctan(x)
  vec4 query = vec4(lower - point, upper - point) / scale;
  vec4 integral = atan(query);
  return (integral.z - integral.x) * (integral.w - integral.y);
}

float edgeInvSqrFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  vec4 offsets = vec4(lower - point, point - upper) / scale;
  float a = max(max(offsets.x, offsets.z), 0.0);
  float b = max(max(offsets.y, offsets.w), 0.0);
  float dsqr = a*a + b*b;
  float invsqr = 1.0 / (dsqr + 1.0);
  return invsqr;//invsqr.x*invsqr.z * invsqr.y*invsqr.w;
}

vec3 toneMapGlow(vec3 x) {
    return 1.0 - exp(-x);
}

void main()
{
    float glow_value = 2.0 * boxInvQrtFalloff(glow_lower, glow_upper, uvpos, glow_sigma);
    //float glow_value = edgeInvSqrFalloff(glow_lower, glow_upper, uvpos, glow_sigma);
    //float glow_value = orthoInvSqrFalloff(glow_lower, glow_upper, uvpos, glow_sigma);
    //float glow_value = boxShadow(glow_lower, glow_upper, uvpos, glow_sigma);
    gl_FragColor =
        color * circlularLightShadow(lower, upper, uvpos, sigma * 1.8) +
        //glow_color * boxShadow(glow_lower, glow_upper, uvpos, glow_sigma);
        glow_color * glow_value;
}
)";
