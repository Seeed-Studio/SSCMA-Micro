#ifndef _MA_MODEL_FOMO_
#define _MA_MODEL_FOMO_

#include <vector>

#include "ma_model_detector.h"

namespace ma::model {

class FOMO : public Detector {
   protected:
    ma_err_t postprocess() override;

    ma_err_t postProcessI8();

   public:
    FOMO(Engine* engine);
    ~FOMO();

    static bool isValid(Engine* engine);

    static const char* getTag();
};

}  // namespace ma::model

#endif
