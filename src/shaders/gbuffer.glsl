#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs gbuffer_vs
in vec3 position;
in vec3 normal;
in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_world_pos;

uniform gbuffer_vs_params {
  mat4 model;
  mat4 camera;
};

void main() {
  vec4 world_pos = model * vec4(position, 1.0f);
  v_uv = uv;
  v_normal = mat3(transpose(inverse(model))) * normal;
  v_world_pos = world_pos.xyz / world_pos.w;
  gl_Position = camera * world_pos;
}
  #pragma sokol @end

  #pragma sokol @fs gbuffer_fs
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_pos;

out vec4 g_world_pos;
out vec3 g_normal;
out vec4 g_albedo;

uniform sampler2D albedo;
uniform sampler2D normal_map;
uniform sampler2D metallic_roughness_map;

vec3 srgb_to_linear(vec3 srgb_color) {
  const float gamma = 2.2f;
  return pow(srgb_color, vec3(gamma));
}

void main() {
  vec3 uv_dx = dFdx(vec3(v_uv, 0.0));
  vec3 uv_dy = dFdy(vec3(v_uv, 0.0));
  vec3 t_ = (uv_dy.t * dFdx(v_world_pos) - uv_dx.t * dFdy(v_world_pos)) /
  (uv_dx.s * uv_dy.t - uv_dy.s * uv_dx.t);
  vec3 T = normalize(t_ - v_normal * dot(v_normal, t_));
  vec3 B = cross(v_normal, T);
  mat3 TBN = mat3(T, B, v_normal);

  g_world_pos.xyz = v_world_pos;
  g_normal = texture(normal_map, v_uv).rgb;
  g_normal = g_normal * 2.0f - 1.0f;
  g_normal = normalize(TBN * g_normal);

  vec4 metallic_roughness = texture(metallic_roughness_map, v_uv);
  g_albedo.rgb = srgb_to_linear(texture(albedo, v_uv).rgb);
  g_albedo.a = metallic_roughness.g;// roughness
  g_world_pos.w = metallic_roughness.b;// metallic
}
  #pragma sokol @end

  #pragma sokol @program gbuffer gbuffer_vs gbuffer_fs