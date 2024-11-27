#ifndef _MA_ENGINE_H_
#define _MA_ENGINE_H_

#include "ma_engine_base.h"

#ifdef MA_USE_ENGINE_TFLITE
#include "ma_engine_tflite.h"
using EngineDefault = ma::engine::EngineTFLite;
#endif

#ifdef MA_USE_ENGINE_CVI
#include "ma_engine_cvi.h"
using EngineDefault = ma::engine::EngineCVI;
#endif

#ifdef MA_USE_ENGINE_HAILO
#include "ma_engine_hailo.h"
using EngineDefault = ma::engine::EngineHailo;
#endif

#endif  // _MA_ENGINE_H_