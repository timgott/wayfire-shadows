// GLSL as cpp string constant (.glsl extension for syntax highlighting)
#include "renderer.hpp"


/* Vertex shader */

const std::string winshadows::shadow_renderer_t::shadow_vert_shader = 
R"(
#version 300 es

in mediump vec2 position;
out mediump vec2 uvpos;

uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(position.xy, 0.0, 1.0);
    uvpos = position.xy;
})";



/* Base fragment shader definitions */

const std::string flag_define(const std::string& name, const bool value) {
    return "#define " + name + " " + (value? "1" : "0") + "\n";
}

const std::string frag_header(const std::string& light_type, const bool glow) {
    return
      "#version 300 es\n" +
      flag_define("CIRCULAR_SHADOW", light_type == "circular") +
      flag_define("GAUSSIAN_SHADOW", light_type == "gaussian") +
      flag_define("SQUARE_SHADOW", light_type == "square") +
      flag_define("GLOW", glow);
}

// All definitions are inserted in the shader, the shader compiler will remove unused ones
const std::string frag_body =
R"(
precision highp float;
in vec2 uvpos;
out vec4 fragColor;
uniform vec2 lower;
uniform vec2 upper;
uniform vec4 color;

uniform float radius;

uniform sampler2D dither_texture;

/* Gaussian shadow */

// Adapted from http://madebyevan.com/shaders/fast-rounded-rectangle-shadows/
// License: CC0 (http://creativecommons.org/publicdomain/zero/1.0/)
// This approximates the error function, needed for the gaussian integral
vec4 erf(vec4 x) {
  vec4 s = sign(x), a = abs(x);
  x = 1.0 + (0.278393 + (0.230389 + 0.078108 * (a * a)) * a) * a;
  x *= x;
  return s - s / (x * x);
}

// Computes a gaussian convolution of a box from lower to upper
float boxGaussian(vec2 lower, vec2 upper, vec2 point, float sigma) {
  vec4 query = vec4(lower - point, upper - point);
  vec4 integral = 0.5 + 0.5 * erf(query * (sqrt(0.5) / sigma));
  return (integral.z - integral.x) * (integral.w - integral.y);
}


/* Circular shadow */

// Antiderivative of sqrt(1-x^2)
float circleIntegral(float x) {
  return (sqrt(1.0-x*x)*x+asin(x)) / 2.0;
}

#define M_PI 3.14159265358

float circleSegment(float dist) {
  return sqrt(1.0-dist*dist);
}

// assuming fullArea is the area of two parts of a circle cut by a horizontal stripe
// (top and bottom are the cut lines)
// compute the remaining area when cut vertically at right
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

// Circle-rectangle overlap
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

// Shadow of rectangle under circular area light
float circularLightShadow(vec2 lower, vec2 upper, vec2 point, float radius) {
  vec4 query = vec4(lower - point, upper - point) / radius;
  return max(circleOverlap(query.st, query.pq), 0.0);
}

// Shadow of rectangle under square area light
float squareShadow(vec2 lower, vec2 upper, vec2 point, float radius) {
  vec2 squareLower = point - radius;
  vec2 squareUpper = point + radius;
  vec2 overlapLower = max(lower, squareLower);
  vec2 overlapUpper = min(upper, squareUpper);
  vec2 overlap = max(overlapUpper - overlapLower, 0.0);
  float maxArea = radius * radius * 4.0;
  return overlap.x * overlap.y / maxArea; // area
}


/* Glow */

uniform vec4 glow_color;
uniform float glow_spread;
uniform vec2 glow_lower;
uniform vec2 glow_upper;

uniform float glow_intensity;
uniform float glow_threshold;

/* Inverse quartic falloff */

vec2 invQrtIntegralPartial(float xmin, float xmax, vec2 y, float z) {
  // Rectangle integral over 1/(x^2+y^2+1)^2
  // Computed using FriCAS:
  //   formatExpression(integrate(integrate(1/((x^2+y^2+z^2)^2), x=a..b, "noPole"), y))$Format1D
  // Then some rewriting to make it valid glsl and simplified a bit

  float a = xmin;
  float b = xmax;

  float zsqr = z*z;
  float s1 = zsqr+a*a;
  float s2 = zsqr+b*b;
  vec2 s3 = zsqr+y*y;
  float t1 = sqrt(s1);
  float t2 = sqrt(s2);
  vec2 t3 = sqrt(s3);

  return (y*t1*t2*t3*(atan((b*t3)/(s3))-atan((a*t3)/(s3)))+(zsqr+y*y)*(b*t1*atan((y*t2)/(s2))+(-a)*t2*atan((y*t1)/(s1))))/((2.0*zsqr*(1.0+y*y*zsqr))*t1*t2);
}

float boxInvQrtFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  vec4 query = vec4(lower - point, upper - point) / scale;
  vec2 integralBounds = invQrtIntegralPartial(query.x, query.z, query.yw, 1.0);
  return (integralBounds.y - integralBounds.x);
}


/* Inverse square falloff, but only vertically and horizontally */

float orthoInvSqrFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  // f = (x^2+1)^(-1)
  // F = arctan(x)
  vec4 query = vec4(lower - point, upper - point) / scale;
  vec4 integral = atan(query);
  return (integral.z - integral.x) * (integral.w - integral.y);
}


/* Inverse square falloff, but only 1d based on distance to window edge */

float distInvSqrFalloff(vec2 lower, vec2 upper, vec2 point, float scale) {
  vec4 offsets = vec4(lower - point, point - upper) / scale;
  float a = max(max(offsets.x, offsets.z), 0.0);
  float b = max(max(offsets.y, offsets.w), 0.0);
  float dsqr = a*a + b*b;
  float invsqr = 1.0 / (dsqr + 1.0);
  return invsqr;//invsqr.x*invsqr.z * invsqr.y*invsqr.w;
}


/* Inverse square falloff integral over window edges (neon) */

vec4 barInvSqrFalloffIntegral(vec4 t, vec4 d, float z) {
  // FriCAS: integrate(1/(t^2+d^2+z^2), t)
  vec4 rsqr = d*d+z*z;
  vec4 r = sqrt(rsqr);
  return atan(t * r / rsqr) / r;
}

vec4 barInvCubicFalloffIntegral(vec4 t, vec4 d, float z) {
  // FriCAS: integrate(1/(t^2+d^2+z^2)^(3/2), t)
  vec4 rsqr = t*t+d*d+z*z;
  vec4 r = sqrt(rsqr);
  return -1.0/(t*r-rsqr);
}

float edgeInvSqrGlow(vec2 lower, vec2 upper, vec2 point, float scale) {
  // distance to edge left, top, right, bottom
  vec4 edgeDists = vec4(lower - point, upper - point);
  vec4 integralLower = barInvSqrFalloffIntegral(edgeDists.tsts, edgeDists, scale);
  vec4 integralUpper = barInvSqrFalloffIntegral(edgeDists.qpqp, edgeDists, scale);

  vec4 integral = integralUpper - integralLower;
  return (integral.s + integral.t + integral.p + integral.q);
}

float lightThreshold(float x, float minThreshold) {
    return max(x - minThreshold, 0.0);
}

vec4 dither(vec2 pos) {
    vec2 size = vec2(textureSize(dither_texture, 0));
    return texture(dither_texture, pos / size) / 256.0 - 0.5 / 256.0;
}

vec4 shadow_color()
{
#if CIRCULAR_SHADOW
    return color * circularLightShadow(lower, upper, uvpos, radius);
#elif SQUARE_SHADOW
    return color * squareShadow(lower, upper, uvpos, radius);
#else // GAUSSIAN_SHADOW
    return color * boxGaussian(lower, upper, uvpos, radius / 2.7);
#endif
}

/* Rectangle shadow+glow fragment shader */

void main()
{
#if GLOW
    float glow_value = edgeInvSqrGlow(glow_lower, glow_upper, uvpos, glow_spread), glow_neon_threshold;
    //float glow_value = boxInvQrtFalloff(glow_lower, glow_upper, uvpos, glow_spread);
    //float glow_value = boxGaussian(glow_lower, glow_upper, uvpos, glow_spread)
    //float glow_value = distInvSqrFalloff(glow_lower, glow_upper, uvpos, glow_spread);
    //float glow_value = orthoInvSqrFalloff(glow_lower, glow_upper, uvpos, glow_spread);
    vec4 out_color =
        shadow_color() +
        glow_intensity * glow_color * lightThreshold(glow_value, glow_threshold);
#else
    vec4 out_color = shadow_color();
#endif
    out_color += dither(uvpos + lower*upper);
    fragColor = out_color;
}

)";

const std::string winshadows::shadow_renderer_t::frag_shader(const std::string &light_type, const bool glow) {
    return frag_header(light_type, glow) + frag_body;
}