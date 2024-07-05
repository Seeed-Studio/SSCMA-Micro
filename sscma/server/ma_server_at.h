#ifndef _MA_SERVER_AT_H_
#define _MA_SERVER_AT_H_

#include <functional>
#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"

#ifndef MA_SEVER_AT_EXECUTOR_STACK_SIZE
#define MA_SEVER_AT_EXECUTOR_STACK_SIZE 20 * 1024
#endif

#ifndef MA_SEVER_AT_EXECUTOR_TASK_PRIO
#define MA_SEVER_AT_EXECUTOR_TASK_PRIO 2
#endif

#ifndef MA_SEVER_AT_CMD_MAX_LENGTH
#define MA_SEVER_AT_CMD_MAX_LENGTH 4096
#endif

namespace ma {

class ATServer;

typedef std::function<ma_err_t(std::vector<std::string>, Transport&, Codec&)> ATServiceCallback;

struct ATService {
    ATService(const std::string& name,
              const std::string& desc,
              const std::string& args,
              ATServiceCallback cb);

    std::string name;
    std::string desc;
    std::string args;
    ATServiceCallback cb;

    friend class ATServer;
};

class ATServer {
public:
    ATServer(Codec& codec);
    ATServer(Codec* codec);
    ~ATServer() = default;

    ma_err_t init();

    ma_err_t start();
    ma_err_t stop();

public:
    ma_err_t addService(const std::string& name,
                        const std::string& desc,
                        const std::string& args,
                        ATServiceCallback cb);
    ma_err_t addService(ATService& service);
    ma_err_t removeService(const std::string& name);
    ma_err_t execute(std::string line, Transport& transport);
    ma_err_t execute(std::string line, Transport* transport);

protected:
    void threadEntry();

private:
    static void threadEntryStub(void* arg);
    Thread* m_thread;
    Executor* m_executor;
    Codec& m_codec;
    Device* m_device;
    Storage* m_storage;
    std::forward_list<Transport*> m_transports;
    std::vector<ATService> m_services;
    ma_wifi_config_t m_wifiConfig;
    ma_mqtt_config_t m_mqttConfig;
    ma_mqtt_topic_config_t m_mqttTopicConfig;
};

}  // namespace ma

#endif  // _MA_SERVER_AT_H_
