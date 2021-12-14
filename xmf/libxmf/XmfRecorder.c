#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "XmfFile.h"
#include "XmfMath.h"
#include "XmfRecorder.h"
#include "XmfWebM.h"

#include <time.h>

#define BIT_RATE_MIN 32
#define BIT_RATE_MAX 2048

struct now_recorder
{
	bool initialized;
	bool enabled;
	XmfWebM* webm;
	int fileNumber;
	uint32_t frameRateMin;
	uint32_t frameRate;
	uint32_t frameWidth;
	uint32_t frameHeight;
	uint32_t videoQuality;
	uint64_t lastUpdateTime;
	char filename[XMF_MAX_PATH];
	char directory[XMF_MAX_PATH];
};

#define TAG "XmfRecorder"

bool XmfRecorder_GenerateFileName(char* prototype, char* directory, int fileNumber, char* pszResult, size_t cchResult)
{
	static const char* fmt = "%s%s%s-%d.webm"; // [prototype][-][timestamp]-[video#].webm
	bool result = false;
	char timestamp[64];
	char filename[XMF_MAX_PATH];
	char* sanitized;
	time_t t = time(NULL);
	struct tm* ptm = localtime(&t);

	if (!directory || strlen(directory) == 0)
		return result;

	strftime(timestamp, sizeof(timestamp), "%FT%T", ptm);
	sprintf_s(filename, sizeof(filename), fmt,
		(prototype && strlen(prototype) > 0) ? prototype : "",
		(prototype && strlen(prototype) > 0) ? "-" : "",
		timestamp, fileNumber);

	sanitized = XmfFile_SanitizeName(filename, '_', 0);

	if (!sanitized)
		return result;

	result = XmfPath_Combine(pszResult, cchResult, directory, sanitized);

	free(sanitized);

	return result;
}

uint32_t XMF_API XmfRecorder_CalculateBitRate(uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate, 
	uint32_t quality)
{
	/* Linear quality <=> bit-rate scale-factor */
	double dResult =
		(double) clamp(quality, XMF_RECORDER_QUALITY_MIN, XMF_RECORDER_QUALITY_MAX)
		* (double) frameWidth
		* (double) frameHeight
		* (double) clamp(frameRate, XMF_RECORDER_FRAME_RATE_MIN, XMF_RECORDER_FRAME_RATE_MAX)
		/ (double) 10    /* translate quality to [%] */
		/ (double) 1024  /* translate bit-rate to [kbps] */
		/ (double) 18.75 /* linear scale factor */;
	return (uint32_t) clamp(dResult, BIT_RATE_MIN, BIT_RATE_MAX);
}

uint32_t XMF_API XmfRecorder_GetTimeout(XmfRecorder* ctx)
{
	int64_t timeout = 0;

	if (ctx && ctx->enabled && ctx->lastUpdateTime > 0)
	{
		timeout = (1000 / ctx->frameRateMin) - (XmfTime_Get() - ctx->lastUpdateTime);

		if (timeout < 0)
			timeout = 0;
	}

	return (uint32_t) timeout;
}

void XMF_API XmfRecorder_Timeout(XmfRecorder* ctx)
{
	if (!ctx || !ctx->enabled)
		return;

	if (ctx->webm)
	{
		XmfWebM_Encode(ctx->webm, NULL, 0, 0, 0, 0);
		ctx->lastUpdateTime = XmfTime_Get();
	}
}

int XMF_API XmfRecorder_Update(XmfRecorder* ctx, uint8_t* frameData, uint32_t frameStep, uint32_t frameWidth,
				uint32_t frameHeight, uint32_t updateX, uint32_t updateY, uint32_t updateWidth,
				uint32_t updateHeight)
{
	if (!ctx->enabled)
	{
		if (ctx->initialized)
			XmfRecorder_Uninit(ctx);

		return 1;
	}

	if (ctx->initialized && (frameWidth != ctx->frameWidth || frameHeight != ctx->frameHeight))
	{
		XmfRecorder_Uninit(ctx);
	}

	if (!ctx->initialized)
	{
		XmfRecorder_SetFrameSize(ctx, frameWidth, frameHeight);

		if (!XmfRecorder_Init(ctx))
			return -1;
	}

	if (ctx->webm)
	{
		XmfWebM_Encode(ctx->webm, frameData, 0, 0, frameWidth, frameHeight);
		ctx->lastUpdateTime = XmfTime_Get();
	}

	return 1;
}

