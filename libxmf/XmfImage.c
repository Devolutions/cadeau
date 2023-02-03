
#include "XmfBmp.h"
#include "XmfPng.h"
#include "XmfFile.h"

#include "XmfImage.h"

bool XmfImage_LoadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step)
{
	const char* ext = XmfFile_Extension(filename, false);

	if (!strcmp(ext, "bmp") || !strcmp(ext, "BMP")) {
		return XmfBmp_LoadFile(filename, data, width, height, step);
	}
	else if (!strcmp(ext, "png") || !strcmp(ext, "PNG")) {
		return XmfPng_LoadFile(filename, data, width, height, step);
	}

	return false;
}

bool XmfImage_SaveFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step)
{
	const char* ext = XmfFile_Extension(filename, false);

	if (!strcmp(ext, "bmp") || !strcmp(ext, "BMP")) {
		return XmfBmp_SaveFile(filename, data, width, height, step);
	} else if (!strcmp(ext, "png") || !strcmp(ext, "PNG")) {
		return XmfPng_SaveFile(filename, data, width, height, step);
	}

	return false;
}

void XmfImage_FreeData(uint8_t* data)
{
	if (data) {
		free(data);
	}
}
