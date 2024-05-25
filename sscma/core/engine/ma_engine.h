#ifndef _MA_ENGINE_H_
#define _MA_ENGINE_H_

#include "core/engine/ma_engine_base.h"

#ifdef MA_USE_ENGINE_TFLITE
#include "core/engine/ma_engine_tflite.h"
using Engine = ma::engine::EngineTFLite;
#endif

#ifdef MA_USE_ENGINE_CVINN
#include "core/engine/ma_engine_cvinn.h"
#define ENGINE ma::engine::EngineCVInn
using Engine = ma::engine::EngineCVInn;
#endif


#endif  // _MA_ENGINE_H_