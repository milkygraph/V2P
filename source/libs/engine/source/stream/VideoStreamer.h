#pragma once

#include <string>
#include <memory>

#include "IStreamStrategy.h"

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
    AVFrame* consumeFrame() const;
    void freeFrame(AVFrame* frame) const;
    void close() const;

private:
    std::unique_ptr<IStreamStrategy> streamStrategy;
};