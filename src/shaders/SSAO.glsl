#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs ssao_vs
in vec2 position;
in vec2 uv;

out vec2 v_uv;

void main() {
  gl_Position = vec4(position, 0.0, 1.0);
  v_uv = uv;
}
  #pragma sokol @end

  #pragma sokol @fs ssao_fs
in vec2 v_uv;

out float color;

uniform sampler2D position_tex;
uniform sampler2D normal_tex;
uniform sampler2D noise_tex;
uniform fs_params {
  float scrren_width;
  float scrren_height;
  mat4 view_matrix;
  mat4 projection_matrix;
  vec3 samples[64];
};

const int kernel_size = 64;
const float radius = 0.5;
const float bias = 0.025;

void main() {
  vec2 noise_scale = vec2(scrren_width / 4.0, scrren_height / 4.0);

  vec3 world_pos = texture(position_tex, v_uv).xyz;
  vec4 view_pos = view_matrix * vec4(world_pos, 1.0f);
  view_pos.xyz = view_pos.xyz / view_pos.w;
  float depth = view_pos.z;

  vec3 normal = texture(normal_tex, v_uv).xyz;

  vec3 noise = normalize(texture(noise_tex, v_uv * noise_scale).xyz);
  vec3 tangent = normalize(noise - normal * dot(noise, normal));
  vec3 B = cross(normal, tangent);
  mat3 TBN = mat3(tangent, B, normal);

  float occlusion = 0.0;
  for (int i =0; i < kernel_size; i++) {
    vec3 sp = TBN * samples[i];
    sp = view_pos.xyz + sp * radius;

    vec4 offset = projection_matrix * vec4(sp, 1.0f);
    offset.xy = (offset.xy / offset.w) * 0.5 + 0.5;

    vec3 sampled_pos = texture(position_tex, offset.xy).xyz;
    vec4 sampled_view_pos = view_matrix * vec4(sampled_pos, 1.0f);
    float sp_depth = sampled_view_pos.z / sampled_view_pos.w;
    float range_check = smoothstep(0.0, 1.0, radius / abs(depth - sp_depth));
    occlusion += (sp_depth < (sp.z + bias) ? 1.0f : 0.0f) * range_check;
  }
  occlusion = 1.0 - (occlusion / kernel_size);
  color = occlusion;
}
  #pragma sokol @end

  #pragma sokol @program ssao ssao_vs ssao_fs