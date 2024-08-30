
#include "server.h"

namespace ma::node {

static constexpr char TAG[] = "ma::node::server";

void NodeServer::onConnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "connect");
    std::string topic = m_topic_in_prefix + "/+";
    mosquitto_subscribe(mosq, NULL, m_topic_in_prefix.c_str(), 0);
    mosquitto_subscribe(mosq, NULL, topic.c_str(), 0);
    response("", json::object({{"code", 0}}));
    m_connected.store(true);
}

void NodeServer::onDisconnect(struct mosquitto* mosq, int rc) {
    MA_LOGD(TAG, "disconnect");
    m_connected.store(false);
}

void NodeServer::onMessage(struct mosquitto* mosq, const struct mosquitto_message* msg) {
    std::string topic = msg->topic;
    std::string id    = "";
    json payload;
    try {
        id      = topic.substr(m_topic_in_prefix.length() + 1);
        payload = json::parse(static_cast<const char*>(msg->payload), nullptr, false);

        if (!payload.contains("name") || !payload.contains("data")) {
            throw NodeException(MA_EINVAL, "invalid payload");
        }
        MA_LOGD(TAG, "Request: %s", payload.dump().c_str());
        m_executor.submit([this, id = std::move(id), payload = std::move(payload)]() -> bool {
            std::string name = payload["name"].get<std::string>();
            const json& data = payload["data"];
            try {
                if (name == "create") {
                    if (!data.contains("type")) {
                        throw NodeException(MA_EINVAL, "invalid payload");
                    }
                    std::string type   = data["type"].get<std::string>();
                    const json& config = data.contains("config") ? data["config"] : json::object();
                    NodeFactory::create(id, type, data, this);
                } else if (name == "destroy") {
                    MA_LOGD(TAG, "22destroy: %s", id.c_str());
                    NodeFactory::destroy(id);
                    MA_LOGD(TAG, "33destroy done: %s", id.c_str());
                } else {
                    Node* node = NodeFactory::find(id);
                    if (node) {
                        node->onMessage(data);
                    }
                }
            } catch (const NodeException& e) {
                this->response(id,
                               json::object({{"type", MA_MSG_TYPE_RESP},
                                             {"name", name},
                                             {"code", e.err()},
                                             {"data", e.what()}}));
                return e.err() == MA_AGAIN;
            } catch (const std::exception& e) {
                this->response(id,
                               json::object({{"type", MA_MSG_TYPE_RESP},
                                             {"name", name},
                                             {"code", MA_EINVAL},
                                             {"data", e.what()}}));
                return false;
            }
            return false;
        });

    } catch (const NodeException& e) {
        response(id,
                 json::object({{"type", MA_MSG_TYPE_RESP},
                               {"name", "request"},
                               {"code", e.err()},
                               {"data", e.what()}}));
    } catch (const std::exception& e) {
        response(id,
                 json::object({{"type", MA_MSG_TYPE_RESP},
                               {"name", "request"},
                               {"code", MA_EINVAL},
                               {"data", e.what()}}));
    }
}

void NodeServer::onConnectStub(struct mosquitto* mosq, void* obj, int rc) {
    NodeServer* server = static_cast<NodeServer*>(obj);
    if (server) {
        server->onConnect(mosq, rc);
    }
}

void NodeServer::onDisconnectStub(struct mosquitto* mosq, void* obj, int rc) {

    NodeServer* server = static_cast<NodeServer*>(obj);
    if (server) {
        server->onDisconnect(mosq, rc);
    }
}

void NodeServer::onMessageStub(struct mosquitto* mosq,
                               void* obj,
                               const struct mosquitto_message* msg) {

    NodeServer* server = static_cast<NodeServer*>(obj);
    if (server) {
        server->onMessage(mosq, msg);
    }
}

void NodeServer::response(const std::string& id, const json& msg) {
    Guard guard(m_mutex);
    std::string topic = m_topic_out_prefix + '/' + id;
    MA_LOGV(TAG, "response:%s ==> %s", topic.c_str(), msg.dump().c_str());
    int mid = mosquitto_publish(
        m_client, nullptr, topic.c_str(), msg.dump().size(), msg.dump().data(), 0, false);
}

NodeServer::NodeServer(std::string client_id)
    : m_client(nullptr), m_connected(false), m_client_id(std::move(client_id)) {
    mosquitto_lib_init();

    m_client = mosquitto_new(m_client_id.c_str(), true, this);
    MA_ASSERT(m_client);

    mosquitto_connect_callback_set(m_client, onConnectStub);
    mosquitto_disconnect_callback_set(m_client, onDisconnectStub);
    mosquitto_message_callback_set(m_client, onMessageStub);

    m_topic_in_prefix  = std::string("sscma/v0/" + m_client_id + "/node/in");
    m_topic_out_prefix = std::string("sscma/v0/" + m_client_id + "/node/out");
}
NodeServer::~NodeServer() {
    if (m_client) {
        mosquitto_disconnect(m_client);
        mosquitto_destroy(m_client);
        m_client = nullptr;
    }
    mosquitto_lib_cleanup();
}

ma_err_t NodeServer::start(std::string host, int port, std::string username, std::string password) {

    if (m_connected.load()) {
        return MA_EBUSY;
    }

    mosquitto_loop_start(m_client);

    mosquitto_reconnect_delay_set(m_client, 2, 30, true);
    if (username.length() > 0 && password.length() > 0) {
        mosquitto_username_pw_set(m_client, username.c_str(), password.c_str());
    }
    int rc = mosquitto_connect(m_client, host.c_str(), port, 60);

    MA_LOGI(TAG, "node server started: mqtt://%s:%d", host.c_str(), port);
    MA_LOGD(
        TAG, "topic_in: %s, topic_out: %s", m_topic_in_prefix.c_str(), m_topic_out_prefix.c_str());


    return m_client && rc == MOSQ_ERR_SUCCESS ? MA_OK : MA_AGAIN;
}

ma_err_t NodeServer::stop() {
    if (m_client && m_connected.load()) {
        mosquitto_disconnect(m_client);
        mosquitto_loop_stop(m_client, true);
    }
    return MA_OK;
}

}  // namespace ma::node