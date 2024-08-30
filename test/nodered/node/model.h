// #pragma once

// #include "node.h"
// #include "camera.h"

// namespace ma::node {

// class ModelNode : public Node {

// public:
//     ModelNode(std::string id);
//     ~ModelNode();

//     ma_err_t onCreate(const json& config, const Response& response) override;

//     ma_err_t onMessage(const json& message, const Response& response) override;

//     ma_err_t onDestroy(const Response& response) override;

// protected:
//     void threadEntry();
//     static void threadEntryStub(void* obj);

// protected:
//     std::string uri_;
//     int times_;
//     bool debug_;
//     bool trace_;
//     bool count_;
//     Engine* engine_;
//     Model* model_;
//     CameraNode* camera_;
//     Thread* thread_;
//     MessageBox raw_frame_;
//     MessageBox jpeg_frame_;
// };

// }  // namespace ma::node