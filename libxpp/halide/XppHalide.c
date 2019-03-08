
static void halide_setup_rgb_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
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

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}

static void halide_setup_ycocg_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
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

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}

static void halide_setup_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width, int height, int stride)
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

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}

static void halide_setup_1d_u32_buffer_t(halide_buffer_t* buffer, uint32_t* data, int width)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->elem_size = 4;
	old_buffer->host = (uint8_t*)data;
	old_buffer->stride[0] = 1;
	old_buffer->host_dirty = true;

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}

static void halide_setup_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width, int height, int stride)
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

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}

static void halide_setup_1d_u8_buffer_t(halide_buffer_t* buffer, uint8_t* data, int width)
{
	buffer_t _old_buffer;
	buffer_t* old_buffer = &_old_buffer;

	memset(old_buffer, 0, sizeof(buffer_t));
	old_buffer->extent[0] = width;
	old_buffer->elem_size = 1;
	old_buffer->host = data;
	old_buffer->stride[0] = 1;
	old_buffer->host_dirty = true;

	halide_upgrade_buffer_t(NULL, NULL, old_buffer, buffer, 1);
}
