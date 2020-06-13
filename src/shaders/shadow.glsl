#pragma sokol @ctype vec3 Eigen::Vector3f
#pragma sokol @ctype mat4 Eigen::Matrix4f

#pragma sokol @vs shadow_vs
in vec3 position;
out float depth;

uniform shadow_vs_params {
  mat4 model;
  mat4 light_space_matrix;
};

void main() {
  gl_Position = light_space_matrix * model * vec4(position, 1.0);
  depth = (gl_Position.z * 0.5 + 0.5) / gl_Position.w;
}
#pragma sokol @end

#pragma sokol @fs shadow_fs
in float depth;
out vec4 color;

vec4 encode_depth(float v) {
  vec4 enc = vec4(1.0, 255.0, 65025.0, 160581375.0) * v;
  enc = fract(enc);
  enc -= enc.yzww * vec4(1.0/255.0,1.0/255.0,1.0/255.0,0.0);
  return enc;
}

void main() {
  color = encode_depth(depth);
}
#pragma sokol @end

#pragma sokol @program shadow shadow_vs shadow_fs
