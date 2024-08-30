#pragma once

#include <string>

#include "core/ma_core.h"
#include "porting/ma_porting.h"

#include "executor.hpp"
#include "node.h"
namespace ma::node {

class NodeServer {
public:
    NodeServer(std::string client_id);
    ~NodeServer();

    ma_err_t start(std::string host     = "localhost",
                   int port             = 1883,
                   std::string username = "",
                   std::string password = "");
    ma_err_t stop();

    // void dispatch(const std::string& id, const json& msg);
    void response(const std::string& id, const json& msg);

protected:
    void onConnect(struct mosquitto* mosq, int rc);
    void onDisconnect(struct mosquitto* mosq, int rc);
    void onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg);

private:
    static void onConnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectStub(struct mosquitto* mosq, void* obj, int rc);
    static void onMessageStub(struct mosquitto* mosq,
                              void* obj,
                              const struct mosquitto_message* msg);

    struct mosquitto* m_client;
    std::string m_client_id;
    std::string m_topic_in_prefix;
    std::string m_topic_out_prefix;
    std::atomic<bool> m_connected;
    Executor m_executor;
    Mutex m_mutex;
};

}  // namespace ma::node