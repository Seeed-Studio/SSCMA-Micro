#include <gtest/gtest.h>


#include <sscma.h>


namespace ma::server {


TEST(Server, AT) {
    ma_err_t err     = MA_OK;
    auto* codec      = new EncoderJSON();
    auto* server     = new ATServer(codec);
    auto* transport  = new Console();
    static int value = 0;

    err = server->addService(
        "VALUE",
        "Set value",
        "<value>",
        [](std::vector<std::string> args, Transport& transport, Encoder& codec) {
            if (args.size() != 2) {
                codec.begin(MA_MSG_TYPE_EVT, MA_EINVAL, args[0]);
                codec.write("value", value);
                codec.end();
                transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
                return MA_OK;
            }
            value = std::stoi(args[1]);
            codec.begin(MA_MSG_TYPE_RESP, MA_OK, args[0]);
            codec.write("value", value);
            codec.end();
            transport.send(reinterpret_cast<const char*>(codec.data()), codec.size());
            return MA_OK;
        });

    EXPECT_EQ(err, MA_OK);

    err = server->addService("VALUE?",
                             "Get value",
                             "",
                             [](std::vector<std::string> args, Transport& transport, Encoder& codec) {
                                 codec.begin(MA_MSG_TYPE_RESP, MA_OK, args[0]);
                                 codec.write("value", value);
                                 codec.end();
                                 transport.send(reinterpret_cast<const char*>(codec.data()),
                                                codec.size());
                                 return MA_OK;
                             });


    EXPECT_EQ(err, MA_OK);

    err = server->execute("AT+VALUE=10", transport);
    EXPECT_EQ(err, MA_OK);

    EXPECT_EQ(value, 10);

    err = server->execute("AT+VALUE?", transport);
    EXPECT_EQ(err, MA_OK);

    err = server->execute("AT+UNKNOWN", transport);
    EXPECT_EQ(err, MA_EINVAL);
}


}  // namespace ma::server
