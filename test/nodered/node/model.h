#pragma once

#include "server.h"
#include "camera.h"
#include "node.h"


namespace ma::node {

class ModelNode : public Node {

public:
    ModelNode(std::string id);
    ~ModelNode();

    ma_err_t onCreate(const json& config) override;
    ma_err_t onStart() override;
    ma_err_t onMessage(const json& message) override;
    ma_err_t onStop() override;
    ma_err_t onDestroy() override;

protected:
    void threadEntry();
    static void threadEntryStub(void* obj);

protected:
    std::string uri_;
    int times_;
    bool debug_;
    bool trace_;
    bool count_;
    Engine* engine_;
    Model* model_;
    CameraNode* camera_;
    Thread* thread_;
    MessageBox raw_frame_;
    MessageBox jpeg_frame_;
};

}  // namespace ma::node