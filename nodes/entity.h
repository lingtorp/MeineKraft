#ifndef MEINEKRAFT_ENTITY_H
#define MEINEKRAFT_ENTITY_H

#include <algorithm>
#include "../render/rendercomponent.h"
#include "transform.h"
#include "../render/render.h"
#include <functional>
#include <future>

/// Semaphore
struct Semaphore {
private:
  size_t value = 0;
  std::mutex mut;

public:
  explicit Semaphore(size_t val) : value(val) {}

  void post(const size_t val = 1) {
    std::unique_lock<std::mutex> lk(mut);
    value += val;
  }

  size_t get_value() {
    std::unique_lock<std::mutex> lk(mut);
    return value;
  }

  bool peek() {
    std::unique_lock<std::mutex> lk(mut);
    return value != 0;
  }

  /// PEek EQuals 
  bool peeq(const size_t i) {
    std::unique_lock<std::mutex> lk(mut);
    return value == i;
  }

  bool try_wait() {
    std::unique_lock<std::mutex> lk(mut);
    if (value == 0) {
      return false;
    } else {
      value--;
      return true;
    }
  }
};

struct JobSystem {
  /// Singleton instance
  static JobSystem& instance() {
    static JobSystem instance;
    return instance;
  }

  struct Worker {
    enum class WorkerState: uint8_t { Working = 0, Ready = 1, Idle = 2, Exit = 3 };
    Semaphore sem; // 0 working, 1 ready, 2 done/idle, 3 exit
    std::function<void()> workload;
    std::thread t;

    Worker(): sem(2), t(&Worker::execute, this) {}
    ~Worker() { t.join(); }

    void execute() {
      while (!sem.peeq(3)) {
        if (sem.peeq(1)) {
          sem.try_wait();
          workload();
          sem.post(2);
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
    }
  };

  std::vector<Worker> thread_pool;

  JobSystem() {
    const size_t num_threads = std::thread::hardware_concurrency() == 0 ? 4 : std::thread::hardware_concurrency();
    Log::info("JobSystem using " + std::to_string(1) + " workers");
    thread_pool = std::vector<Worker>(2);
  }
  ~JobSystem() {
    for (int i = 0; i < thread_pool.size(); i++) {
      thread_pool[i].sem.post(3);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  // Async
  ID execute(const std::function<void()>& func) {
    uint64_t i = 0;
    while (true) {
      if (thread_pool[i].sem.peeq(2)) {
        thread_pool[i].workload = std::move(func);
        thread_pool[i].sem.try_wait();
        return i;
      }
      i = (i + 1) % thread_pool.size();
    }
  }

  // Blocking
  void wait_on(const std::vector<ID>& ids) {
    for (int i = 0; i < ids.size(); i++) {
      while (thread_pool[ids[i]].sem.peeq(0)) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }
  }

  // Blocking
  void wait_on_all() {
    for (int i = 0; i < thread_pool.size(); i++) {
      while (thread_pool[i].sem.peeq(0)) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
      }
    }
  }
};
  

/*********************************************************************************/

struct ActionComponent {
  std::function<void(uint64_t, uint64_t)> action;
  ActionComponent(const std::function<void(uint64_t, uint64_t)>& action): action(action) {}
};

struct ActionSystem {
  ActionSystem() {}
  ~ActionSystem() {}
  /// Singleton instance
  static ActionSystem& instance() {
    static ActionSystem instance;
    return instance;
  }

  std::vector<ActionComponent> components;

  void add_component(const ActionComponent& component) {
    components.emplace_back(component);
  }

  void remove_component(const ID id) {
    // TODO: Implement
  }

  void execute_actions(const uint64_t frame, const uint64_t dt) {
    for (const auto& component : components) {
      component.action(frame, dt);
    }
  }
};

/*********************************************************************************/

// Mapping: Entity ID <--> Alive?
struct EntitySystem {
private:
  std::vector<ID> entities;
  std::unordered_map<ID, ID> lut;

public:
  EntitySystem() {}
  ~EntitySystem() {}
  /// Singleton instance
  static EntitySystem& instance() {
    static EntitySystem instance;
    return instance;
  }
  
  /// Generates a new Entity id to be used when identifying this instance of Entity
  ID new_entity() const {
    // TODO: Implement something for real
    static uint64_t e_id = 1; // 0 ID is usually (default value for ints..) used for invalid IDs in systems
    return e_id++;
  };

  /// Lookup if the Entity is alive
  bool lookup(ID entity) const {

  }

  // Map entity ID to the right bitflag
  void destroy_entity(const ID& id) {
    // TODO: Implement by removing all the components owned by the Entity (again this is mainly a convenicence thing)
  }
};

/// Game object 
struct Entity {
    ID id;

    Entity(): id(EntitySystem::instance().new_entity()) {}
    ~Entity() {
      EntitySystem::instance().destroy_entity(id);
    }

    /** Component handling for convenience **/
    inline void attach_component(const RenderComponent& component) {
      Renderer::instance().add_component(component, id);
    }

    inline void attach_component(const TransformComponent& component) {
      TransformSystem::instance().add_component(component, id);
    }

    inline void attach_component(const ActionComponent& component) {
      ActionSystem::instance().add_component(component);
    }

    inline void deattach_component(const RenderComponent& component) {
      Renderer::instance().remove_component(id);
    }

    inline void deattach_component(const TransformComponent& component) {
      TransformSystem::instance().remove_component(id);
    }

    inline void deattach_component(const ActionComponent& component) {
      ActionSystem::instance().remove_component(id);
    }
};

#endif // MEINEKRAFT_ENTITY_H
