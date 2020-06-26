#pragma sokol @ctype mat4 Eigen::Matrix4f
#pragma sokol @vs rect_to_cube_vs
in vec3 position;

out vec3 v_position;

uniform vs_params {
  mat4 camera;
};

void main() {
  v_position = position;
  gl_Position =  camera * vec4(position, 1.0);
}
  #pragma sokol @end

  // **********

  #pragma sokol @fs rect_to_cube_fs
in vec3 v_position;

out vec4 FragColor;

uniform sampler2D equirectangular_map;

const vec2 inv_atan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v) {
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= inv_atan;
  uv += 0.5;
  return uv;
}

void main() {
  vec2 uv = SampleSphericalMap(normalize(v_position));
  vec3 color = texture(equirectangular_map, uv).rgb;

  FragColor = vec4(color, 1.0);
}
  #pragma sokol @end

  #pragma sokol @program rect_to_cube rect_to_cube_vs rect_to_cube_fs