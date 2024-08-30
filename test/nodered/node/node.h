#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

#include "core/ma_core.h"
#include "porting/ma_porting.h"

using json = nlohmann::json;

namespace ma::node {

class NodeFactory;
class NodeServer;

using Response = std::function<void(const json&)>;

class NodeException : public std::exception {
private:
    ma_err_t err_;
    std::string msg_;

public:
    NodeException(ma_err_t err, const std::string& msg) : err_(err), msg_(msg) {}
    ma_err_t err() const {
        return err_;
    }
    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

class Node {
public:
    Node(std::string type, std::string id);
    virtual ~Node();

    virtual ma_err_t onCreate(const json& config)   = 0;
    virtual ma_err_t onStart()                      = 0;
    virtual ma_err_t onMessage(const json& message) = 0;
    virtual ma_err_t onStop()                       = 0;
    virtual ma_err_t onDestroy()                    = 0;

    const std::string& id() const {
        return id_;
    }
    const std::string& type() const {
        return type_;
    }
    const std::vector<std::string>& dependencies() const {
        return dependencies_;
    }

    const std::string dump() const {
        return json({{"id", id_}, {"type", type_}, {"dependencies", dependencies_}}).dump();
    }

protected:
    Mutex mutex_;
    std::string id_;
    std::string type_;
    NodeServer* server_;
    std::atomic_bool started_;
    std::vector<std::string> dependencies_;

    friend class NodeServer;
    friend class NodeFactory;
};

class NodeFactory {

    using CreateNode = std::function<Node*(const std::string&)>;

    struct NodeCreator {
        CreateNode create;
        bool singleton;
    };

public:
    static Node* create(const std::string id,
                        const std::string type,
                        const json& data,
                        NodeServer* server);
    static void destroy(const std::string id);
    static Node* find(const std::string id);

    static void registerNode(const std::string type, CreateNode create, bool singleton = false);

private:
    static std::unordered_map<std::string, NodeCreator>& registry();
    static std::unordered_map<std::string, Node*> m_nodes;
    static Mutex m_mutex;
};

#define REGISTER_NODE(type, node)                                                                \
    class node##Registrar {                                                                      \
    public:                                                                                      \
        node##Registrar() {                                                                      \
            NodeFactory::registerNode(type, [](const std::string& id) { return new node(id); }); \
        }                                                                                        \
    };                                                                                           \
    static node##Registrar g_##node##Registrar;

#define REGISTER_NODE_SINGLETON(type, node)                                      \
    class node##Registrar {                                                      \
    public:                                                                      \
        node##Registrar() {                                                      \
            NodeFactory::registerNode(                                           \
                type, [](const std::string& id) { return new node(id); }, true); \
        }                                                                        \
    };                                                                           \
    static node##Registrar g_##node##Registrar;

}  // namespace ma::node