void XMF_API XmfRecorder_UpdateFrame(XmfRecorder* ctx, uint8_t* buffer, uint32_t updateX, uint32_t updateY,
                                 uint32_t updateWidth, uint32_t updateHeight, uint32_t surfaceStep)
{
	XmfRecorder_Update(ctx, buffer, surfaceStep, ctx->frameWidth, ctx->frameHeight, updateX,
	                   updateY, updateWidth, updateHeight);
}


void XMF_API XmfRecorder_SetMinimumFrameRate(XmfRecorder* ctx, uint32_t frameRate)
{
	ctx->frameRateMin = clamp(frameRate, XMF_RECORDER_FRAME_RATE_MIN, XMF_RECORDER_FRAME_RATE_MAX);
}

void XMF_API XmfRecorder_SetFrameRate(XmfRecorder* ctx, uint32_t frameRate)
{
	ctx->frameRate = clamp(frameRate, XMF_RECORDER_FRAME_RATE_MIN, XMF_RECORDER_FRAME_RATE_MAX);
}

void XMF_API XmfRecorder_SetFrameSize(XmfRecorder* ctx, uint32_t frameWidth, uint32_t frameHeight)
{
	ctx->frameWidth = frameWidth;
	ctx->frameHeight = frameHeight;
}

void XMF_API XmfRecorder_SetVideoQuality(XmfRecorder* ctx, uint32_t videoQuality)
{
	ctx->videoQuality = clamp(videoQuality, XMF_RECORDER_QUALITY_MIN, XMF_RECORDER_QUALITY_MAX);
}

void XMF_API XmfRecorder_SetFilename(XmfRecorder* ctx, const char* filename)
{
	strncpy(ctx->filename, filename, sizeof(ctx->filename) - 1);
}

void XMF_API XmfRecorder_SetDirectory(XmfRecorder* ctx, const char* directory)
{
	if (!ctx || !directory)
		return;

	strncpy(ctx->directory, directory, sizeof(ctx->directory) - 1);
}

bool XMF_API XmfRecorder_IsEnabled(XmfRecorder* ctx)
{
	if (!ctx)
		return false;

	return ctx->enabled;
}

void XMF_API XmfRecorder_SetEnabled(XmfRecorder* ctx, bool enabled)
{
	if (ctx)
		ctx->enabled = enabled;
}

size_t XMF_API XmfRecorder_GetPath(XmfRecorder* ctx, char* path, size_t size)
{
	if (!ctx)
		return 0;

	size_t path_len = strlen(ctx->directory);

	if (size > path_len)
		strncpy(path, ctx->directory, path_len);

	return path_len + 1;
}

bool XMF_API XmfRecorder_Init(XmfRecorder* ctx)
{
	uint32_t targetBitRate;
	char filename[XMF_MAX_PATH];

	if (ctx->initialized)
		return true;

	if (!XmfRecorder_GenerateFileName(ctx->filename, ctx->directory, ctx->fileNumber, filename, sizeof(filename)))
		return false;

	ctx->webm = XmfWebM_New();

	if (!ctx->webm)
		return false;

	targetBitRate = XmfRecorder_CalculateBitRate(ctx->frameWidth, ctx->frameHeight, ctx->frameRate, ctx->videoQuality);

	if (!XmfWebM_Init(ctx->webm, ctx->frameWidth, ctx->frameHeight, ctx->frameRate, targetBitRate, filename))
		goto error;

	ctx->initialized = true;

	return true;

error:
	if (ctx->webm)
	{
		XmfWebM_Free(ctx->webm);
		ctx->webm = NULL;
	}

	return false;
}

void XMF_API XmfRecorder_Uninit(XmfRecorder* ctx)
{
	if (!ctx->initialized)
		return;

	if (ctx->webm)
	{
		if (ctx->initialized)
			XmfWebM_Finalize(ctx->webm);

		XmfWebM_Free(ctx->webm);
		ctx->webm = NULL;
	}

	ctx->initialized = false;
	ctx->lastUpdateTime = 0;
	ctx->fileNumber++;
}

XmfRecorder* XMF_API XmfRecorder_New()
{
	XmfRecorder* ctx;

	ctx = (XmfRecorder*) calloc(1, sizeof(XmfRecorder));

	if (!ctx)
		return NULL;

	ctx->frameRateMin = 5;
	ctx->frameRate = 24;
	ctx->fileNumber = 1;
    ctx->videoQuality = 5;
    ctx->enabled = true;

	return ctx;
}

void XMF_API XmfRecorder_Free(XmfRecorder* ctx)
{
	if (!ctx)
		return;

	XmfRecorder_Uninit(ctx);

	free(ctx);
}
