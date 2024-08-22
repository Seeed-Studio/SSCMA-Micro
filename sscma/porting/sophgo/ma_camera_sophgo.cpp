// #include "ma_camera_sophgo.h"

// namespace ma {

// static const char* TAG = "ma::camera::sophgo";


// Mutex CameraSophgo::s_mutex(false);
// std::atomic<bool> CameraSophgo::s_opened                 = false;
// std::atomic<bool> CameraSophgo::s_streaming              = false;
// Camera* CameraSophgo::s_attach[CameraSophgo::ISP_CH_MAX] = {nullptr};

// CameraSophgo::CameraSophgo(uint8_t index) : Camera(index), m_msgBox(10) {
//     Guard guard(s_mutex);
//     app_ipcam_Param_Load();
//     if (s_opened.load()) {
//         MA_LOGE(TAG, "Camera is already opened, cannot attach new channel");
//         MA_ASSERT(0);
//         return;
//     }
//     if (index >= CameraSophgo::ISP_CH_MAX) {
//         MA_LOGE(TAG, "Index %d out of range", index);
//         MA_ASSERT(0);
//         return;
//     }
//     if (s_attach[index] != nullptr) {
//         MA_LOGE(TAG, "Channel %d is already attached", index);
//         MA_ASSERT(0);
//         return;
//     }
//     s_attach[index] = this;
// }

// CameraSophgo::~CameraSophgo() {
//     Guard guard(s_mutex);
//     close();
//     s_attach[m_index] = nullptr;
// }

// int CameraSophgo::callbackStub(void* pdata, void* pargs) {
//     ((CameraSophgo*)pargs)->callback(pdata);
//     return CVI_SUCCESS;
// }

// void CameraSophgo::callback(void* pdata) {
//     if (!m_streaming.load() && !m_capturing.load()) {
//         MA_LOGW(TAG, "%d callback but not streaming", m_index);
//         return;
//     }
//     if (m_format == MA_PIXEL_FORMAT_RGB888) {
//         video_frame_t* frame = (video_frame_t*)pdata;
//         ma_img_t* img        = static_cast<ma_img_t*>(ma_malloc(sizeof(ma_img_t)));
//         if (img == nullptr) {
//             MA_LOGW(TAG, "%d Discard frame", m_index);
//             return;
//         }
//         img->size = frame->size;
//         img->data = static_cast<uint8_t*>(ma_malloc(frame->size));
//         if (img->data == nullptr) {
//             MA_LOGW(TAG, "%d Discard frame", m_index);
//             ma_free(img);
//             return;
//         }
//         memcpy(img->data, frame->pdata, frame->size);
//         img->width     = frame->width;
//         img->height    = frame->height;
//         img->rotate    = MA_PIXEL_ROTATE_0;
//         img->timestamp = frame->timestamp;
//         img->format    = MA_PIXEL_FORMAT_RGB888;
//         if (!m_msgBox.post(img, Tick::fromMilliseconds(30))) {
//             MA_LOGW(TAG, "%d Discard frame", m_index);
//             ma_free(img->data);
//             ma_free(img);
//         }
//     } else if (m_format == MA_PIXEL_FORMAT_H264 || m_format == MA_PIXEL_FORMAT_H265) {
//         VENC_STREAM_S* stream = (VENC_STREAM_S*)pdata;
//         for (int i = 0; i < stream->u32PackCount; i++) {
//             VENC_PACK_S* pack = &stream->pstPack[i];
//             ma_img_t* img     = static_cast<ma_img_t*>(ma_malloc(sizeof(ma_img_t)));
//             if (img == nullptr) {
//                 MA_LOGW(TAG, "%d Discard frame", m_index);
//                 return;
//             }
//             img->size = pack->u32Len - pack->u32Offset;
//             img->data = static_cast<uint8_t*>(ma_malloc(img->size));
//             if (img->data == nullptr) {
//                 MA_LOGW(TAG, "%d Discard frame", m_index);
//                 ma_free(img);
//                 return;
//             }
//             memcpy(img->data, (uint8_t*)pack->pu8Addr + pack->u32Offset, img->size);
//             img->width     = m_width;
//             img->height    = m_height;
//             img->rotate    = MA_PIXEL_ROTATE_0;
//             img->timestamp = pack->u64PTS;
//             img->format    = m_format;
//             if (!m_msgBox.post(img, Tick::fromMilliseconds(30))) {
//                 MA_LOGW(TAG, "%d Discard frame", m_index);
//                 ma_free(img->data);
//                 ma_free(img);
//             }
//         }
//     }
//     return;
// }


