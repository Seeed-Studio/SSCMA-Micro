#pragma once

#include "core/el_types.h"

namespace edgelab {

class lwRingBuffer {
   public:
    lwRingBuffer(size_t len) : buf(new char[len]), len(len), head(0), tail(0) {}
    ~lwRingBuffer() { delete[] buf; }
    void push(char c) {
        if (isFull()) {
            head = (head + 1) % len;
        }
        buf[tail] = c;
        tail      = (tail + 1) % len;
    }
    void push(const char* str, int slen) {
        for (int i = 0; i < slen; i++) {
            push(str[i]);
        }
    }
    void pop(char* str, int slen) {
        if (slen > size()) {
            slen = size();
        }
        for (int i = 0; i < slen; i++) {
            str[i] = pop();
        }
    }
    char pop() {
        if (isEmpty()) {
            return 0;
        }
        char c = buf[head];
        head   = (head + 1) % len;
        return c;
    }
    bool   isEmpty() { return head == tail; }
    bool   isFull() { return (tail + 1) % len == head; }
    size_t size() { return (tail - head + len) % len; }
    size_t capacity() { return len; }
    void   clear() { head = tail; }

    friend lwRingBuffer& operator>>(lwRingBuffer& input, char& c) {
        c = input.pop();
        return input;
    }
    friend lwRingBuffer& operator<<(lwRingBuffer& output, char c) {
        output.push(c);
        return output;
    }
    char operator[](int i) { return buf[(head + i) % len]; }
    int  find(char c) {
        for (int i = head; i != tail; i = (i + 1) % len) {
            if (buf[i] == c) {
                return (i - head + len) % len;
            }
        }
        return -1;
    }
    bool match(const char* str, int slen) {
        if (slen > size()) {
            return false;
        }
        for (int i = 0; i < slen; i++) {
            if (buf[(head + i) % len] != str[i]) {
                return false;
            }
        }
        return true;
    }
    int extract(char c, char* str, int slen) {
        int i = find(c);
        if (i == -1) {
            return 0;
        }
        if (slen < i) {
            head = (head + i + 1) % len;
            return 0;
        }
        for (int j = 0; j <= i; j++) {
            str[j] = buf[(head + j) % len];
        }
        head = (head + i + 1) % len;
        return i + 1;
    }

   private:
    char*  buf;
    size_t len;
    int    head;
    int    tail;
};

}  // namespace edgelab
