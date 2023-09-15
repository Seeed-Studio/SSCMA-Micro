#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
    Executor(size_t worker_stack_size = REPL_EXECUTOR_STACK_SIZE, size_t worker_priority = REPL_EXECUTOR_PRIO)
        : _task_queue_lock(),
          _task_stop_requested(false),
          _worker_thread_stop_requested(false),
          _worker_ret(),
          _worker_handler(),
          _worker_name(new char[configMAX_TASK_NAME_LEN]{}),
          _worker_stack_size(worker_stack_size),
          _worker_priority(worker_priority) {
        static uint16_t worker_id = 0;
        volatile size_t length    = configMAX_TASK_NAME_LEN - 1;
        std::snprintf(_worker_name, length, "task_executor_%2X", worker_id++);
    }

    ~Executor() { stop(); }

    void start() {
        _worker_ret =
          xTaskCreate(&Executor::c_run, _worker_name, _worker_stack_size, this, _worker_priority, &_worker_handler);
    }

    void stop() {
        _worker_thread_stop_requested.store(true, std::memory_order_relaxed);
        if (_worker_ret == pdPASS) [[likely]]
            vTaskDelete(_worker_handler);
    }

    void add_task(repl_task_t task) {
        const Guard<Mutex> guard(_task_queue_lock);
        _task_queue.push(task);
        _task_stop_requested.store(true, std::memory_order_relaxed);
    }

    const char* get_worker_name() const { return _worker_name; }

   protected:
    void run() {
        while (!_worker_thread_stop_requested.load(std::memory_order_relaxed)) {
            repl_task_t task;
            {
                const Guard<Mutex> guard(_task_queue_lock);
                if (!_task_queue.empty()) {
                    task = std::move(_task_queue.front());
                    _task_queue.pop();
                    if (_task_queue.empty()) [[likely]]
                        _task_stop_requested.store(false, std::memory_order_seq_cst);
                    else
                        _task_stop_requested.store(true, std::memory_order_seq_cst);
                }
            }
            if (task) task(_task_stop_requested);
            vTaskDelay(15 / portTICK_PERIOD_MS);  // TODO: use yield
        }
    }

    static void c_run(void* this_pointer) { static_cast<Executor*>(this_pointer)->run(); }

   private:
    Mutex             _task_queue_lock;
    std::atomic<bool> _task_stop_requested;
    std::atomic<bool> _worker_thread_stop_requested;

    BaseType_t   _worker_ret;
    TaskHandle_t _worker_handler;
    char*        _worker_name;
    size_t       _worker_stack_size;
    size_t       _worker_priority;

    std::queue<repl_task_t> _task_queue;
};

}  // namespace sscma::repl
