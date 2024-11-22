#ifndef _MA_CODEC_BASE_H_
#define _MA_CODEC_BASE_H_


#include <forward_list>
#include <string>
#include <vector>

#include "core/ma_common.h"
#include "porting/ma_sensor.h"

namespace ma {

class Encoder {
public:
    Encoder()          = default;
    virtual ~Encoder() = default;

    /*!
     * @brief Encoder is valid.
     *
     * @retval true if valid
     * @retval false if invalid
     */
    virtual operator bool() const = 0;

    /*!
     * @brief Encoder type for begin.
     *
     * @retval MA_OK on success
     */
    virtual ma_err_t begin() = 0;

    /*!
     * @brief Encoder type for begin.
     *
     * @param[in] type
     * @param[in] code
     * @param[in] name
     * @retval MA_OK on success
     */
    virtual ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name) = 0;

    /*!
     * @brief Encoder type for begin.
     *
     * @param[in] type
     * @param[in] code
     * @param[in] name
     * @param[in] data
     * @retval MA_OK on success
     */
    virtual ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name, const std::string& data) = 0;


    /*!
     * @brief Encoder type for begin.
     *
     * @param[in] type
     * @param[in] code
     * @param[in] name
     * @param[in] data
     * @retval MA_OK on success
     */
    virtual ma_err_t begin(ma_msg_type_t type, ma_err_t code, const std::string& name, uint64_t data) = 0;

    /*!
     * @brief Encoder type for end.
     *
     * @retval MA_OK on success
     */
    virtual ma_err_t end() = 0;

    /*!
     * @brief Reset the proto object.
     *
     * @retval MA_OK on success
     */
    virtual ma_err_t reset() = 0;


    /*!
     * @brief Encoder type for convert to string
     *
     * @retval std::string
     */
    virtual const std::string& toString() const = 0;

    /*!
     * @brief Encoder type for get data
     *
     * @retval data
     */
    virtual const void* data() const = 0;

    /*!
     * @brief Encoder type for get size
     *
     * @retval size
     */
    virtual const size_t size() const = 0;

    /*!
     * @brief Encoder type for remove.
     *
     * @param[in] key
     * @retval MA_OK on success
     */
    virtual ma_err_t remove(const std::string& key) = 0;

    /*!
     * @brief Encoder type for write int8_t value.
     *
     * @param[in] key
     * @param[in] value int8_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int8_t value) = 0;

    /*!
     * @brief Encoder type for write int16_t value.
     *
     * @param[in] key
     * @param[in] value int16_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int16_t value) = 0;

    /*!
     * @brief Encoder type for write int32_t value.
     *
     * @param[in] key
     * @param[in] value int32_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int32_t value) = 0;

    /*!
     * @brief Encoder type for write int64_t value.
     *
     * @param[in] key
     * @param[in] value int64_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int64_t value) = 0;

    /*!
     * @brief Encoder type for write uint8_t value.
     *
     * @param[in] key
     * @param[in] value uint8_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint8_t value) = 0;

    /*!
     * @brief Encoder type for write uint16_t value.
     *
     * @param[in] key
     * @param[in] value uint16_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint16_t value) = 0;

    /*!
     * @brief Encoder type for write uint32_t value.
     *
     * @param[in] key
     * @param[in] value uint32_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint32_t value) = 0;

    /*!
     * @brief Encoder type for write uint64_t value.
     *
     * @param[in] key
     * @param[in] value uint64_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint64_t value) = 0;

    /*!
     * @brief Encoder type for write float value.
     *
     * @param[in] key
     * @param[in] value float typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, float value) = 0;

    /*!
     * @brief Encoder type for write double value.
     *
     * @param[in] key
     * @param[in] value double typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, double value) = 0;

    /*!
     * @brief Encoder type for write const std::string value.
     *
     * @param[in] key
     * @param[in] value const std::string typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, const std::string& value) = 0;

    /*!
     * @brief Encoder type for write ma_model_t value.
     *
     * @param[in] key
     * @param[in] value ma_model_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, ma_model_t value) = 0;

    /*!
     * @brief Encoder type for write ma_perf_t
     *
     * @param[in] value ma_perf_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(ma_perf_t value) = 0;

    /*!
     * @brief Encoder type for write std::forward_list<ma_class_t> value.
     *
     * @param[in] value std::forward_list<ma_class_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::forward_list<ma_class_t>& value) = 0;

    /*!
     * @brief Encoder type for write std::vector<ma_point_t> value.
     *
     * @param[in] value std::vector<ma_point_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::forward_list<ma_point_t>& value) = 0;

    /*!
     * @brief Encoder type for write std::forward_list<ma_bbox_t> value.
     *
     * @param[in] value std::forward_list<ma_bbox_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::forward_list<ma_bbox_t>& value) = 0;

    /*!
     * @brief Encoder type for write std::forward_list<ma_keypoint3f_t> value.
     *
     * @param[in] value std::forward_list<ma_keypoint3f_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::forward_list<ma_keypoint3f_t>& value) = 0;

    /*!
     * @brief Encoder type for write std::forward_list<ma_model_t> value.
     *
     * @param[in] value std::forward_list<ma_model_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::vector<ma_model_t>& value) = 0;

    virtual ma_err_t write(const std::vector<Sensor*>& value) = 0;

    virtual ma_err_t write(const Sensor* value, size_t preset) = 0;

    virtual ma_err_t write(const std::string& key, const char* buffer, size_t size) = 0;


    virtual ma_err_t write(const in4_info_t& value) = 0;
    virtual ma_err_t write(const in6_info_t& value) = 0;

    virtual ma_err_t write(const ma_wifi_config_t& value, int* stat = nullptr) = 0;

    virtual ma_err_t write(const ma_mqtt_config_t& value, int* stat = nullptr) = 0;

    virtual ma_err_t write(const ma_mqtt_topic_config_t& value) = 0;

    virtual ma_err_t write(int algo_id, int cat, int input_from, int tscore, int tiou) = 0;
};

class Decoder {
public:
    virtual ~Decoder() = default;

    virtual operator bool() const = 0;

    virtual ma_err_t begin(const void* data, size_t size) = 0;
    virtual ma_err_t begin(const std::string& str)        = 0;
    virtual ma_err_t end()                                = 0;

    virtual ma_msg_type_t type() const = 0;
    virtual ma_err_t code() const      = 0;
    virtual std::string name() const   = 0;


    virtual ma_err_t read(const std::string& key, int8_t& value) const      = 0;
    virtual ma_err_t read(const std::string& key, int16_t& value) const     = 0;
    virtual ma_err_t read(const std::string& key, int32_t& value) const     = 0;
    virtual ma_err_t read(const std::string& key, int64_t& value) const     = 0;
    virtual ma_err_t read(const std::string& key, uint8_t& value) const     = 0;
    virtual ma_err_t read(const std::string& key, uint16_t& value) const    = 0;
    virtual ma_err_t read(const std::string& key, uint32_t& value) const    = 0;
    virtual ma_err_t read(const std::string& key, uint64_t& value) const    = 0;
    virtual ma_err_t read(const std::string& key, float& value) const       = 0;
    virtual ma_err_t read(const std::string& key, double& value) const      = 0;
    virtual ma_err_t read(const std::string& key, std::string& value) const = 0;
    virtual ma_err_t read(ma_perf_t& value)                                 = 0;
    virtual ma_err_t read(std::forward_list<ma_class_t>& value)             = 0;
    virtual ma_err_t read(std::forward_list<ma_point_t>& value)             = 0;
    virtual ma_err_t read(std::forward_list<ma_bbox_t>& value)              = 0;
};

}  // namespace ma

#endif  // _MA_CODEC_BASE_H_