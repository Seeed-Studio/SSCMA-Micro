#ifndef _MA_SERVER_AT_H_
#define _MA_SERVER_AT_H_

#include <functional>
#include <string>

#include "core/ma_common.h"

#include "core/codec/ma_codec.h"
#include "porting/ma_transport.h"


namespace ma {

class ATServer;

typedef std::function<ma_err_t(std::vector<std::string>, Transport&, Codec&)> ATServiceCallback;

struct ATService {
    ATService(const std::string& name,
              const std::string& desc,
              const std::string& args,
              ATServiceCallback cb);

    std::string name;
    std::string desc;
    std::string args;
    ATServiceCallback cb;

    friend class ATServer;
};

class ATServer {
public:
    ATServer(Codec& codec) : m_codec(codec) {}
    ATServer(Codec* codec) : m_codec(*codec) {}
    ~ATServer() = default;

    ma_err_t addService(const std::string& name,
                        const std::string& desc,
                        const std::string& args,
                        ATServiceCallback cb);
    ma_err_t addService(ATService& service);
    ma_err_t removeService(const std::string& name);
    ma_err_t execute(std::string line, Transport& transport);
    ma_err_t execute(std::string line, Transport* transport);

private:
    std::vector<ATService> m_services;
    Codec& m_codec;
};

}  // namespace ma

#endif  // _MA_SERVER_AT_H_
