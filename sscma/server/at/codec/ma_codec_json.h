#ifndef _MA_CODEC_JSON_H_
#define _MA_CODEC_JSON_H_

#include <cJSON.h>

#include "core/ma_common.h"
#include "porting/ma_osal.h"

#include "ma_codec_base.h"

namespace ma {

class EncoderJSON final : public Encoder {

public:
    EncoderJSON();
    ~EncoderJSON();

    operator bool() const override;

    ma_err_t begin() override;
    ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name) override;
    ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name, const std::string& data) override;
    ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name, uint64_t data) override;
    ma_err_t end() override;
    ma_err_t reset() override;

    ma_err_t remove(const std::string& key) override;

    ma_err_t write(const std::string& key, const char* buffer, size_t size) override;


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
    ma_err_t write(const std::string& key, ma_model_t value) override;
    ma_err_t write(ma_perf_t value) override;

    ma_err_t write(const std::forward_list<ma_class_t>& value) override;
    ma_err_t write(const std::forward_list<ma_point_t>& value) override;
    ma_err_t write(const std::forward_list<ma_bbox_t>& value) override;
    ma_err_t write(const std::forward_list<ma_keypoint3f_t>& value) override;

    ma_err_t write(const std::vector<ma_model_t>& value) override;


    ma_err_t write(const std::vector<Sensor*>& value) override;
    ma_err_t write(const Sensor* value, size_t preset) override;


    ma_err_t write(const in4_info_t& value) override;
    ma_err_t write(const in6_info_t& value) override;
    ma_err_t write(const ma_wifi_config_t& value, int* stat = nullptr) override;

    ma_err_t write(const ma_mqtt_config_t& value, int* stat = nullptr) override;

    ma_err_t write(const ma_mqtt_topic_config_t& value) override;


    ma_err_t write(int algo_id, int cat, int input_from, int tscore, int tiou) override;


    const std::string& toString() const override;
    const void* data() const override;
    const size_t size() const override;

private:
    cJSON* m_root;
    cJSON* m_data;
    Mutex m_mutex;
    mutable std::string m_string;
};

class DecoderJSON final : public Decoder {

public:
    DecoderJSON();
    ~DecoderJSON();

    operator bool() const override;

    ma_err_t begin(const void* data, size_t size) override;
    ma_err_t begin(const std::string& str) override;
    ma_err_t end() override;

    ma_msg_type_t type() const override;
    ma_err_t code() const override;
    std::string name() const override;

    ma_err_t read(const std::string& key, int8_t& value) const override;
    ma_err_t read(const std::string& key, int16_t& value) const override;
    ma_err_t read(const std::string& key, int32_t& value) const override;
    ma_err_t read(const std::string& key, int64_t& value) const override;
    ma_err_t read(const std::string& key, uint8_t& value) const override;
    ma_err_t read(const std::string& key, uint16_t& value) const override;
    ma_err_t read(const std::string& key, uint32_t& value) const override;
    ma_err_t read(const std::string& key, uint64_t& value) const override;
    ma_err_t read(const std::string& key, float& value) const override;
    ma_err_t read(const std::string& key, double& value) const override;
    ma_err_t read(const std::string& key, std::string& value) const override;
    ma_err_t read(ma_perf_t& value) override;
    ma_err_t read(std::forward_list<ma_class_t>& value) override;
    ma_err_t read(std::forward_list<ma_point_t>& value) override;
    ma_err_t read(std::forward_list<ma_bbox_t>& value) override;

private:
    std::string m_string;
    cJSON* m_root;
    cJSON* m_data;
    ma_msg_type_t m_type;
    ma_err_t m_code;
    std::string m_name;
    Mutex m_mutex;
};


}  // namespace ma

#endif  // _MA_CODEC_JSON_H_