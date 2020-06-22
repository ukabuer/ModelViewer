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

void main() {
  vec3 color = texture(rendered, v_uv).rgb;
  //  color = color / (color + vec3(1.0f));
  //  color = pow(color, vec3(1.0f / 2.2f));

  FragColor = vec4(color, 1.0f);
}
  #pragma sokol @end

  #pragma sokol @program postprocess postprocess_vs postprocess_fs