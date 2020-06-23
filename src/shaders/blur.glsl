#pragma sokol @vs blur_vs
in vec2 position;
in vec2 uv;

out vec2 v_uv;

void main() {
  v_uv = uv;
  gl_Position = vec4(position, 0.0f, 1.0f);
}
#pragma sokol @end

  #pragma sokol @fs blur_fs
in vec2 v_uv;
out float result;

uniform sampler2D tex;
uniform blur_fs_params {
  float texture_width;
  float texture_height;
};

void main() {
  vec2 texel_size = vec2(1.0f / texture_width, 1.0f / texture_height);
  for (int y = -1; y <= 1; y++) {
    for (int x = -1; x <= 1; x++) {
      vec2 offset = vec2(float(x), float(y)) * texel_size;
      result += texture(tex, v_uv + offset).r;
    }
  }

  result = result / (3.0f * 3.0f);
}
  #pragma sokol @end

  #pragma sokol @program blur  blur_vs blur_fs