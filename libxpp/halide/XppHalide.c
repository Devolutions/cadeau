#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <HalideRuntime.h>

#include "XppHalide.h"

/*
 * halide_buffer_t:
 * uint64_t device;
 * const struct halide_device_interface_t *device_interface;
 * uint8_t* host;
 * uint64_t flags;
 * struct halide_type_t type;
 * int32_t dimensions;
 * halide_dimension_t* dim;
 * void* padding;
 *
 * halide_type_t:
 * uint8_t code; // halide_type_code_t
 * uint8_t bits; // The number of bits of precision of a single scalar value of this type.
 * uint16_t lanes; // How many elements in a vector. This is 1 for scalar types.
 *
 * halide_type_code_t:
 * halide_type_int = 0,   //!< signed integers
 * halide_type_uint = 1,  //!< unsigned integers
 * halide_type_float = 2, //!< floating point numbers
 * halide_type_handle = 3 //!< opaque pointer type (void *)
 *
 * halide_buffer_flags:
 * halide_buffer_flag_host_dirty = 1,
 * halide_buffer_flag_device_dirty = 2
 *
 * halide_dimension_t:
 * int32_t min;
 * int32_t extent;
 * int32_t stride;
 * uint32_t flags;
 */

const struct halide_type_t halide_type_int8_t  = { halide_type_int,  8, 1 }; // halide_type_of<int8_t>
const struct halide_type_t halide_type_int16_t = { halide_type_int, 16, 1 }; // halide_type_of<int16_t>
const struct halide_type_t halide_type_int32_t = { halide_type_int, 32, 1 }; // halide_type_of<int32_t>
const struct halide_type_t halide_type_int64_t = { halide_type_int, 64, 1 }; // halide_type_of<int64_t>

const struct halide_type_t halide_type_uint8_t  = { halide_type_uint,  8, 1 }; // halide_type_of<uint8_t>
const struct halide_type_t halide_type_uint16_t = { halide_type_uint, 16, 1 }; // halide_type_of<uint16_t>
const struct halide_type_t halide_type_uint32_t = { halide_type_uint, 32, 1 }; // halide_type_of<uint32_t>
const struct halide_type_t halide_type_uint64_t = { halide_type_uint, 64, 1 }; // halide_type_of<uint64_t>

void halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint8_t;
	buffer->host = data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = 4;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dim[1].min = 0;
	buffer->dim[1].extent = width;
	buffer->dim[1].stride = 4;
	buffer->dim[1].flags = 0;
	buffer->dim[2].min = 0;
	buffer->dim[2].extent = height;
	buffer->dim[2].stride = stride;
	buffer->dim[2].flags = 0;
	buffer->dimensions = 3;
	buffer->padding = NULL;
}

void halide_setup_16s_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_int16_t;
	buffer->host = data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dim[1].min = 0;
	buffer->dim[1].extent = height;
	buffer->dim[1].stride = stride;
	buffer->dim[1].flags = 0;
	buffer->dimensions = 2;
	buffer->padding = NULL;
}

void halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint8_t;
	buffer->host = data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dim[1].min = 0;
	buffer->dim[1].extent = height;
	buffer->dim[1].stride = stride;
	buffer->dim[1].flags = 0;
	buffer->dimensions = 2;
	buffer->padding = NULL;
}

void halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint32_t;
	buffer->host = (uint8_t*) data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dim[1].min = 0;
	buffer->dim[1].extent = height;
	buffer->dim[1].stride = stride;
	buffer->dim[1].flags = 0;
	buffer->dimensions = 2;
	buffer->padding = NULL;
}

void halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint32_t;
	buffer->host = (uint8_t*) data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dimensions = 1;
	buffer->padding = NULL;
}

void halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint32_t;
	buffer->host = data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dim[1].min = 0;
	buffer->dim[1].extent = height;
	buffer->dim[1].stride = stride;
	buffer->dim[1].flags = 0;
	buffer->dimensions = 2;
	buffer->padding = NULL;
}

void halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width)
{
	buffer->device = 0;
	buffer->device_interface = NULL;
	buffer->type = halide_type_uint8_t;
	buffer->host = data;
	buffer->flags = halide_buffer_flag_host_dirty;
	buffer->dim[0].min = 0;
	buffer->dim[0].extent = width;
	buffer->dim[0].stride = 1;
	buffer->dim[0].flags = 0;
	buffer->dimensions = 1;
	buffer->padding = NULL;
}
