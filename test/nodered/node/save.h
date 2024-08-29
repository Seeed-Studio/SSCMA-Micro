#pragma once

#include "camera.h"
#include "node.h"

namespace ma::server::node {

class SaveNode : public Node {


public:
    SaveNode(std::string id);
    ~SaveNode();

    ma_err_t onCreate(const json& config, const Response& response) override;

    ma_err_t onMessage(const json& message, const Response& response) override;

    ma_err_t onDestroy(const Response& response) override;

protected:
    void threadEntry();
    static void threadEntryStub(void* obj);

private:
    std::string generateFileName();
    bool recycle();

protected:
    std::ofstream file_;
    std::string storage_;
    uint64_t max_size_;
    int slice_;
    CameraNode* camera_;
    MessageBox frame_;
    Thread* thread_;
};

}  // namespace ma::server::node