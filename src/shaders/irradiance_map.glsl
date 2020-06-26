//@formatter:off
#pragma sokol @ctype mat4 Eigen::Matrix4f
#pragma sokol @vs irradiance_map_vs
in vec3 position;
out vec3 v_position;

uniform irradiance_map_vs_params {
  mat4 camera;
};

void main() {
  v_position = position;
  gl_Position = camera * vec4(position, 1.0f);
}
#pragma sokol @end

#pragma sokol @fs irradiance_map_fs
in vec3 v_position;

uniform samplerCube environment;

out vec4 FragColor;

const float PI = 3.14159265359;

void main() {
  // the sample direction equals the hemisphere's orientation
  vec3 normal = normalize(v_position);

  vec3 irradiance = vec3(0.0);

  vec3 up    = vec3(0.0, 1.0, 0.0);
  vec3 right = cross(up, normal);
  up = cross(normal, right);
  float sampleDelta = 0.025;
  float nrSamples = 0.0;

  for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
    for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
      // spherical to cartesian (in tangent space)
      vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
      // tangent space to world
      vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

      irradiance += texture(environment, sampleVec).rgb * cos(theta) * sin(theta);
      nrSamples++;
    }
  }
  irradiance = PI * irradiance * (1.0 / float(nrSamples));

  FragColor = vec4(irradiance, 1.0);
}
#pragma sokol @end

#pragma sokol @program irradiance_map irradiance_map_vs irradiance_map_fs