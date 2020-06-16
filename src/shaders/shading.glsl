#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype vec4 Eigen::Vector4f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs shading_vs
in vec2 position;
in vec2 uv;

out vec2 v_uv;

void main() {
  gl_Position = vec4(position, 0.0, 1.0);
  v_uv = uv;
}
  #pragma sokol @end

  #pragma sokol @fs shading_fs
in vec2 v_uv;

out vec4 color;

uniform shading_fs_params {
  vec3 view_pos;
  vec3 light_direction;
  mat4 light_matrix;
};

uniform sampler2D g_world_pos;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D shadow_map;
uniform sampler2D ao_map;

float decode_depth(vec4 rgba) {
  return dot(rgba, vec4(1.0, 1.0/255.0, 1.0/65025.0, 1.0/160581375.0));
}

float calculate_shadow(vec3 world_pos, float bias) {
  vec4 light_space_postion = light_matrix * vec4(world_pos, 1.0f);
  vec3 projection_coords = light_space_postion.xyz / light_space_postion.w;
  projection_coords = projection_coords * 0.5 + 0.5;

  float closest_depth = decode_depth(texture(shadow_map, projection_coords.xy));
  float current_depth = projection_coords.z;

  float shadow = (current_depth - bias) > closest_depth  ? 1.0 : 0.0;
  return shadow;
}

void main() {
  vec3 world_pos = texture(g_world_pos, v_uv).xyz;
  vec3 normal = texture(g_normal, v_uv).xyz;
  vec4 albedo = texture(g_albedo, v_uv);
  vec3 diffuse = albedo.rgb;
  float shiniess = albedo.a;
  float ao = texture(ao_map, v_uv).r;

  vec3 view_dir = normalize(view_pos - world_pos);
  vec3 light_dir = normalize(-light_direction);

  float bias = max(0.05 * (1.0 - dot(normal, light_dir)), 0.005);
  float shadow = calculate_shadow(world_pos, bias);

  vec3 ambient = diffuse * 0.4f;
  if (shadow > 0.0) {
    color = vec4(ambient * ao, 1.0f);
  } else {
    vec3 halfway = normalize(view_dir + light_dir);

    float diffuse_strength = max(dot(light_dir, normal), 0.0);
    float specular_strength = pow(max(dot(halfway, normal), 0.0), 32);
    color = vec4(ambient * ao + (diffuse_strength * diffuse + specular_strength * vec3(1.0)), 1.0f);
  }

}
  #pragma sokol @end

  #pragma sokol @program shading shading_vs shading_fs