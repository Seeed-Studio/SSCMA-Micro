// #include "ma_client_at.h"


// namespace ma::client {

// ATClient::ATClient(Transport* transport, Encoder* codec) : transport(transport), codec(codec) {}

// ATClient::~ATClient() {}

// bool ATClient::operator bool() const {
//     return transport && codec;
// }

// ma_err_t ATClient::begin() {
//     return MA_OK;
// }

// ma_err_t ATClient::Break() {}


// ma_err_t ATClient::request(std::string line, bool wait, ma_tick_t timeout) {
//     ma_err_t ret = MA_OK;
//     ATRequest req;

//     auto extract = [](const std::string& str) -> std::string {
//         size_t start = str.find("+");
//         if (start != std::string::npos) {
//             size_t end = str.find("=", start);
//             return str.substr(start + 1, end - (start + 1));
//         }
//         return "";
//     };

//     if (wait) {
//         req.cmd = extract(line);
//         queue.emplace_front(std::move(req));
//     }

//     if (transport->send(line.c_str(), line.size()) != MA_OK) {
//         goto exit;
//     }

//     if (wait) {
//         char* buf = nullptr;
//         if (!req.msg.fetch(&buf, timeout)) {
//             goto exit;
//         }
//         // TODO: parse response
//     }

// exit:
//     if (wait) {
//         // remove this command from queue
//         auto it = std::find_if(queue.begin(), queue.end(), [cmd = req.cmd](const ATRequest& req) {
//             return req.cmd == cmd;
//         })
//     }
//     return MA_OK;
// }

// }  // namespace ma::client