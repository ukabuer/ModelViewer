#pragma once
#include <cstdint>
#include <sokol_gfx.h>

sg_image irradiance_convolution(const sg_image &environment_map);

sg_image rect_image_to_cube_map(const char *filepath, sg_buffer cube_buffer,
                                sg_buffer cube_index_buffer);

sg_image prefilter_environment_map(const sg_image &environment_map);