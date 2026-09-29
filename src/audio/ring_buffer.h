#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Lock-free Single-Producer Single-Consumer ring buffer
// ─────────────────────────────────────────────────────────────────────────────
//
// Design constraints:
//   - No heap allocation after construction
//   - No mutexes (uses atomic indices)
//   - Suitable for audio callback → processing thread transfer
//   - Fixed capacity, set at construction
//   - Overwrites oldest data on overflow (configurable)
//

#include <atomic>
#include <cassert>
#include <cstring>
#include <vector>
#include <span>
#include <cstddef>

namespace localvoice {

template<typename T>
class RingBuffer {
public:
    /// Construct with fixed capacity. Memory is allocated once here.
    explicit RingBuffer(size_t capacity)
        : buffer_(capacity)
        , capacity_(capacity) {
        assert(capacity > 0);
    }

    /// Write samples into the ring buffer.
    /// Returns the number of samples actually written.
    /// If the buffer is full, oldest data is silently overwritten.
    size_t write(std::span<const T> data) noexcept {
        const size_t count = data.size();
        if (count == 0 || capacity_ == 0) return 0;

        size_t w = write_pos_.load(std::memory_order_relaxed);
        const size_t r = read_pos_.load(std::memory_order_acquire);

        for (size_t i = 0; i < count; ++i) {
            buffer_[w % capacity_] = data[i];
            w++;
        }

        write_pos_.store(w, std::memory_order_release);

        // If we wrote past the read position, advance it (overflow)
        if (w - r > capacity_) {
            read_pos_.store(w - capacity_, std::memory_order_release);
            overflow_count_.fetch_add(1, std::memory_order_relaxed);
        }

        return count;
    }

    /// Read up to max_count samples into the output buffer.
    /// Returns the number of samples actually read.
    size_t read(std::span<T> output) noexcept {
        const size_t max_count = output.size();
        if (max_count == 0) return 0;

        size_t r = read_pos_.load(std::memory_order_relaxed);
        const size_t w = write_pos_.load(std::memory_order_acquire);

        const size_t avail = w - r;
        const size_t to_read = (avail < max_count) ? avail : max_count;

        for (size_t i = 0; i < to_read; ++i) {
            output[i] = buffer_[(r + i) % capacity_];
        }

        read_pos_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

    /// Number of samples available for reading
    [[nodiscard]] size_t available() const noexcept {
        auto w = write_pos_.load(std::memory_order_acquire);
        auto r = read_pos_.load(std::memory_order_acquire);
        return w - r;
    }

    /// Total capacity
    [[nodiscard]] size_t capacity() const noexcept { return capacity_; }

    /// Number of overflow events (diagnostic)
    [[nodiscard]] size_t overflow_count() const noexcept {
        return overflow_count_.load(std::memory_order_relaxed);
    }

    /// Reset to empty state. NOT thread-safe — call only when no readers/writers.
    void reset() noexcept {
        write_pos_.store(0, std::memory_order_relaxed);
        read_pos_.store(0, std::memory_order_relaxed);
    }

private:
    std::vector<T>      buffer_;
    size_t              capacity_;
    std::atomic<size_t> write_pos_{0};
    std::atomic<size_t> read_pos_{0};
    std::atomic<size_t> overflow_count_{0};
};

} // namespace localvoice
