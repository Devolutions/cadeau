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
 *
 * halide_dimension_t:
 * int32_t min;
 * int32_t extent;
 * int32_t stride;
 * uint32_t flags;
 */

const struct halide_type_t halide_type_uint8_t  = { halide_type_uint,  8, 1 }; // halide_type_of<uint8_t>
const struct halide_type_t halide_type_uint16_t = { halide_type_uint, 16, 1 }; // halide_type_of<uint16_t>
const struct halide_type_t halide_type_uint32_t = { halide_type_uint, 32, 1 }; // halide_type_of<uint32_t>
const struct halide_type_t halide_type_uint64_t = { halide_type_uint, 64, 1 }; // halide_type_of<uint64_t>

bool halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4]; // FIXME: this will go out of scope
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = 4;
	old_buffer->stride[0] = 1;
	old_buffer->extent[1] = width;
	old_buffer->stride[1] = 4;
	old_buffer->extent[2] = height;
	old_buffer->stride[2] = stride;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;
	buffer->host = data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = 4;
	dim[0].stride = 1;
	dim[1].extent = width;
	dim[1].stride = 4;
	dim[2].extent = height;
	dim[2].stride = stride;
	buffer->dimensions = 3;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4];
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = width;
	old_buffer->stride[0] = 1;
	old_buffer->extent[1] = height;
	old_buffer->stride[1] = stride;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;
	buffer->host = data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = width;
	dim[0].stride = 1;
	dim[1].extent = height;
	dim[1].stride = stride;
	buffer->dimensions = 2;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4];
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 4;
	old_buffer->host = (uint8_t*) data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = width;
	old_buffer->stride[0] = 1;
	old_buffer->extent[1] = height;
	old_buffer->stride[1] = stride;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;
	buffer->host = (uint8_t*) data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = width;
	dim[0].stride = 1;
	dim[1].extent = height;
	dim[1].stride = stride;
	buffer->dimensions = 2;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4];
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 4;
	old_buffer->host = (uint8_t*) data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = width;
	old_buffer->stride[0] = 1;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;
	buffer->host = (uint8_t*) data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = width;
	dim[0].stride = 1;
	buffer->dimensions = 1;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4];
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 4;
	old_buffer->host = data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = width;
	old_buffer->stride[0] = 1;
	old_buffer->extent[1] = height;
	old_buffer->stride[1] = stride;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint32_t;
	buffer->host = data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = width;
	dim[0].stride = 1;
	dim[1].extent = height;
	dim[1].stride = stride;
	buffer->dimensions = 2;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}

bool halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width)
{
	buffer_t _old_buffer;
	halide_dimension_t dim[4];
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->host_dirty = true;
	old_buffer->extent[0] = width;
	old_buffer->stride[0] = 1;

	memset(buffer, 0, sizeof(halide_buffer_t));
	buffer->type = halide_type_uint8_t;
	buffer->host = data;

	memset(dim, 0, sizeof(dim));
	buffer->dim = (halide_dimension_t*) &dim;
	dim[0].extent = width;
	dim[0].stride = 1;
	buffer->dimensions = 1;

	return (halide_upgrade_buffer_t(NULL, "", old_buffer, buffer, 0) == 0) ? true : false;
}
