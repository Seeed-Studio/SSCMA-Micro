#include "ma_camera_sg200x.h"


namespace ma {

static const char* TAG = "ma::camera::sg200x";

struct presets_wrapper_t {
    const char* description;
    uint16_t width;
    uint16_t height;
    int fps;
};

static const presets_wrapper_t _presets[] = {
    {"1920x1080 @ 30fps", 1920, 1080, 30},
    {"1920x1080 @ 15fps", 1920, 1080, 15},
    {"1920x1080 @ 5fps", 1920, 1080, 5},
    {"1280x720 @ 30fps", 1280, 720, 30},
    {"1280x720 @ 15fps", 1280, 720, 15},
    {"1280x720 @ 5fps", 1280, 720, 5},
};


#define CAMERA_INIT()                               \
    {                                               \
        Thread::enterCritical();                    \
        Thread::sleep(Tick::fromMilliseconds(100)); \
        MA_LOGD(TAG, "start video");                \
        startVideo();                               \
        Thread::sleep(Tick::fromSeconds(1));        \
        Thread::exitCritical();                     \
    }

#define CAMERA_DEINIT()                             \
    {                                               \
        Thread::enterCritical();                    \
        MA_LOGD(TAG, "deinit video");               \
        Thread::sleep(Tick::fromMilliseconds(100)); \
        deinitVideo();                              \
        Thread::sleep(Tick::fromSeconds(1));        \
        Thread::exitCritical();                     \
    }


int CameraSG200X::vencCallback(void* pData, void* pArgs) {

    APP_DATA_CTX_S* pstDataCtx        = (APP_DATA_CTX_S*)pArgs;
    APP_DATA_PARAM_S* pstDataParam    = &pstDataCtx->stDataParam;
    APP_VENC_CHN_CFG_S* pstVencChnCfg = (APP_VENC_CHN_CFG_S*)pstDataParam->pParam;
    VENC_CHN VencChn                  = pstVencChnCfg->VencChn;

    if (!m_streaming) {
        return CVI_SUCCESS;
    }

    VENC_STREAM_S* pstStream = (VENC_STREAM_S*)pData;
    VENC_PACK_S* ppack;

    for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
        ppack           = &pstStream->pstPack[i];
        ma_img_t* frame = new ma_img_t;
        frame->data     = new uint8_t[ppack->u32Len - ppack->u32Offset];
        memcpy(frame->data, ppack->pu8Addr + ppack->u32Offset, ppack->u32Len - ppack->u32Offset);
        frame->size      = ppack->u32Len - ppack->u32Offset;
        frame->width     = m_channels[VencChn].width;
        frame->height    = m_channels[VencChn].height;
        frame->format    = m_channels[VencChn].format;
        frame->timestamp = Tick::current();
        frame->count     = pstStream->u32PackCount;
        frame->index     = i;
        frame->physical  = false;
        if (m_channels[VencChn].format == MA_PIXEL_FORMAT_H264) {
            switch (ppack->DataType.enH264EType) {
                case H264E_NALU_ISLICE:
                case H264E_NALU_SPS:
                case H264E_NALU_IDRSLICE:
                case H264E_NALU_SEI:
                case H264E_NALU_PPS:
                    frame->key = true;
                    break;
                default:
                    frame->key = false;
                    break;
            }
        }
        if (!m_channels[VencChn].queue->post(frame, Tick::fromMilliseconds(1000 / m_channels[VencChn].fps))) {
            delete[] frame->data;
            delete frame;
        }
    }
    return CVI_SUCCESS;
}

int CameraSG200X::vpssCallback(void* pData, void* pArgs) {

    APP_VENC_CHN_CFG_S* pstVencChnCfg = (APP_VENC_CHN_CFG_S*)pArgs;
    VIDEO_FRAME_INFO_S* VpssFrame     = (VIDEO_FRAME_INFO_S*)pData;
    VIDEO_FRAME_S* f                  = &VpssFrame->stVFrame;


    if (!m_streaming) {
        return CVI_SUCCESS;
    }
    ma_img_t* frame  = new ma_img_t;
    frame->physical  = true;
    frame->data      = reinterpret_cast<uint8_t*>(f->u64PhyAddr);
    frame->size      = f->u32Length[0] + f->u32Length[1] + f->u32Length[2];
    frame->width     = m_channels[pstVencChnCfg->VencChn].width;
    frame->height    = m_channels[pstVencChnCfg->VencChn].height;
    frame->format    = m_channels[pstVencChnCfg->VencChn].format;
    frame->timestamp = Tick::current();
    frame->count     = 1;
    frame->index     = 1;
    if (m_channels[pstVencChnCfg->VencChn].format == MA_PIXEL_FORMAT_H264) {
        frame->key = true;
    }
    if (!m_channels[pstVencChnCfg->VencChn].queue->post(frame, Tick::fromMilliseconds(1000 / m_channels[pstVencChnCfg->VencChn].fps))) {
        m_channels[pstVencChnCfg->VencChn].sem.signal();
        delete frame;
    }

    if (m_channels[pstVencChnCfg->VencChn].sem.wait(Tick::fromMilliseconds(2000)) == false) {
        delete frame;
    }

    return CVI_SUCCESS;
}

