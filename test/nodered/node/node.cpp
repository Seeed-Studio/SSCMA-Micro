#include "node.h"

namespace ma::node {

static constexpr char TAG[] = "ma::node";

Node::Node(std::string type, std::string id)
    : type_(std::move(type)),
      id_(std::move(id)),
      server_(nullptr),
      started_(false),
      dependencies_() {}

Node::~Node() = default;

std::unordered_map<std::string, Node*> NodeFactory::m_nodes;
Mutex NodeFactory::m_mutex;

Node* NodeFactory::create(const std::string id,
                          const std::string type,
                          const json& data,
                          NodeServer* server) {

    Guard guard(m_mutex);
    std::string _type = type;
    std::transform(_type.begin(), _type.end(), _type.begin(), ::tolower);

    MA_LOGW(TAG, "create: %s ==> %s", id.c_str(), _type.c_str());

    if (m_nodes.find(id) != m_nodes.end()) {
        throw NodeException(MA_EEXIST, "node already exists: " + id);
    }

    auto it = registry().find(_type);

    if (it == registry().end()) {
        throw NodeException(MA_ENOENT, "node type not found: " + _type);
    }

    if (it->second.singleton) {
        for (auto node : m_nodes) {
            if (node.second->type_ == _type) {
                throw NodeException(MA_EBUSY, "node _type is singleton: " + _type);
            }
        }
    }

    if (data.contains("dependencies")) {
        for (auto dep : data["dependencies"].get<std::vector<std::string>>()) {
            if (m_nodes.find(dep) == m_nodes.end()) {
                throw NodeException(MA_AGAIN, "node dependency not found: " + dep);
            }
        }
    }


    Node* n    = it->second.create(id);
    n->server_ = server;
    if (data.contains("dependencies")) {
        n->dependencies_ = data["dependencies"].get<std::vector<std::string>>();
    }
    m_nodes[id] = n;

    try {
        n->onCreate(data.contains("config") ? data["config"] : json::object());
    } catch (const NodeException& e) {
        delete n;
        m_nodes.erase(id);
        throw e;
    }

    if (!n->dependencies_.empty()) {
        for (auto dep : n->dependencies_) {
            if (m_nodes.find(dep) == m_nodes.end()) {
                throw NodeException(MA_ENOENT, "node dependency not found: " + dep);
            }
            m_nodes[dep]->onStart();
        }
    }

    n->onStart();

    return n;
}

void NodeFactory::destroy(const std::string id) {
    Guard guard(m_mutex);

    MA_LOGW(TAG, "destroy node: %s", id.c_str());

    auto node = m_nodes.find(id);
    if (node == m_nodes.end()) {
        throw NodeException(MA_ENOENT, "node not found: " + id);
    }

    // check if the node has dependencies
    for (auto node : m_nodes) {
        if (std::find(node.second->dependencies_.begin(), node.second->dependencies_.end(), id) !=
            node.second->dependencies_.end()) {
            MA_LOGV(TAG, "node has dependencies: %s, stop it", node.second->id().c_str());
            node.second->onStop();
        }
    }

    node->second->onStop();
    node->second->onDestroy();

    delete node->second;
    m_nodes.erase(id);

    return;
}

Node* NodeFactory::find(const std::string id) {
    Guard guard(m_mutex);
    auto node = m_nodes.find(id);
    if (node == m_nodes.end()) {
        return nullptr;
    }
    return node->second;
}

void NodeFactory::registerNode(const std::string type, CreateNode create, bool singleton) {
    MA_LOGD(TAG, "register node type: %s %d", type.c_str(), singleton);
    std::string _type = type;
    std::transform(_type.begin(), _type.end(), _type.begin(), ::tolower);
    registry()[_type] = {create, singleton};
}

std::unordered_map<std::string, NodeFactory::NodeCreator>& NodeFactory::registry() {
    static std::unordered_map<std::string, NodeFactory::NodeCreator> _registry;
    return _registry;
}


}  // namespace ma::node