// CameraSophgo::operator bool() const {
//     return true;
// }

// ma_err_t CameraSophgo::open(uint16_t width, uint16_t height, ma_pixel_format_t format, int fps) {

//     Guard guard(m_mutex);

//     if (m_opened.load()) {
//         return MA_OK;
//     }

//     if (m_index == ISP_CH0 && format != MA_PIXEL_FORMAT_H265) {
//         return MA_EINVAL;
//     }
//     if (m_index == ISP_CH1 && format != MA_PIXEL_FORMAT_H264) {
//         return MA_EINVAL;
//     }
//     if (m_index == ISP_CH2 && format != MA_PIXEL_FORMAT_RGB888) {
//         return MA_EINVAL;
//     }

//     Guard guard2(s_mutex);

//     m_width  = width;
//     m_height = height;
//     m_format = format;

//     APP_PARAM_SYS_CFG_S* sys   = app_ipcam_Sys_Param_Get();
//     APP_PARAM_VPSS_CFG_T* vpss = app_ipcam_Vpss_Param_Get();
//     APP_PARAM_VENC_CTX_S* venc = app_ipcam_Venc_Param_Get();

//     APP_PARAM_VB_CFG_S* vb = &sys->vb_pool[m_index];
//     vb->bEnable            = CVI_TRUE;
//     vb->width              = m_width;
//     vb->height             = m_height;
//     vb->fmt = (m_format == MA_PIXEL_FORMAT_RGB888) ? PIXEL_FORMAT_RGB_888 : PIXEL_FORMAT_NV21;

//     VPSS_CHN_ATTR_S* vpss_chn = &vpss->astVpssGrpCfg[0].astVpssChnAttr[m_index];
//     vpss_chn->u32Width        = m_width;
//     vpss_chn->u32Height       = m_height;
//     vpss_chn->enPixelFormat =
//         (m_format == MA_PIXEL_FORMAT_RGB888) ? PIXEL_FORMAT_RGB_888 : PIXEL_FORMAT_NV21;

//     APP_VENC_CHN_CFG_S* venc_chn = &venc->astVencChnCfg[m_index];
//     venc_chn->u32Width           = m_width;
//     venc_chn->u32Height          = m_height;
//     if (fps != -1) {
//         venc_chn->u32DstFrameRate = fps;
//     }
//     switch (m_format) {
//         case MA_PIXEL_FORMAT_RGB888:
//             venc_chn->enType = PT_JPEG;
//             break;
//         case MA_PIXEL_FORMAT_H264:
//             venc_chn->enType = PT_H264;
//             break;
//         case MA_PIXEL_FORMAT_H265:
//             venc_chn->enType = PT_H265;
//             break;
//         default:
//             break;
//     }

//     MA_LOGD(TAG,
//             "Open channel %d @ %p width %d height %d format %d",
//             m_index,
//             this,
//             m_width,
//             m_height,
//             m_format);

//     app_ipcam_Venc_Consumes(m_index, 0, callbackStub, this);
//     m_opened.store(true);

//     if (s_opened.load() == false) {
//         for (uint8_t i = 0; i < ISP_CH_MAX; i++) {
//             if (s_attach[i] != nullptr && s_attach[i]->isOpened() == false) {
//                 return MA_OK;
//             } else {
//                 MA_LOGW(TAG, "Channel %d @ %p is opened", i, s_attach[i]);
//             }
//         }
//         MA_LOGD(TAG, "Open Camera after all channels are opened");
//         app_ipcam_Sys_Init();
//         app_ipcam_Vi_Init();
//         app_ipcam_Vpss_Init();
//         app_ipcam_Venc_Init(APP_VENC_ALL);
//         s_opened.store(true);
//     }

