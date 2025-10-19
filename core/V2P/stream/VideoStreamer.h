#pragma once

#include <string>
#include <memory>

#include "IStreamStrategy.h"
#include "VideoFrame.h" // <-- Include this

/**
 * @brief The main context class that the client interacts with.
 * It holds a specific streaming strategy and delegates work to it.
 */
class VideoStreamer {
public:
    // The streamer takes ownership of the strategy
    VideoStreamer(std::unique_ptr<IStreamStrategy> strategy);
    ~VideoStreamer();

    bool open(const std::string& url) const;

    // Updated method signature
    bool getNextVideoFrame(VideoFrame& outFrame) const;

    double getClock() const;

    void setAudioCallback(AudioCallback callback) const;

    void close() const;

private:
    std::unique_ptr<IStreamStrategy> streamStrategy;
};
