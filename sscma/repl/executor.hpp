#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "sscma/definations.hpp"
#include "sscma/types.hpp"

namespace sscma::repl {

using namespace edgelab;

using namespace sscma::types;

// TODO: memory order should be optimized for different architecures
class Executor {
   public:
    Executor()
        : _task_queue_lock(),
          _task_stop_requested(false),
          _worker_thread_stop_requested(false),
          _worker_name(SSCMA_EXECUTOR_WORKER_NAME_PREFIX),
          _worker_handler() {
        static uint8_t     worker_id    = 0u;
        static const char* hex_literals = "0123456789ABCDEF";

        // prepare worker name (FreeRTOS task required), reserve 2 bytes for uint8_t hex string
        _worker_name.reserve(_worker_name.length() + (sizeof(uint8_t) << 1) + 1);
        EL_ASSERT(_worker_name.size() < configMAX_TASK_NAME_LEN);

        // convert worker id to hex string
        _worker_name += hex_literals[worker_id >> 4];
        _worker_name += hex_literals[worker_id & 0x0f];

        [[maybe_unused]] auto ret = xTaskCreate(&Executor::c_run,
                                                _worker_name.c_str(),
                                                CONFIG_SSCMA_REPL_EXECUTOR_STACK_SIZE,
                                                this,
                                                CONFIG_SSCMA_REPL_EXECUTOR_PRIO,
                                                &_worker_handler);
        EL_ASSERT(ret == pdPASS);  // TODO: handle error
    }

    ~Executor() {
        _task_stop_requested.store(true, std::memory_order_seq_cst);
        _worker_thread_stop_requested.store(true, std::memory_order_seq_cst);
        while (_worker_thread_stop_requested.load()) yield();  // wait for destory
        vTaskDelete(_worker_handler);
    }

    // the Callable must be a function object or a lambda, the prototype is repl_task_t
    template <typename Callable> inline void add_task(Callable&& task) {
        const Guard<Mutex> guard(_task_queue_lock);
        _task_queue.push(std::forward<Callable>(task));
    }

    inline bool try_stop_task() {
        bool has_requested = !_task_stop_requested.load();
        _task_stop_requested.store(true, std::memory_order_seq_cst);
        return has_requested;
    }

    inline void cancel_all_tasks() {
        const Guard<Mutex> guard(_task_queue_lock);
        try_stop_task();
        while (!_task_queue.empty()) _task_queue.pop();
    }

   protected:
    inline void yield() const { vTaskDelay(10 / portTICK_PERIOD_MS); }

    void run() {
        while (!_worker_thread_stop_requested.load()) {
            repl_task_t task{};
            {
                const Guard<Mutex> guard(_task_queue_lock);
                if (!_task_queue.empty()) [[likely]] {
                    task = std::move(_task_queue.front());  // or std::function::swap
                    _task_queue.pop();
                }
            }  // RAII is important here
            if (task) [[likely]] {
                task(_task_stop_requested);
                if (_task_stop_requested.load()) [[unlikely]]                      // did request stop
                    _task_stop_requested.store(false, std::memory_order_seq_cst);  // reset the flag
                continue;                                                          // skip yield
            }
            _task_stop_requested.store(false, std::memory_order_seq_cst);
            yield();
        }
        _worker_thread_stop_requested.store(false, std::memory_order_seq_cst);  // reset the flag
    }

    static void c_run(void* this_pointer) { static_cast<Executor*>(this_pointer)->run(); }

   private:
    Mutex             _task_queue_lock;
    std::atomic<bool> _task_stop_requested;
    std::atomic<bool> _worker_thread_stop_requested;

    std::string  _worker_name;
    TaskHandle_t _worker_handler;

    std::queue<repl_task_t> _task_queue;
};

}  // namespace sscma::repl
