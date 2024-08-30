// #pragma once

// #include "camera.h"
// #include "node.h"

// namespace ma::node {

// class StreamNode : public Node {

// public:
//     StreamNode(std::string id);
//     ~StreamNode();

//     ma_err_t onCreate(const json& config, const Response& response) override;

//     ma_err_t onMessage(const json& message, const Response& response) override;

//     ma_err_t onDestroy(const Response& response) override;

// protected:
//     void threadEntry();
//     static void threadEntryStub(void* obj);

// protected:
//     std::string url_;
//     CameraNode* camera_;
//     MessageBox frame_;
//     Thread* thread_;
// };

// }  // namespace ma::node