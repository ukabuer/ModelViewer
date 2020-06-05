#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs vs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

out vec3 v_normal;
out vec2 v_uv;
out vec3 v_world_pos;

uniform vs_params {
  mat4 model;
  mat4 camera;
};

void main() {
  vec4 world_pos = model * vec4(position, 1.0f);
  v_uv = uv;
  v_normal = normal;
  v_world_pos = world_pos.xyz / world_pos.w;
  gl_Position = camera * world_pos;
}
  #pragma sokol @end

  #pragma sokol @fs fs
in vec3 v_normal;
in vec2 v_uv;
in vec3 v_world_pos;
out vec4 color;

uniform sampler2D albedo;
uniform fs_params {
  vec3 view_pos;
};

void main() {
  vec3 view_dir = normalize(view_pos - v_world_pos);
  vec3 light_dir = normalize(vec3(0.0f, 4.0f, 0.0f) - v_world_pos);
  vec3 halfway_dir = normalize(view_dir + light_dir);

  float diffuse_strength = max(dot(v_normal, light_dir), 0.0f);
  float specular_strength = pow(max(dot(v_normal, halfway_dir), 0.0f), 32);

  vec3 base = texture(albedo, v_uv).rgb;
  vec3 ambient = base * 0.1f;

  color = vec4(ambient + base * diffuse_strength + specular_strength * vec3(1.0f), 1.0f);
}
  #pragma sokol @end

  #pragma sokol @program triangle vs fs