#include "node.h"

namespace ma::node {

static constexpr char TAG[] = "ma::node";

Node::Node(std::string type, std::string id)
    : type_(std::move(type)),
      id_(std::move(id)),
      server_(nullptr),
      started_(false),
      dependences_() {}

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

    // check dependences first
    if (data.contains("dependences")) {
        auto dependences = data["dependences"].get<std::vector<std::string>>();
        for (auto& dependence : dependences) {
            if (m_nodes.find(dependence) == m_nodes.end()) {  // if dependence not found
                throw NodeException(MA_ENOENT, "dependence not found: " + dependence);
            }
        }
    }

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
    Node* n    = it->second.create(id);
    n->server_ = server;
    if (data.contains("dependences")) {
        n->dependences_ = data["dependences"].get<std::vector<std::string>>();
    }
    m_nodes[id] = n;

    try {
        n->onCreate(data.contains("config") ? data["config"] : json::object());
    } catch (const NodeException& e) {
        delete n;
        m_nodes.erase(id);
        throw e;
    }
    return n;
}

void NodeFactory::destroy(const std::string id) {
    Guard guard(m_mutex);
    auto node = m_nodes.find(id);
    if (node == m_nodes.end()) {
        throw NodeException(MA_ENOENT, "node not found: " + id);
    }

    // check if the node has dependences
    for (auto node : m_nodes) {
        if (std::find(node.second->dependences_.begin(), node.second->dependences_.end(), id) !=
            node.second->dependences_.end()) {
            throw NodeException(MA_EBUSY, "node has dependences: " + id);
        }
    }

    node->second->onDestroy();

    delete node->second;
    m_nodes.erase(node);

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
