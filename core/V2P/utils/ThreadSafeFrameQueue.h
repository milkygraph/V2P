#pragma once

#include <V2P/stream/VideoFrame.h>
#include <iostream>
#include <mutex>
#include <optional>

class ThreadSafeFrameQueue {
public:
    /**
     * @brief Pushes a new frame, overwriting the old one.
     * Called by the Streaming Thread.
     */
    void push(const VideoFrame& frame) {
        std::cout << "Writing the video frame" << std::endl;
        std::lock_guard<std::mutex> lock(mutex);
        this->frame = frame;
        hasNewFrame = true;
    }

    /**
     * @brief Tries to pop the latest frame.
     * @param outFrame The frame to be filled.
     * @return True if a new, unread frame was popped, false otherwise.
     */
    bool tryPop(VideoFrame& outFrame) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!hasNewFrame) {
            return false; // No new frame to show
        }

        // We have a new frame. Copy it out and mark it as "read".
        outFrame = frame.value(); 
        hasNewFrame = false;
        return true;
    }

private:
    std::mutex mutex;
    std::optional<VideoFrame> frame;
    bool hasNewFrame = false;
};
