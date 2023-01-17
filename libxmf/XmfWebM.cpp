
#include "XmfWebM.h"

#include <xpp/color.h>

#include <time.h>

#include <vpx/vp8cx.h>
#include <vpx/vpx_encoder.h>

#include <mkvmuxer/mkvmuxer.h>
#include <mkvmuxer/mkvmuxerutil.h>
#include <mkvmuxer/mkvwriter.h>

#include "XmfFile.h"
#include "XmfMath.h"
#include "XmfTime.h"
#include "XmfString.h"
#include "XmfMkvWriter.h"

typedef enum stereo_format
{
    STEREO_FORMAT_MONO = 0,
    STEREO_FORMAT_LEFT_RIGHT = 1,
    STEREO_FORMAT_BOTTOM_TOP = 2,
    STEREO_FORMAT_TOP_BOTTOM = 3,
    STEREO_FORMAT_RIGHT_LEFT = 11
} stereo_format_t;

struct xmf_webm
{
    char filename[XMF_MAX_PATH];
    uint32_t frame_rate;
    uint64_t frame_count;
    uint64_t frame_time;
    uint64_t first_encode_time;
    uint64_t last_encode_time;
    vpx_codec_pts_t pts;
    vpx_image_t* img;
    vpx_codec_ctx_t codec;
    vpx_codec_enc_cfg_t cfg;
    FILE* fp;
    XmfBipBuffer* bb;
    XmfTimeSource ts;
    int64_t last_pts_ns;
    bool pending_frame;
    void* mkv_writer;
    void* segment;
};

static const int kVideoTrackNumber = 1;
static const int kKeyFrameInterval = 15;

static const char* XmfWebM_CodecIdFromFourCC(uint32_t fourcc)
{
    const char* codec_id = NULL;

    switch (fourcc) 
    {
        case VP8_FOURCC:
            codec_id = "V_VP8";
            break;

        case VP9_FOURCC:
            codec_id = "V_VP9";
            break;
    }

    return codec_id;
}

/**
 * EBML Date: signed 8 octets integer in nanoseconds with 0 indicating
 * the precise beginning of the millennium (at 2001-01-01T00:00:00,000000000 UTC)
 */

static const int64_t NANO_MULTIPLIER = 1000000000;
static const int64_t EPOCH_MILLENIUM = 978307200L; // beginning of the millennium (at 2001-01-01T00:00:00,000000000 UTC)

static int64_t XmfWebM_GetEbmlDate()
{
    int64_t unix_time;
    int64_t ebml_time;

    unix_time = (int64_t) time(NULL); // number of seconds since Epoch (1970-01-01 00:00:00 +0000 UTC)
    ebml_time = ((unix_time - EPOCH_MILLENIUM) * NANO_MULTIPLIER); // change Epoch to millenium, convert to nanoseconds

    return ebml_time;
}

void XmfWebM_WriteFileHeader(XmfWebM* ctx,
                 const vpx_codec_enc_cfg_t* cfg,
                 stereo_format_t stereo_fmt, uint32_t fourcc,
                 const vpx_rational_t* par)
{
    int64_t date_utc;
    char writing_app[64];

    mkvmuxer::IMkvWriter* const mkv_writer =
        reinterpret_cast<mkvmuxer::IMkvWriter*>(ctx->mkv_writer);
    mkvmuxer::Segment* const segment =
        reinterpret_cast<mkvmuxer::Segment*>(ctx->segment);

    segment->Init(mkv_writer);
    segment->set_mode(mkvmuxer::Segment::kFile);
    segment->OutputCues(true);

    mkvmuxer::SegmentInfo* const info = segment->GetSegmentInfo();

    /* timecode scale in nanoseconds (1000000 means all timecodes are in milliseconds) */
    const uint64_t kTimecodeScale = 1000000;
    info->set_timecode_scale(kTimecodeScale);

    sprintf_s(writing_app, sizeof(writing_app), "%s %s", "vpxenc", vpx_codec_version_str());

    info->set_writing_app(writing_app);

    date_utc = XmfWebM_GetEbmlDate();
    info->set_date_utc(date_utc);

    const uint64_t video_track_id =
        segment->AddVideoTrack(static_cast<int>(cfg->g_w),
                       static_cast<int>(cfg->g_h), kVideoTrackNumber);

    mkvmuxer::VideoTrack* const video_track = static_cast<mkvmuxer::VideoTrack*>(
        segment->GetTrackByNumber(video_track_id));

    video_track->SetStereoMode(stereo_fmt);

    const char* codec_id = XmfWebM_CodecIdFromFourCC(fourcc);
    video_track->set_codec_id(codec_id);

    if ((par->num > 1) || (par->den > 1)) 
    {
        const uint64_t display_width = static_cast<uint64_t>(
            ((cfg->g_w * par->num * 1.0) / par->den) + .5);

        video_track->set_display_width(display_width);
        video_track->set_display_height(cfg->g_h);
    }
}

