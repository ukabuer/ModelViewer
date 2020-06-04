#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <cstdint>
#include <sokol_gfx.h>
#include <tinygltf/tiny_gltf.h>
#include <vector>

#define MATH_PI 3.1415926

struct Geometry {
    sg_buffer vertices;
    sg_buffer indices;

    static auto Box(float width = 1.0f, float height = 1.0f, float depth = 1.0f) -> Geometry;
    static auto Sphere(float radius, uint32_t subdivisions_axis,
                       uint32_t subdivisions_height, float start_latitude_in_radians = 0,
                       float end_latitude_in_radians = MATH_PI, float start_longitude_in_radians = 0,
                       float end_longitude_in_radians = 2 * MATH_PI) -> Geometry;
};

struct Mesh {
    Geometry geometry{};
    unsigned char *albedo_map = nullptr;
    unsigned char *normal_map = nullptr;
    unsigned char *metallic_map = nullptr;
    unsigned char *roughness_map = nullptr;
};

class Scene {
public:
    tinygltf::Model model;
    std::vector<sg_buffer> buffers;
    std::vector<sg_image> textures;

    static auto Load(const char *filename) -> Scene;
};
