#ifndef _MA_ENGINE_H_
#define _MA_ENGINE_H_

#include "ma_engine_base.h"

#ifdef MA_USE_ENGINE_TFLITE
#include "ma_engine_tflite.h"
#endif

#ifdef MA_USE_ENGINE_CVI
#include "ma_engine_cvi.h"
#endif

#endif  // _MA_ENGINE_H_