void XmfWebM_WriteBlock(XmfWebM* ctx, const vpx_codec_enc_cfg_t* cfg, const vpx_codec_cx_pkt_t* pkt)
{
    mkvmuxer::Segment* const segment =
        reinterpret_cast<mkvmuxer::Segment*>(ctx->segment);

    int64_t pts_ns = pkt->data.frame.pts * 1000000;

    if (pts_ns <= ctx->last_pts_ns) 
    {
        pts_ns = ctx->last_pts_ns + 1000000;
    }

    ctx->last_pts_ns = pts_ns;

    segment->AddFrame(static_cast<uint8_t*>(pkt->data.frame.buf),
              pkt->data.frame.sz, kVideoTrackNumber, pts_ns,
              pkt->data.frame.flags & VPX_FRAME_IS_KEY);
}

void XmfWebM_WriteFileFooter(XmfWebM* ctx)
{
    mkvmuxer::MkvWriter* const writer =
        reinterpret_cast<mkvmuxer::MkvWriter*>(ctx->mkv_writer);
    mkvmuxer::Segment* const segment =
        reinterpret_cast<mkvmuxer::Segment*>(ctx->segment);
    segment->Finalize();
    delete segment;
    delete writer;
    ctx->mkv_writer = NULL;
    ctx->segment = NULL;
}

int XmfWebM_EncodeImage(XmfWebM* ctx, vpx_image_t* img, vpx_codec_pts_t start, uint64_t duration)
{
    vpx_codec_pts_t keyframe_every;
    vpx_codec_err_t res;
    vpx_codec_iter_t iter = NULL;
    const vpx_codec_cx_pkt_t* pkt = NULL;
    int flags = 0;
    int got_pkts = 0;

    keyframe_every = kKeyFrameInterval * ctx->frame_rate;

    if (ctx->frame_count % keyframe_every == 0)
        flags |= VPX_EFLAG_FORCE_KF;

    ctx->last_encode_time = XmfTimeSource_Get(&ctx->ts);

    res = vpx_codec_encode(&ctx->codec, img, start, duration, flags, VPX_DL_REALTIME);

    if (res != VPX_CODEC_OK)
        return -1;

    while ((pkt = vpx_codec_get_cx_data(&ctx->codec, &iter)) != NULL)
    {
        got_pkts = 1;

        if (pkt->kind == VPX_CODEC_CX_FRAME_PKT)
            XmfWebM_WriteBlock(ctx, &ctx->cfg, pkt);
    }

    ctx->pts += duration;
    ctx->frame_count++;

    return got_pkts;
}

int XmfWebM_EncodeInternal(XmfWebM* ctx, bool force)
{
    uint32_t ms_per_frame;
    uint64_t ms_since_last_encode;

    ms_per_frame = 1000 / ctx->frame_rate;
    ms_since_last_encode = XmfTimeSource_Get(&ctx->ts) - ctx->last_encode_time;

    if (!force && ms_since_last_encode < ms_per_frame)
        return 0;

    if (ctx->pts == 0)
        ms_since_last_encode = XmfTimeSource_Get(&ctx->ts) - ctx->frame_time;

    XmfWebM_EncodeImage(ctx, ctx->img, ctx->pts, ms_since_last_encode);

    return 1;
}

int XMF_API XmfWebM_Encode(XmfWebM* ctx, uint8_t* srcData, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    uint32_t step[3];

    if (!ctx->pending_frame)
    {
        ctx->first_encode_time = XmfTimeSource_Get(&ctx->ts);
        goto convert_frame;
    }

    XmfWebM_EncodeInternal(ctx, !srcData);

convert_frame:
    if (!srcData)
        return 0;

    step[0] = (uint32_t) ctx->img->stride[0];
    step[1] = (uint32_t) ctx->img->stride[1];
    step[2] = (uint32_t) ctx->img->stride[2];

    Xpp_RGBToYCbCr420_8u_P3AC4R(srcData, width * 4, ctx->img->planes, step, width, height);
    ctx->frame_time = XmfTimeSource_Get(&ctx->ts);
    ctx->pending_frame = true;

    return 1;
}

