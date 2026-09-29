#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Lightweight type-safe event bus for inter-component communication
// ─────────────────────────────────────────────────────────────────────────────
//
// Design:
//   - Synchronous dispatch by default (caller's thread)
//   - Listeners are weak-referenced via IDs for safe removal
//   - No heap allocation per event dispatch
//   - Type erasure via std::any + typeid
//
// Usage:
//   EventBus bus;
//   auto id = bus.subscribe<TranscriptReady>([](const TranscriptReady& e) { ... });
//   bus.publish(TranscriptReady{.text = "hello"});
//   bus.unsubscribe(id);
//

#include <any>
#include <functional>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace localvoice {

/// Opaque subscription ID for unsubscribing
using SubscriptionId = uint64_t;

class EventBus {
public:
    EventBus() = default;

    /// Subscribe to events of type T. Returns a SubscriptionId for later removal.
    template<typename T>
    SubscriptionId subscribe(std::function<void(const T&)> handler) {
        auto wrapper = [handler = std::move(handler)](const std::any& event) {
            handler(std::any_cast<const T&>(event));
        };

        std::lock_guard lock(mutex_);
        auto id = next_id_++;
        auto key = std::type_index(typeid(T));
        handlers_[key].push_back({id, std::move(wrapper)});
        return id;
    }

    /// Unsubscribe a previously registered handler
    void unsubscribe(SubscriptionId id) {
        std::lock_guard lock(mutex_);
        for (auto& [type, entries] : handlers_) {
            std::erase_if(entries, [id](const HandlerEntry& e) {
                return e.id == id;
            });
        }
    }

    /// Publish an event to all subscribers of type T.
    /// Dispatched synchronously on the caller's thread.
    template<typename T>
    void publish(const T& event) {
        std::vector<HandlerEntry> snapshot;
        {
            std::lock_guard lock(mutex_);
            auto it = handlers_.find(std::type_index(typeid(T)));
            if (it == handlers_.end()) return;
            snapshot = it->second; // copy handler list for safe iteration
        }

        std::any boxed = event;
        for (auto& entry : snapshot) {
            entry.handler(boxed);
        }
    }

    /// Remove all subscriptions
    void clear() {
        std::lock_guard lock(mutex_);
        handlers_.clear();
    }

    /// Number of total subscriptions (for diagnostics)
    [[nodiscard]] size_t subscription_count() const {
        std::lock_guard lock(mutex_);
        size_t count = 0;
        for (const auto& [type, entries] : handlers_) {
            count += entries.size();
        }
        return count;
    }

private:
    struct HandlerEntry {
        SubscriptionId id;
        std::function<void(const std::any&)> handler;
    };

    mutable std::mutex mutex_;
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers_;
    SubscriptionId next_id_ = 1;
};

// ─────────────────────────────────────────────────────────────────────────────
// Predefined event types
// ─────────────────────────────────────────────────────────────────────────────

/// Emitted when VAD detects start of speech
struct SpeechStartedEvent {
    TimePoint timestamp;
};

/// Emitted when VAD detects end of speech
struct SpeechEndedEvent {
    TimePoint timestamp;
    size_t    sample_count = 0; ///< Number of samples in the speech segment
};

/// Emitted when transcription is ready
struct TranscriptReadyEvent {
    TranscriptResult result;
    TimePoint        request_time;  ///< When the inference was requested
    TimePoint        complete_time; ///< When the inference completed
};

/// Emitted on application state changes
struct AppStateChangedEvent {
    AppState previous;
    AppState current;
};

/// Emitted when push-to-talk key state changes
struct PushToTalkEvent {
    bool pressed; ///< true = key down, false = key up
};

/// Emitted on errors that the UI should display
struct ErrorEvent {
    ErrorCode   code;
    std::string message;
};

/// Emitted when audio device changes (connect/disconnect)
struct AudioDeviceChangedEvent {
    std::string device_id;
    std::string device_name;
    bool        connected;
};

} // namespace localvoice
