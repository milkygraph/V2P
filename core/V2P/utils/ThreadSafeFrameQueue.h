#pragma once

#include <V2P/stream/VideoFrame.h>
#include <queue>
#include <mutex>
#include <condition_variable>

class ThreadSafeFrameQueue {
public:
    explicit ThreadSafeFrameQueue(size_t maxSize = 30)
        : maxSize(maxSize) {}

    /**
     * @brief Push a new frame into the queue.
     * Blocks if queue is full until space becomes available.
     */
    void push(const VideoFrame& frame) {
        std::unique_lock<std::mutex> lock(mutex);
        condFull.wait(lock, [this]() { return queue.size() < maxSize; });

        queue.push(frame);
        condEmpty.notify_one();
    }

    /**
     * @brief Tries to pop a frame. Blocks until a frame is available.
     * @param outFrame The destination for the popped frame.
     * @return True if a frame was popped, false if stopped.
     */
    bool pop(VideoFrame& outFrame) {
        std::unique_lock<std::mutex> lock(mutex);
        condEmpty.wait(lock, [this]() { return !queue.empty() || stopped; });

        if (stopped && queue.empty())
            return false;

        outFrame = std::move(queue.front());
        queue.pop();
        condFull.notify_one();
        return true;
    }

    /**
     * @brief Try popping without blocking (non-blocking version).
     * @return True if frame was available, false otherwise.
     */
    bool tryPop(VideoFrame& outFrame) {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty())
            return false;

        outFrame = std::move(queue.front());
        queue.pop();
        condFull.notify_one();
        return true;
    }

    /**
     * @brief Clear the queue.
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        std::queue<VideoFrame> empty;
        std::swap(queue, empty);
    }

    /**
     * @brief Stop the queue (wakes up any waiting threads).
     */
    void stop() {
        std::lock_guard<std::mutex> lock(mutex);
        stopped = true;
        condEmpty.notify_all();
        condFull.notify_all();
    }

    /**
     * @brief Current number of frames in the queue.
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.size();
    }

private:
    mutable std::mutex mutex;
    std::condition_variable condEmpty;
    std::condition_variable condFull;
    std::queue<VideoFrame> queue;
    size_t maxSize;
    bool stopped = false;
};
