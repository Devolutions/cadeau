
#include "XmfBmp.h"
#include "XmfPng.h"
#include "XmfFile.h"

#include "XmfImage.h"

bool XmfImage_WriteFile(const char* filename, const uint8_t* data, uint32_t width, uint32_t height, uint32_t step)
{
	const char* ext = XmfFile_Extension(filename, false);

	if (!strcmp(ext, "bmp") || !strcmp(ext, "BMP")) {
		return XmfBmp_WriteFile(filename, data, width, height, step);
	} else if (!strcmp(ext, "png") || !strcmp(ext, "PNG")) {
		return XmfPng_WriteFile(filename, data, width, height, step);
	}

	return false;
}

bool XmfImage_ReadFile(const char* filename, uint8_t** data, uint32_t* width, uint32_t* height, uint32_t* step)
{
	const char* ext = XmfFile_Extension(filename, false);

	if (!strcmp(ext, "bmp") || !strcmp(ext, "BMP")) {
		return XmfBmp_ReadFile(filename, data, width, height, step);
	}
	else if (!strcmp(ext, "png") || !strcmp(ext, "PNG")) {
		return XmfPng_ReadFile(filename, data, width, height, step);
	}

	return false;
}
