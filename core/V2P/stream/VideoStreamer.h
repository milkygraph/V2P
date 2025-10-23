#pragma once

#include <string>
#include <memory>
#include <thread>

#include "IStreamStrategy.h"
#include "VideoFrame.h"

/**
 * @brief The main context class that the client interacts with.
 * It holds a specific streaming strategy and delegates work to it.
 */
class VideoStreamer {
public:
    // The streamer takes ownership of the strategy
    VideoStreamer(std::unique_ptr<IStreamStrategy> strategy);
    ~VideoStreamer();

    bool open(const std::string& url);

    bool getNextVideoFrame(VideoFrame& outFrame);

    double getClock() const;

    void setAudioCallback(AudioCallback callback) const;

    void close();

    void enableAudio() { streamStrategy->enableAudio(); }
    void disableAudio() { streamStrategy->disableAudio(); }

private:
    std::unique_ptr<IStreamStrategy> streamStrategy;

    void run(); // worker thread function
    bool isOpen = false;

    ThreadSafeFrameQueue videoQueue; // The bridge
    std::thread thread;
    std::atomic<bool> isRunning;
    std::atomic<double> audioClock;
};