void XMF_API XmfWebM_Finalize(XmfWebM* ctx)
{
    vpx_ref_frame_t ref;

    ref.frame_type = VP8_LAST_FRAME;
    ref.img = *ctx->img;
    vpx_codec_control(&ctx->codec, VP8_SET_REFERENCE, &ref);

    XmfWebM_Encode(ctx, NULL, 0, 0, 0, 0);
    ctx->pending_frame = false;

    while (true)
    {
        if (XmfWebM_EncodeImage(ctx, NULL, -1, 1) != 1)
            break;
    }
}

bool XMF_API XmfWebM_Init(XmfWebM* ctx, uint32_t frameWidth, uint32_t frameHeight, uint32_t frameRate,
               uint32_t targetBitRate, const char* filename, XmfBipBuffer* bb, XmfTimeSource* ts)
{
    vpx_codec_err_t res;
    vpx_rational_t par = { 1, 1 };

    ctx->img = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, frameWidth, frameHeight, 16);

    if (!ctx->img)
        goto error;

    res = vpx_codec_enc_config_default(vpx_codec_vp8_cx(), &ctx->cfg, 0);

    if (res)
        goto error;

    ctx->frame_rate = frameRate;

    ctx->cfg.g_w = frameWidth;
    ctx->cfg.g_h = frameHeight;
    ctx->cfg.g_timebase.num = 1;
    ctx->cfg.g_timebase.den = 1000;
    ctx->cfg.rc_target_bitrate = targetBitRate;
    ctx->cfg.g_error_resilient = (vpx_codec_er_flags_t) VPX_ERROR_RESILIENT_DEFAULT;
    
    ctx->bb = bb;

    if (filename) {
        sprintf_s(ctx->filename, sizeof(ctx->filename) - 1, "%s", filename);
    }

    if (ts) {
        ctx->ts.func = ts->func;
        ctx->ts.param = ts->param;
    }

#if 0
    ctx->fp = XmfFile_Open(ctx->filename, "wb");

    if (!ctx->fp)
        goto error;

    ctx->mkv_writer = new mkvmuxer::MkvWriter(ctx->fp);
#else
    ctx->mkv_writer = XmfMkvWriter_New();

    if (ctx->bb) {
        XmfMkvWriter_SetBipBuffer((XmfMkvWriter*) ctx->mkv_writer, ctx->bb);
    } else {
        ctx->fp = XmfFile_Open(ctx->filename, "wb");

        if (!ctx->fp)
            goto error;

        XmfMkvWriter_SetFilePointer((XmfMkvWriter*) ctx->mkv_writer, ctx->fp);
    }
#endif

    ctx->segment = new mkvmuxer::Segment();

    if (!ctx->mkv_writer || !ctx->segment)
        goto error;

    if (vpx_codec_enc_init(&ctx->codec, vpx_codec_vp8_cx(), &ctx->cfg, 0))
        goto error;

    XmfWebM_WriteFileHeader(ctx, &ctx->cfg, STEREO_FORMAT_MONO, VP8_FOURCC, &par);

    return true;

error:
    XmfWebM_Uninit(ctx);
    return false;
}

uint64_t XMF_API XmfWebM_FrameCount(XmfWebM* ctx)
{
    if (!ctx)
        return 0;

    return ctx->frame_count;
}

uint64_t XMF_API XmfWebM_Duration(XmfWebM* ctx)
{
    if (!ctx)
        return 0;

    return ctx->last_encode_time - ctx->first_encode_time;
}

void XMF_API XmfWebM_Uninit(XmfWebM* ctx)
{
    if (!ctx)
        return;

    if (ctx->img) 
    {
        vpx_img_free(ctx->img);
        ctx->img = NULL;
    }

    vpx_codec_destroy(&ctx->codec);

    if (ctx->pts > 0)
        XmfWebM_WriteFileFooter(ctx);

    if (ctx->fp)
    {
        fclose(ctx->fp);
        ctx->fp = NULL;

        if (ctx->pts == 0)
            XmfFile_Delete(ctx->filename);
    }
}

XmfWebM* XMF_API XmfWebM_New()
{
    XmfWebM* ctx;

    ctx = (XmfWebM*) calloc(1, sizeof(XmfWebM));

    if (!ctx)
        return NULL;

    ctx->ts.func = XmfTimeSource_System;
    ctx->ts.param = NULL;

    return ctx;
}

void XMF_API XmfWebM_Free(XmfWebM* ctx)
{
    if (!ctx)
        return;

    XmfWebM_Uninit(ctx);
}
