#pragma once

#include <cstdint>
#include <functional>
#include <list>
#include <utility>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"

namespace sscma {

namespace types {

typedef std::function<void(void*)> hooked_cb_t;

}

namespace prototypes {

using namespace edgelab;

using namespace sscma::types;

class Supervisable {
   public:
    virtual ~Supervisable()             = default;
    virtual void poll_from_supervisor() = 0;
};

class Interface {
   public:
    virtual ~Interface() = default;
};

class StatefulInterface : public Interface {
   public:
    virtual ~StatefulInterface() = default;

    virtual bool is_interface_up() const = 0;

    void add_post_up_callback(void* from, hooked_cb_t cb) {
        const Guard guard(_post_up_callbacks_lock);
        _post_up_callbacks.emplace_back(std::make_pair(from, cb));
    }

    void add_pre_down_callback(void* from, hooked_cb_t cb) {
        const Guard guard(_pre_down_callbacks_lock);
        _pre_down_callbacks.emplace_back(std::make_pair(from, cb));
    }

    void remove_post_up_callback(void* from) {
        const Guard guard(_post_up_callbacks_lock);
        _post_up_callbacks.remove_if([from](const std::pair<void*, hooked_cb_t>& cb) { return cb.first == from; });
    }

    void remove_pre_down_callback(void* from) {
        const Guard guard(_pre_down_callbacks_lock);
        _pre_down_callbacks.remove_if([from](const std::pair<void*, hooked_cb_t>& cb) { return cb.first == from; });
    }

    void invoke_post_up_callbacks() {
        _post_up_callbacks_lock.lock();
        auto copy = _post_up_callbacks;
        _post_up_callbacks_lock.unlock();
        for (auto& cb : copy) cb.second(cb.first);
    }

    void invoke_pre_down_callbacks() {
        _pre_down_callbacks_lock.lock();
        auto copy = _pre_down_callbacks;
        _pre_down_callbacks_lock.unlock();
        for (auto& cb : copy) cb.second(cb.first);
    }

   private:
    Mutex                                    _post_up_callbacks_lock;
    std::list<std::pair<void*, hooked_cb_t>> _post_up_callbacks;
    Mutex                                    _pre_down_callbacks_lock;
    std::list<std::pair<void*, hooked_cb_t>> _pre_down_callbacks;
};

}  // namespace prototypes

}  // namespace sscma
