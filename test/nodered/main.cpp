#include "node/server.h"
#include <gtest/gtest.h>

namespace ma::server {


TEST(Server, Node) {

    NodeServer server("recamera");
    server.start("localhost", 1883);

    while (true) {
        Thread::sleep(Tick::fromSeconds(1));
    }
}

}  // namespace ma::server