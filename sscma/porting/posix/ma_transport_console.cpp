

#include <iostream>
#include <string>

#include "ma_transport_console.h"

namespace ma {

Console::Console() : Transport(MA_TRANSPORT_CONSOLE) {}

Console::~Console() {}
ma_err_t Console::open() {
    return MA_OK;
}

ma_err_t Console::close() {
    return MA_OK;
}
size_t Console::available() const {
    return 1;
}

size_t Console::send(const char* data, size_t length, int timeout) {
    std::cout.write(data, length);
    std::cout.flush();
    return length;
}

size_t Console::receive(char* data, size_t length, int timeout) {
    std::cin.read(data, length);
    return std::cin.gcount();
}

size_t Console::receiveUtil(char* data, size_t length, char delimiter, int timeout) {
    std::cin.getline(data, length, delimiter);
    return std::cin.gcount();
}

Console::operator bool() const {
    return true;
}

}  // namespace ma
