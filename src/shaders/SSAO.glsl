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
  float screen_width;
  float screen_height;
  mat4 view_matrix;
  mat4 projection_matrix;
  vec4 samples[64];// padding bug with sokol-shdc if use vec3 [issues#27]
};

const int kernel_size = 64;
const float radius = 1.0;
const float bias = 0.025;

void main() {
  vec2 noise_scale = vec2(screen_width / 4.0, screen_height / 4.0);

  vec3 world_pos = texture(position_tex, v_uv).xyz;
  vec4 view_pos = view_matrix * vec4(world_pos, 1.0f);
  view_pos.xyz = view_pos.xyz / view_pos.w;
  float depth = view_pos.z;

  vec3 normal = texture(normal_tex, v_uv).xyz;
  vec3 viewspace_normal = mat3(transpose(inverse(view_matrix))) * normal;

  vec3 noise = normalize(texture(noise_tex, v_uv * noise_scale).xyz);
  vec3 tangent = normalize(noise - viewspace_normal * dot(noise, viewspace_normal));
  vec3 B = cross(viewspace_normal, tangent);
  mat3 TBN = mat3(tangent, B, viewspace_normal);

  float occlusion = 0.0;
  for (int i = 0; i < kernel_size; i++) {
    vec3 sampled = TBN * samples[i].xyz;
    sampled = view_pos.xyz + sampled * radius;

    vec4 offset = projection_matrix * vec4(sampled, 1.0f);
    offset.xy = (offset.xy / offset.w) * 0.5 + 0.5;

    vec3 actual_pos = texture(position_tex, offset.xy).xyz;
    vec4 actual_view_pos = view_matrix * vec4(actual_pos, 1.0f);
    float actual_depth = actual_view_pos.z / actual_view_pos.w;
    float range_check = smoothstep(0.0, 1.0, radius / abs(depth - actual_depth));
    occlusion += (actual_depth >= (sampled.z + bias) ? 1.0f : 0.0f) * range_check;
  }
  occlusion = 1.0 - (occlusion / kernel_size);
  color = occlusion;
}
  #pragma sokol @end

  #pragma sokol @program ssao ssao_vs ssao_fs