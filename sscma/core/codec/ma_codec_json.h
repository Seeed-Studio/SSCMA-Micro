#ifndef _MA_CODEC_JSON_H_
#define _MA_CODEC_JSON_H_

#include <cJSON.h>

#include "core/ma_common.h"
#include "porting/ma_osal.h"

#include "ma_codec_base.h"

namespace ma {

class CodecJSON final : public Codec {

public:
    CodecJSON();
    ~CodecJSON();

    operator bool() const override;

    ma_err_t begin() override;
    ma_err_t begin(ma_reply_t reply, ma_err_t code, const std::string& name) override;
    ma_err_t begin(ma_reply_t reply,
                   ma_err_t code,
                   const std::string& name,
                   const std::string& data) override;
    ma_err_t end() override;
    ma_err_t reset() override;

    ma_err_t remove(const std::string& key) override;

    ma_err_t write(const std::string& key, int8_t value) override;
    ma_err_t write(const std::string& key, int16_t value) override;
    ma_err_t write(const std::string& key, int32_t value) override;
    ma_err_t write(const std::string& key, int64_t value) override;
    ma_err_t write(const std::string& key, uint8_t value) override;
    ma_err_t write(const std::string& key, uint16_t value) override;
    ma_err_t write(const std::string& key, uint32_t value) override;
    ma_err_t write(const std::string& key, uint64_t value) override;
    ma_err_t write(const std::string& key, float value) override;
    ma_err_t write(const std::string& key, double value) override;
    ma_err_t write(const std::string& key, const std::string& value) override;
    ma_err_t write(ma_perf_t value) override;
    ma_err_t write(std::vector<ma_class_t>& value) override;
    ma_err_t write(std::vector<ma_point_t>& value) override;
    ma_err_t write(std::vector<ma_bbox_t>& value) override;

    const std::string& toString() const override;
    const void* data() const override;
    const size_t size() const override;

private:
    cJSON* m_root;
    cJSON* m_data;
    Mutex m_mutex;
    mutable std::string m_string;
};

}  // namespace ma

#endif  // _MA_CODEC_JSON_H_