//     return MA_OK;
// }


// ma_err_t CameraSophgo::close() {
//     Guard guard(m_mutex);
//     Guard guard2(s_mutex);
//     if (m_opened.load() == false) {
//         return MA_OK;
//     }
//     MA_LOGD(TAG, "Close channel %d @ %p", m_index, this);
//     stop();
//     if (s_opened.load() == true) {
//         for (uint8_t i = 0; i < ISP_CH_MAX; i++) {
//             if (s_attach[i] != nullptr && s_attach[i]->isOpened() == true) {
//                 return MA_OK;
//             }
//         }
//         s_opened.store(false);
//         MA_LOGD(TAG, "Close Camera after all channels are closed");
//         app_ipcam_Vpss_DeInit();
//         app_ipcam_Vi_DeInit();
//         app_ipcam_Sys_DeInit();
//     }
//     return MA_OK;
// }

// ma_err_t CameraSophgo::start() {
//     Guard guard(m_mutex);

//     if (m_opened.load() == false) {
//         return MA_EINVAL;
//     }

//     for (uint8_t i = 0; i < ISP_CH_MAX; i++) {
//         if (s_attach[i] != nullptr && s_attach[i]->isOpened() == false) {
//             MA_LOGW(TAG, "Channel %d has been opened, please open it first", i);
//             return MA_AGAIN;
//         }
//     }

//     m_streaming.store(true);

//     if (!s_streaming.load()) {
//         Guard guard(s_mutex);
//         for (uint8_t i = 0; i < ISP_CH_MAX; i++) {
//             if (s_attach[i] != nullptr && s_attach[i]->isStreaming() == false) {
//                 return MA_OK;
//             }
//         }
//         MA_LOGD(TAG, "Start all channels after all channels are started");
//         app_ipcam_Venc_Start(APP_VENC_ALL);
//         s_streaming.store(true);
//     }

//     return MA_OK;
// }

// ma_err_t CameraSophgo::stop() {
//     Guard guard(m_mutex);
//     if (m_opened.load() == false) {
//         return MA_EINVAL;
//     }
//     if (m_streaming.load() == false) {
//         return MA_OK;
//     }

//     MA_LOGD(TAG, "Stop channel %d @ %p", m_index, this);

//     m_streaming.store(false);

//     if (s_streaming.load() == true) {
//         for (uint8_t i = 0; i < ISP_CH_MAX; i++) {
//             if (s_attach[i] != nullptr && s_attach[i]->isStreaming() == false) {
//                 return MA_OK;
//             }
//         }
//         MA_LOGD(TAG, "Stop all channels after all channels are stopped");
//         app_ipcam_Venc_Stop(APP_VENC_ALL);
//         s_streaming.store(false);
//     }

//     return MA_OK;
// }


// ma_err_t CameraSophgo::capture() {
//     // TODO: capture
//     // Guard guard(m_mutex);
//     // if (m_opened.load() == false) {
//     //     return MA_EINVAL;
//     // }
//     // if (m_streaming.load()) {
//     //     m_capturing.store(true);
//     //     return MA_OK;
//     // }
//     // app_ipcam_Venc_Start(static_cast<APP_VENC_CHN_E>(1 << m_index));

//     return MA_ENOTSUP;
// }


// ma_err_t CameraSophgo::read(ma_img_t* img, ma_tick_t timeout) {
//     // TODO: read
//     Guard guard(m_mutex);
//     if (m_streaming.load() == false && m_capturing.load() == false) {
//         return MA_EINVAL;
//     }
//     ma_img_t* tmp = nullptr;
//     if (m_msgBox.fetch(reinterpret_cast<void**>(&tmp), timeout) == false) {
//         memset(img, 0, sizeof(ma_img_t));
//         return MA_ETIMEOUT;
//     }
//     *img = *tmp;
//     ma_free(tmp);
//     return MA_OK;
// }

// ma_err_t CameraSophgo::release(ma_img_t* img) {
//     // TODO: release
//     if (!m_opened.load()) {
//         return MA_EINVAL;
//     }
//     if (img->data) {
//         free(img->data);
//     }
//     return MA_OK;
// }

// }  // namespace ma