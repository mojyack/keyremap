#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

#ifdef CUTIL_NS
namespace CUTIL_NS {
#endif

template <class T>
class Critical {
  private:
    mutable std::mutex mutex;
    T                  data;

  public:
    auto access() -> std::pair<const std::lock_guard<std::mutex>, T&> {
        return std::pair<const std::lock_guard<std::mutex>, T&>{mutex, data};
    }

    auto try_access() -> std::optional<std::pair<const std::lock_guard<std::mutex>, T&>> {
        if(mutex.try_lock()) {
            return std::optional<std::pair<const std::lock_guard<std::mutex>, T&>>{{mutex, std::adopt_lock}, data};
        }
        return std::nullopt;
    }

    auto assume_locked() -> T& {
        return data;
    }

    auto unsafe_access() -> T& {
        return data;
    }

    auto access() const -> std::pair<const std::lock_guard<std::mutex>, const T&> {
        return std::pair<const std::lock_guard<std::mutex>, const T&>{mutex, data};
    }

    auto try_access() const -> std::optional<std::pair<const std::lock_guard<std::mutex>, const T&>> {
        if(mutex.try_lock()) {
            return std::optional<const std::pair<std::lock_guard<std::mutex>, const T&>>{{mutex, std::adopt_lock}, data};
        }
        return std::nullopt;
    }

    auto assume_locked() const -> const T& {
        return data;
    }

    auto unsafe_access() const -> const T& {
        return data;
    }

    auto swap(T new_data) -> T {
        auto [lock, data] = access();
        std::swap(new_data, data);
        return new_data;
    }

    auto get_raw_mutex() -> std::mutex& {
        return mutex;
    }

    template <class... Args>
    Critical(Args&&... args) : data(std::move(args)...) {}
    Critical() {}
};

class Event {
  private:
    std::atomic_flag flag;

  public:
    auto wait() -> void {
        flag.clear();
        flag.wait(false);
    }

    auto wakeup() -> void {
        flag.test_and_set();
        flag.notify_all();
    }

    Event() = default;

    Event(const Event&) {}
};

class TimerEvent {
  private:
    std::condition_variable condv;
    Critical<bool>          waked;

  public:
    auto wait() -> void {
        waked.access().second = false;
        auto lock             = std::unique_lock<std::mutex>(waked.get_raw_mutex());
        condv.wait(lock, [this]() { return waked.assume_locked(); });
    }

    auto wait_for(auto duration) -> bool {
        waked.access().second = false;
        auto lock             = std::unique_lock<std::mutex>(waked.get_raw_mutex());
        return condv.wait_for(lock, duration, [this]() { return waked.assume_locked(); });
    }

    auto wakeup() -> void {
        waked.access().second = true;
        condv.notify_all();
    }
};

#ifdef CUTIL_NS
}
#endif
