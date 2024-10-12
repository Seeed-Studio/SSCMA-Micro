#ifndef _MA_EXCEPTION_H_
#define _MA_EXCEPTION_H_

#ifdef __cplusplus
#include <string>
#endif

#include "ma_config_internal.h"
#include "ma_types.h"

#ifdef __cplusplus

namespace ma {

#if MA_USE_EXCEPTION
class Exception : public std::exception {
private:
    ma_err_t err_;
    std::string msg_;

public:
    Exception(ma_err_t err, const std::string& msg) : err_(err), msg_(msg) {}
    ma_err_t err() const {
        return err_;
    }
    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

#define MA_TRY      try
#define MA_THROW(e) throw e
#define MA_CATCH(x) catch (x)
#else
class Exception {
private:
    ma_err_t err_;
    std::string msg_;

public:
    Exception(ma_err_t err, const std::string& msg) : err_(err), msg_(msg) {}
    ma_err_t err() const {
        return err_;
    }
    const char* what() const noexcept {
        return msg_.c_str();
    }
};

#define MA_TRY if (true)
#define MA_THROW(e)
#define MA_CATCH(x) if (e.err() != MA_OK)
#endif
}  // namespace ma

#endif

#endif  // _MA_EXCEPTION_H_