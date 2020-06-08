#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

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

out vec4 color;

uniform postprocess_fs_params {
  vec3 view_pos;
};

uniform sampler2D g_world_pos;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;

void main() {
  vec3 world_pos = texture(g_world_pos, v_uv).xyz;
  vec3 normal = texture(g_normal, v_uv).xyz;
  vec4 albedo = texture(g_albedo, v_uv);
  vec3 diffuse = albedo.rgb;
  float shiniess = albedo.a;

  vec3 view_dir = normalize(view_pos - world_pos);
  vec3 light_dir = normalize(vec3(0, 2, 0) - world_pos);
  vec3 halfway = normalize(view_dir + light_dir);

  float diffuse_strength = max(dot(light_dir, normal), 0.0);
  float specular_strength = pow(max(dot(halfway, normal), 0.0), 32);

  color = vec4(diffuse * 0.1 + diffuse_strength * diffuse + specular_strength * vec3(1.0), 1.0f);
}
  #pragma sokol @end

  #pragma sokol @program postprocess postprocess_vs postprocess_fs