int CameraSG200X::vencCallbackStub(void* pData, void* pArgs, void* pUserData) {
    return reinterpret_cast<CameraSG200X*>(pUserData)->vencCallback(pData, pArgs);
}

int CameraSG200X::vpssCallbackStub(void* pData, void* pArgs, void* pUserData) {
    return reinterpret_cast<CameraSG200X*>(pUserData)->vpssCallback(pData, pArgs);
}

CameraSG200X::CameraSG200X(size_t id) : Camera(id) {
    for (const auto& preset : _presets) {
        m_presets.push_back({.description = preset.description});
    }
    m_presets.shrink_to_fit();

    app_ipcam_Param_Load();

    for (int i = 0; i < CHN_MAX; i++) {
        m_channels[i].configured = false;
        m_channels[i].enabled    = false;
        m_channels[i].fps        = 30;
    }
    m_channels[CHN_RAW].format  = MA_PIXEL_FORMAT_RGB888;
    m_channels[CHN_JPEG].format = MA_PIXEL_FORMAT_JPEG;
    m_channels[CHN_H264].format = MA_PIXEL_FORMAT_H264;
}


CameraSG200X::~CameraSG200X() {
    if (m_initialized) {
        deInit();
    }
}

ma_err_t CameraSG200X::init(size_t preset_idx) noexcept {
    if (m_initialized) [[unlikely]] {
        return MA_EINVAL;
    }

    if (preset_idx >= m_presets.size()) [[unlikely]] {
        return MA_EINVAL;
    }

    m_channels[CHN_H264].width      = _presets[preset_idx].width;
    m_channels[CHN_H264].height     = _presets[preset_idx].height;
    m_channels[CHN_H264].fps        = _presets[preset_idx].fps;
    m_channels[CHN_H264].format     = MA_PIXEL_FORMAT_H264;
    m_channels[CHN_H264].configured = true;

    m_initialized = true;

    return MA_OK;
}

void CameraSG200X::deInit() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }
    if (m_streaming) [[unlikely]] {
        stopStream();
    }

    m_initialized = false;
}

ma_err_t CameraSG200X::startStream(StreamMode mode) noexcept {
    if (!m_initialized) [[unlikely]] {
        return MA_EINVAL;
    }
    if (m_streaming) [[unlikely]] {
        return MA_OK;
    }
    MA_LOGD(TAG, "CameraSG200X::startStream: %d", m_id);
    for (int i = 0; i < CHN_MAX; i++) {
        if (m_channels[i].enabled && !m_channels[i].configured) {
            MA_LOGW(TAG, "Channel %d is not configured", i);
            return MA_AGAIN;
        }
    }
    for (int i = 0; i < CHN_MAX; i++) {
        if (m_channels[i].enabled) {
            video_ch_param_t param;
            param.width  = m_channels[i].width;
            param.height = m_channels[i].height;
            param.fps    = m_channels[i].fps;

            switch (m_channels[i].format) {
                case MA_PIXEL_FORMAT_JPEG:
                    param.format = VIDEO_FORMAT_JPEG;
                    break;
                case MA_PIXEL_FORMAT_H264:
                    param.format = VIDEO_FORMAT_H264;
                    break;
                case MA_PIXEL_FORMAT_H265:
                    param.format = VIDEO_FORMAT_H265;
                    break;
                case MA_PIXEL_FORMAT_RGB888:
                    param.format = VIDEO_FORMAT_RGB888;
                    break;
                case MA_PIXEL_FORMAT_YUV422:
                    param.format = VIDEO_FORMAT_NV21;
                    break;
                default:
                    return MA_ENOTSUP;
                    break;
            }
            MA_LOGD(TAG, "width: %d, height: %d, fps: %d format: %d", param.width, param.height, param.fps, param.format);
            setupVideo(static_cast<video_ch_index_t>(i), &param);

            if (m_channels[i].queue != nullptr) {
                delete m_channels[i].queue;
            }
            m_channels[i].queue = new MessageBox(param.fps);

            if (param.format == VIDEO_FORMAT_RGB888 || param.format == VIDEO_FORMAT_NV21) {
                registerVideoFrameHandler(static_cast<video_ch_index_t>(i), 0, vpssCallbackStub, this);
            } else {
                registerVideoFrameHandler(static_cast<video_ch_index_t>(i), 0, vencCallbackStub, this);
            }
        }
    }
    CAMERA_INIT();
    m_streaming = true;
    return MA_OK;
}

