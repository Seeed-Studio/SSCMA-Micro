
#include "core/ma_common.h"

#include "porting/ma_osal.h"
#include "porting/ma_porting.h"
#include "porting/ma_storage.h"
#include "porting/ma_device.h"

#include "core/cv/ma_cv.h"
#include "core/codec/ma_codec.h"
#include "core/engine/ma_engine.h"
#include "core/model/ma_model.h"

#include "core/utils/ma_ringbuffer.hpp"

#include "server/ma_server.h"

#if __has_include(<ma_porting_extra.h>)
#include <ma_porting_extra.h>
#endif

using namespace ma;
