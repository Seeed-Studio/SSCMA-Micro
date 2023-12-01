#pragma once

#include <algorithm>
#include <cstdint>
#include <list>

#include "core/synchronize/el_guard.hpp"
#include "core/synchronize/el_mutex.hpp"
#include "porting/el_misc.h"
#include "sscma/definations.hpp"
#include "sscma/prototypes.hpp"

namespace sscma {

namespace types {

using namespace sscma::prototypes;

struct SupervisedObject {
    Supervisable* object;
    uint16_t      priority;
};

}  // namespace types

namespace repl {

using namespace edgelab;

using namespace sscma::types;
using namespace sscma::prototypes;

class Supervisor {
   public:
    [[nodiscard]] static Supervisor* get_supervisor() {
        static Supervisor supervisor;
        return &supervisor;
    }

    ~Supervisor() noexcept = default;

    void register_supervised_object(Supervisable* object, uint16_t priority) {
        const Guard<Mutex> guard(_supervised_objects_lock);
        _supervised_objects.remove_if(
          [object](const SupervisedObject& supervised_object) { return supervised_object.object == object; });
        auto it = std::upper_bound(_supervised_objects.begin(),
                                   _supervised_objects.end(),
                                   priority,
                                   [](uint16_t lfs, const SupervisedObject& rhs) { return lfs < rhs.priority; });
        _supervised_objects.insert(it, {object, priority});
    }

    void unregister_supervised_object(Supervisable* object) {
        const Guard<Mutex> guard(_supervised_objects_lock);
        _supervised_objects.remove_if(
          [object](const SupervisedObject& supervised_object) { return supervised_object.object == object; });
    }

    decltype(auto) get_supervised_objects() const {
        const Guard<Mutex> guard(_supervised_objects_lock);
        return _supervised_objects;
    }

   protected:
    Supervisor() noexcept {
        [[maybe_unused]] auto ret = xTaskCreate(&Supervisor::c_run,
                                                SSCMA_REPL_SUPERVISOR_NAME,
                                                SSCMA_REPL_SUPERVISOR_STACK_SIZE,
                                                this,
                                                SSCMA_REPL_SUPERVISOR_PRIO,
                                                nullptr);
        EL_ASSERT(ret == pdPASS);  // TODO: handle error
    }

    void run() {
    Loop: {
        _supervised_objects_lock.lock();
        for (const auto& supervised_object : _supervised_objects) supervised_object.object->poll_from_supervisor();
        _supervised_objects_lock.unlock();
        el_sleep(SSCMA_REPL_SUPERVISOR_POLL_DELAY);
    }
        goto Loop;
    }

    static void c_run(void* this_pointer) { static_cast<Supervisor*>(this_pointer)->run(); }

   private:
    std::list<SupervisedObject> _supervised_objects;
    Mutex                       _supervised_objects_lock;
};

}  // namespace repl

}  // namespace sscma
