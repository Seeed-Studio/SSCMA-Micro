#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>

#include "nlohmann/json.hpp"

#include "core/ma_core.h"
#include "porting/ma_porting.h"

using json = nlohmann::json;

namespace ma::server {

class NodeFactory;
class NodeServer;

using Response = std::function<void(const json&)>;

class Node {
public:
    Node(std::string type, std::string id);
    virtual ~Node();

    virtual ma_err_t onCreate(const json& config, const Response& response)   = 0;
    virtual ma_err_t onMessage(const json& message, const Response& response) = 0;
    virtual ma_err_t onDestroy(const Response& response)                      = 0;

    const std::string& id() const {
        return id_;
    }
    const std::string& type() const {
        return type_;
    }

protected:
    Mutex mutex_;
    std::string id_;
    std::string type_;
    std::vector<std::string> wires_;

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
    static Node* create(const std::string& type, const std::string& id);
    static void destroy(const std::string& id);
    static Node* get(const std::string& id);
    static std::vector<Node*> getNodes(const std::string& type);
    static void clear();

    static void registerNode(const std::string& type, CreateNode create, bool singleton = false);

private:
    static std::unordered_map<std::string, NodeCreator>& registry();
    static std::unordered_map<std::string, Node*> m_nodes;
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

}  // namespace ma::server
