#ifndef XPP_HALIDE_INTERNAL_H
#define XPP_HALIDE_INTERNAL_H

#include <HalideRuntime.h>

bool halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
bool halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
bool halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride);
bool halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width);
bool halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride);
bool halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width);

#endif /* XPP_HALIDE_INTERNAL_H */
