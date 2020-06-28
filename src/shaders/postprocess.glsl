#pragma sokol @vs postprocess_vs
in vec2 position;
in vec2 uv;

out vec2 v_uv;

void main() {
  gl_Position = vec4(position, 0.0, 1.0);
  v_uv = uv;
}
  #pragma sokol @end

  #pragma sokol @fs postprocess_fs
in vec2 v_uv;

out vec4 FragColor;

uniform sampler2D rendered;
uniform sampler2D bright_color;

// ACES tone map
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 tone_map_ACES(vec3 color) {
  const float A = 2.51;
  const float B = 0.03;
  const float C = 2.43;
  const float D = 0.59;
  const float E = 0.14;
  return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

vec3 linear_to_srgb(vec3 linear_color) {
  const float gamma = 2.2f;
  return pow(linear_color, vec3(1.0f / gamma));
}

void main() {
  vec3 bloom = texture(bright_color, v_uv).rgb;
  vec3 color = texture(rendered, v_uv).rgb + bloom;
  color = tone_map_ACES(color);
  color = linear_to_srgb(color);

  FragColor = vec4(color, 1.0f);
}
  #pragma sokol @end

  #pragma sokol @program postprocess postprocess_vs postprocess_fs