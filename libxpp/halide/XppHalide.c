#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <HalideRuntime.h>

#include "XppHalide.h"

/*
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
 */

const struct halide_type_t halide_type_uint8_t  = { halide_type_uint,  8, 1 }; // halide_type_of<uint8_t>
const struct halide_type_t halide_type_uint16_t = { halide_type_uint, 16, 1 }; // halide_type_of<uint16_t>
const struct halide_type_t halide_type_uint32_t = { halide_type_uint, 32, 1 }; // halide_type_of<uint32_t>
const struct halide_type_t halide_type_uint64_t = { halide_type_uint, 64, 1 }; // halide_type_of<uint64_t>

bool halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = 4;
	old_buffer->extent[1] = width;
	old_buffer->extent[2] = height;
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->stride[0] = 1;
	old_buffer->stride[1] = 4;
	old_buffer->stride[2] = stride;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->extent[1] = height;
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->stride[0] = 1;
	old_buffer->stride[1] = stride;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->extent[1] = height;
	old_buffer->elem_size = 4;
	old_buffer->host = (uint8_t*)data;
	old_buffer->stride[0] = 1;
	old_buffer->stride[1] = stride;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->elem_size = 4;
	old_buffer->host = (uint8_t*)data;
	old_buffer->stride[0] = 1;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->extent[1] = height;
	old_buffer->elem_size = 4;
	old_buffer->host = data;
	old_buffer->stride[0] = 1;
	old_buffer->stride[1] = stride;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->stride[0] = 1;
	old_buffer->host_dirty = true;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}
