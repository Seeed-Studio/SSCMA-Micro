#include "node.h"

namespace ma::server {

static constexpr char TAG[] = "ma::server::node";


Node::Node(std::string type, std::string id) : type_(std::move(type)), id_(std::move(id)) {}

Node::~Node() = default;


std::unordered_map<std::string, Node*> NodeFactory::m_nodes;

Node* NodeFactory::create(const std::string& type, const std::string& id) {
    std::string _type = type;
    std::transform(_type.begin(), _type.end(), _type.begin(), ::tolower);

    MA_LOGD(TAG, "create node: %s %s", _type.c_str(), id.c_str());

    if (m_nodes.find(id) != m_nodes.end()) {
        return nullptr;
    }

    auto it = registry().find(type);
    if (it == registry().end()) {
        return nullptr;
    }


    if (it->second.singleton) {
        for (auto node : m_nodes) {
            if (node.second->type_ == _type) {
                MA_LOGW(TAG, "singleton node already exists: %s %s", _type.c_str(), id.c_str());
                return nullptr;
            }
        }
    }
    Node* n     = it->second.create(id);
    m_nodes[id] = n;

    return n;
}

void NodeFactory::destroy(const std::string& id) {
    auto node = m_nodes.find(id);
    if (node == m_nodes.end()) {
        return;
    }
    delete node->second;
    m_nodes.erase(node);
}

Node* NodeFactory::get(const std::string& id) {
    auto node = m_nodes.find(id);
    if (node == m_nodes.end()) {
        return nullptr;
    }
    return node->second;
}

std::vector<Node*> NodeFactory::getNodes(const std::string& type) {
    std::string _type = type;
    std::transform(_type.begin(), _type.end(), _type.begin(), ::tolower);
    std::vector<Node*> nodes;
    nodes.clear();
    for (auto node : m_nodes) {
        if (node.second->type_ == _type) {
            nodes.push_back(node.second);
        }
    }
    return nodes;
}

void NodeFactory::clear() {
    for (auto node : m_nodes) {
        delete node.second;
    }
    m_nodes.clear();
}

void NodeFactory::registerNode(const std::string& type, CreateNode create, bool singleton) {
    std::string _type = type;
    MA_LOGD(TAG, "register node: %s %d", type.c_str(), singleton);
    std::transform(_type.begin(), _type.end(), _type.begin(), ::tolower);
    registry()[_type] = {create, singleton};
}

std::unordered_map<std::string, NodeFactory::NodeCreator>& NodeFactory::registry() {
    static std::unordered_map<std::string, NodeFactory::NodeCreator> _registry;
    return _registry;
}


}  // namespace ma::server
