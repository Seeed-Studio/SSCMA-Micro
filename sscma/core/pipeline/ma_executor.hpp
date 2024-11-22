#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include "../ma_common.h"
#include "porting/ma_osal.h"

namespace ma {

typedef std::function<void(std::atomic<bool>&)> task_t;

class Executor {
   public:
    Executor(std::size_t stack_size = MA_SEVER_AT_EXECUTOR_STACK_SIZE, std::size_t priority = MA_SEVER_AT_EXECUTOR_TASK_PRIO)
        : _task_queue_lock(), _task_reload(false), _worker_name(MA_EXECUTOR_WORKER_NAME_PREFIX), _worker_handler() {
        static uint8_t     worker_id    = 0u;
        static const char* hex_literals = "0123456789ABCDEF";

        worker_id++;

        // prepare worker name (FreeRTOS task required), reserve 2 bytes for uint8_t hex string
        _worker_name.reserve(_worker_name.length() + (sizeof(uint8_t) << 1) + 1);

        // convert worker id to hex string
        _worker_name += hex_literals[worker_id >> 4];
        _worker_name += hex_literals[worker_id & 0x0f];

        _worker_handler = new Thread(_worker_name.c_str(), &Executor::c_run, this, priority, stack_size);
        MA_ASSERT(_worker_handler);
        if (!_worker_handler->start(this)) {
            delete _worker_handler;
            MA_ASSERT(false);
        }
    }

    ~Executor() {
        cancel();
        _worker_handler->stop();
        delete _worker_handler;
    }

    // the Callable must be a function object or a lambda, the prototype is task_t
    template <typename Callable> inline void submit(Callable&& callable) {
        const Guard guard(_task_queue_lock);
        _task_queue.push(std::forward<Callable>(callable));
        MA_LOGD("E", "Executor::submit: %s, task count = %zu", _worker_name.c_str(), _task_queue.size());
    }

    inline void cancel() {
        const Guard guard(_task_queue_lock);
        while (!_task_queue.empty()) _task_queue.pop();
    }

   protected:
    void run() {
        while (true) {
            Thread::sleep(Tick::fromMilliseconds(10));
            Thread::yield();
            task_t      task{};
            const Guard guard(_task_queue_lock);
            if (!_task_queue.empty()) [[likely]] {
                task = std::move(_task_queue.front());  // or std::function::swap
                _task_queue.pop();
            }
            if (task) [[likely]] {
                _task_reload.store(false);
                task(_task_reload);
                if (_task_reload.load()) [[unlikely]]
                    _task_queue.push(std::move(task));  // put back the task
            }
        }
    }

    static void c_run(void* this_pointer) { static_cast<Executor*>(this_pointer)->run(); }

   private:
    Mutex             _task_queue_lock;
    std::atomic<bool> _task_reload;
    std::string       _worker_name;
    Thread*           _worker_handler;

    std::queue<task_t> _task_queue;
};

}  // namespace ma