void CameraSG200X::stopStream() noexcept {
    if (!m_initialized) [[unlikely]] {
        return;
    }
    if (!m_streaming) [[unlikely]] {
        return;
    }
    MA_LOGD(TAG, "CameraSG200X::stopStream: %d", m_id);
    m_streaming = false;
    CAMERA_DEINIT();
}

ma_err_t CameraSG200X::commandCtrl(CtrlType ctrl, CtrlMode mode, CtrlValue& value) noexcept {
    if (!m_initialized) [[unlikely]] {
        return MA_EPERM;
    }
    switch (ctrl) {
        case kChannel:
            if (mode == kWrite) {
                if (value.i32 < 0 || value.i32 >= CHN_MAX) {
                    MA_LOGD(TAG, "Invalid channel: %d", value.i32);
                    return MA_EINVAL;
                }
                MA_LOGD(TAG, "%d kChannel: %d", chn, value.i32);
                chn                     = value.i32;
                m_channels[chn].enabled = true;
            } else {
                value.i32 = chn;
            }
            break;
        case kWindow:
            if (mode == kWrite) {
                MA_LOGD(TAG, "%d kWindow: %d, %d", chn, value.u16s[0], value.u16s[1]);
                m_channels[chn].width      = value.u16s[0];
                m_channels[chn].height     = value.u16s[1];
                m_channels[chn].configured = true;
            } else if (mode == kRead) {
                value.u16s[0] = m_channels[chn].width;
                value.u16s[1] = m_channels[chn].height;
            }
            break;
        case kFormat:
            if (mode == kWrite) {
                MA_LOGD(TAG, "%d kFormat: %d", chn, value.i32);
                m_channels[chn].format = static_cast<ma_pixel_format_t>(value.i32);
            } else if (mode == kRead) {
                value.i32 = m_channels[chn].format;
            }
            break;
        case kFps:
            if (mode == kWrite) {
                MA_LOGD(TAG, "%d kFps: %d", chn, value.i32);
                m_channels[chn].fps = value.i32;
            } else if (mode == kRead) {
                value.i32 = m_channels[chn].fps;
            }
            break;
        default:
            return MA_ENOTSUP;
            break;
    }
    return MA_OK;
}

ma_err_t CameraSG200X::retrieveFrame(ma_img_t& frame, ma_pixel_format_t format) noexcept {
    if (!m_streaming) [[unlikely]] {
        return MA_EPERM;
    }
    int chn = 0;
    switch (format) {
        case MA_PIXEL_FORMAT_JPEG:
            chn = 1;
            break;
        case MA_PIXEL_FORMAT_H264:
            chn = 2;
            break;
        default:
            break;
    }

    if (!m_channels[chn].enabled) [[unlikely]] {
        return MA_EPERM;
    }

    ma_img_t* img = nullptr;
    if (!m_channels[chn].queue->fetch(reinterpret_cast<void**>(&img), Tick::fromMilliseconds(1000 / m_channels[chn].fps))) {
        return MA_AGAIN;
    }

    if (img != nullptr) {
        frame = *img;
        delete img;
    }

    return MA_OK;
}

void CameraSG200X::returnFrame(ma_img_t& frame) noexcept {
    if (!frame.physical) {
        delete[] frame.data;
    } else {
        int chn = 0;
        switch (frame.format) {
            case MA_PIXEL_FORMAT_JPEG:
                chn = 1;
                break;
            case MA_PIXEL_FORMAT_H264:
                chn = 2;
                break;
            default:
                break;
        }
        m_channels[chn].sem.signal();
    }
}

}  // namespace ma