// #ifndef _MA_CLIENT_AT_H_
// #define _MA_CLIENT_AT_H_

// #include <string>

// #include "core/ma_core.h"
// #include "porting/ma_porting.h"

// namespace ma::client {

// class ATRequest {
// public:
//     ATRequest() : data(nullptr){};
//     ~ATRequest(){};

//     std::string cmd;
//     MessageBox msg;
//     friend class ATClient;
// };

// class ATClient final {
// public:
//     ATClient(Transport* transport, Encoder* codec) = default;
//     ~ATClient()                                    = default;

//     bool operator bool() const;

//     ma_err_t begin();

//     ma_err_t Break();


// public:
//     ma_err_t request(std::string line,
//                      std::string& response,
//                      bool wait         = true,
//                      ma_tick_t timeout = Tick::WaitForever);


// private:
//     Transport* transport;
//     Encoder* codec;
//     Mutex mutex;
//     std::forward_list<ATRequest> queue;
// };

// }  // namespace ma::client