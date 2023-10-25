#pragma once

#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <queue>
#include <utility>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "sscma/definations.hpp"
#include "sscma/types.hpp"

namespace sscma::repl {

using namespace edgelab;
using namespace sscma::types;

class Executor {
   public:
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    Executor(size_t worker_stack_size = REPL_EXECUTOR_STACK_SIZE, size_t worker_priority = REPL_EXECUTOR_PRIO)
        : _task_queue_lock(),
          _task_stop_requested(false),
          _worker_thread_stop_requested(false),
          _worker_ret(),
          _worker_handler(),
          _worker_name(new char[configMAX_TASK_NAME_LEN]{}),
          _worker_stack_size(worker_stack_size),
          _worker_priority(worker_priority) {
        static uint8_t       worker_id    = 0u;
        static const char*   hex_literals = "0123456789ABCDEF";
        static const char*   header_name  = "sscma#executor#00";
        static const uint8_t header_size  = std::strlen(header_name);

        EL_ASSERT(header_size <= configMAX_TASK_NAME_LEN);

        std::memset(_worker_name, 0, configMAX_TASK_NAME_LEN);  // not trust initializer
        std::memcpy(_worker_name, header_name, header_size);

        _worker_name[header_size - 2] = hex_literals[worker_id >> 4];
        _worker_name[header_size - 1] = hex_literals[worker_id & 0x0f];
    }
#else
    Executor() : _task_queue_lock(), _task_stop_requested(false), _worker_thread_stop_requested(false) {}
#endif

    ~Executor() {
        stop();
        delete[] _worker_name;
        _worker_name = nullptr;
    }

    void start() {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        _worker_ret =
          xTaskCreate(&Executor::c_run, _worker_name, _worker_stack_size, this, _worker_priority, &_worker_handler);
#endif
    }

    void stop() {
        _worker_thread_stop_requested.store(true);
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        if (_worker_ret == pdPASS) [[likely]]
            vTaskDelete(_worker_handler);
#endif
    }

    template <typename Callable> void add_task(Callable&& task) {
        const Guard<Mutex> guard(_task_queue_lock);

        _task_queue.push(std::forward<Callable>(task));
        _task_stop_requested.store(true);
    }

    const char* get_worker_name() const {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        return _worker_name;
#else
        return "";
#endif
    }

    void run() {
#if CONFIG_EL_HAS_FREERTOS_SUPPORT
        while (!_worker_thread_stop_requested.load()) {
            repl_task_t task{};
            {
                const Guard<Mutex> guard(_task_queue_lock);
                if (!_task_queue.empty()) {
                    task = std::move(_task_queue.front());  // or std::function::swap
                    _task_queue.pop();
                    if (_task_queue.empty()) [[likely]]
                        _task_stop_requested.store(false);
                    else
                        _task_stop_requested.store(true);
                }
            }
            if (task) task(_task_stop_requested);
            vTaskDelay(15 / portTICK_PERIOD_MS);  // TODO: use yield
        }
#else
        repl_task_t task{};
        {
            const Guard<Mutex> guard(_task_queue_lock);
            if (!_task_queue.empty()) {
                task = std::move(_task_queue.front());
                _task_queue.pop();
                if (_task_queue.empty()) [[likely]]
                    _task_stop_requested.store(false);
                else
                    _task_stop_requested.store(true);
            }
            if (task) task(_task_stop_requested);
        }
#endif
    }

    static void c_run(void* this_pointer) { static_cast<Executor*>(this_pointer)->run(); }

   private:
    Mutex             _task_queue_lock;
    std::atomic<bool> _task_stop_requested;
    std::atomic<bool> _worker_thread_stop_requested;

#if CONFIG_EL_HAS_FREERTOS_SUPPORT
    BaseType_t   _worker_ret;
    TaskHandle_t _worker_handler;
    char*        _worker_name;
    size_t       _worker_stack_size;
    size_t       _worker_priority;
#endif

    std::queue<repl_task_t> _task_queue;
};

}  // namespace sscma::repl
