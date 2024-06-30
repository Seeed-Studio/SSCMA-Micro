#ifndef _MA_CODEC_BASE_H_
#define _MA_CODEC_BASE_H_


#include <string>
#include <vector>

#include "core/ma_common.h"


namespace ma {

class Codec {
public:
    Codec()          = default;
    virtual ~Codec() = default;

    /*!
     * @brief Codec is valid.
     *
     * @retval true if valid
     * @retval false if invalid
     */
    virtual operator bool() const = 0;

    /*!
     * @brief Codec type for begin.
     *
     * @retval MA_OK on success
     */
    virtual ma_err_t begin() = 0;

    /*!
     * @brief Codec type for begin.
     *
     * @param[in] reply
     * @param[in] code
     * @param[in] name
     * @retval MA_OK on success
     */
    virtual ma_err_t begin(ma_reply_t reply, ma_err_t code, const std::string& name) = 0;

    /*!
     * @brief Codec type for begin.
     *
     * @param[in] reply
     * @param[in] code
     * @param[in] name
     * @param[in] data
     * @retval MA_OK on success
     */
    virtual ma_err_t begin(ma_reply_t reply, ma_err_t code, const std::string& name, const std::string& data) = 0;

    /*!
     * @brief Codec type for end.
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
     * @brief Codec type for convert to string
     *
     * @retval std::string
     */
    virtual const std::string& toString() const = 0;

    /*!
     * @brief Codec type for get data
     *
     * @retval data
     */
    virtual const void* data() const = 0;

    /*!
     * @brief Codec type for get size
     *
     * @retval size
     */
    virtual const size_t size() const = 0;

    /*!
     * @brief Codec type for remove.
     *
     * @param[in] key
     * @retval MA_OK on success
     */
    virtual ma_err_t remove(const std::string& key) = 0;

    /*!
     * @brief Codec type for write boolean value.
     *
     * @param[in] key
     * @param[in] value Boolean typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, bool value) = 0;

    /*!
     * @brief Codec type for write int8_t value.
     *
     * @param[in] key
     * @param[in] value int8_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int8_t value) = 0;

    /*!
     * @brief Codec type for write int16_t value.
     *
     * @param[in] key
     * @param[in] value int16_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int16_t value) = 0;

    /*!
     * @brief Codec type for write int32_t value.
     *
     * @param[in] key
     * @param[in] value int32_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int32_t value) = 0;

    /*!
     * @brief Codec type for write int64_t value.
     *
     * @param[in] key
     * @param[in] value int64_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, int64_t value) = 0;

    /*!
     * @brief Codec type for write uint8_t value.
     *
     * @param[in] key
     * @param[in] value uint8_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint8_t value) = 0;

    /*!
     * @brief Codec type for write uint16_t value.
     *
     * @param[in] key
     * @param[in] value uint16_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint16_t value) = 0;

    /*!
     * @brief Codec type for write uint32_t value.
     *
     * @param[in] key
     * @param[in] value uint32_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint32_t value) = 0;

    /*!
     * @brief Codec type for write uint64_t value.
     *
     * @param[in] key
     * @param[in] value uint64_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, uint64_t value) = 0;

    /*!
     * @brief Codec type for write float value.
     *
     * @param[in] key
     * @param[in] value float typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, float value) = 0;

    /*!
     * @brief Codec type for write double value.
     *
     * @param[in] key
     * @param[in] value double typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, double value) = 0;

    /*!
     * @brief Codec type for write const std::string value.
     *
     * @param[in] key
     * @param[in] value const std::string typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(const std::string& key, const std::string& value) = 0;

    /*!
     * @brief Codec type for write ma_perf_t
     *
     * @param[in] value ma_perf_t typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(ma_perf_t value) = 0;

    /*!
     * @brief Codec type for write std::vector<ma_class_t> value.
     *
     * @param[in] value std::vector<ma_class_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(std::vector<ma_class_t>& value) = 0;

    /*!
     * @brief Codec type for write std::vector<ma_point_t> value.
     *
     * @param[in] value std::vector<ma_point_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(std::vector<ma_point_t>& value) = 0;

    /*!
     * @brief Codec type for write std::vector<ma_bbox_t> value.
     *
     * @param[in] value std::vector<ma_bbox_t> typed value to write.
     * @retval MA_OK on success
     */
    virtual ma_err_t write(std::vector<ma_bbox_t>& value) = 0;
};

}  // namespace ma

#endif  // _MA_CODEC_BASE_H_