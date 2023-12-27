#pragma once

#include "core/el_types.h"

namespace edgelab {

class lwRingBuffer {
   public:
    lwRingBuffer(size_t len) : buf(new char[len]), len(len), head(0), tail(0) {}
    ~lwRingBuffer() { delete[] buf; }

    void put(char c) {
        if (isFull()) {
            head = (head + 1) % len;
        }
        buf[tail] = c;
        tail      = (tail + 1) % len;
    }
    size_t put(const char* str, size_t slen) {
        if (slen > free()) {
            slen = free();
        }
        for (size_t i = 0; i < slen; i++) {
            put(str[i]);
        }
        return slen;
    }
    void push(char c) { this->put(c); }

    char get() {
        if (isEmpty()) {
            return 0;
        }
        char c = buf[head];
        head   = (head + 1) % len;
        return c;
    }
    size_t get(char* str, size_t slen) {
        if (slen > size()) {
            slen = size();
        }
        for (size_t i = 0; i < slen; i++) {
            str[i] = get();
        }
        return slen;
    }
    char pop() { return get(); }

    bool   isEmpty() { return head == tail; }
    bool   isFull() { return (tail + 1) % len == head; }
    size_t size() { return (tail - head + len) % len; }
    size_t free() { return (head - tail - 1 + len) % len; }
    size_t capacity() { return len; }
    void   clear() { head = tail; }

    friend lwRingBuffer& operator>>(lwRingBuffer& input, char& c) {
        c = input.get();
        return input;
    }
    friend lwRingBuffer& operator<<(lwRingBuffer& output, char c) {
        output.put(c);
        return output;
    }
    char operator[](size_t i) { return buf[(head + i) % len]; }

    /**
     * Find the first occurrence of c in the ring buffer
     * @param c the character to search for
     * @return the index of the first occurrence of c in the ring buffer
     *         if c is not found, return this->len
     */
    size_t find(char c) {
        for (size_t i = head; i != tail; i = (i + 1) % len) {
            if (buf[i] == c) {
                return (i - head + len) % len;
            }
        }
        return this->len;
    }

    /**
     * Check if the ring buffer contains the string *str
     * @param str the string to search for
     * @param slen the length of the string
     * @return true if the ring buffer contains the string *str
     *         false otherwise
     */
    bool match(const char* str, size_t slen) {
        if (slen > size()) {
            return false;
        }
        for (size_t i = 0; i < slen; i++) {
            if (buf[(head + i) % len] != str[i]) {
                return false;
            }
        }
        return true;
    }

    /**
     * Extract string from the ring buffer until the character c is found
     * @param c the character to search for
     * @param str the string to extract
     * @param slen the length of the string
     * @return the length of the string extracted
     */
    size_t extract(char c, char* str, size_t slen) {
        size_t i = find(c);
        if (i == this->len) {
            return 0;
        }
        if (slen < i) {
            head = (head + i + 1) % len;
            return 0;
        }
        for (size_t j = 0; j <= i; j++) {
            str[j] = buf[(head + j) % len];
        }
        head = (head + i + 1) % len;
        return i + 1;
    }

   private:
    char*  buf;
    size_t len;
    size_t head;
    size_t tail;
};

}  // namespace edgelab
