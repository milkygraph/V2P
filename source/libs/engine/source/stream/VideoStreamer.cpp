#include "VideoStreamer.h"

VideoStreamer::VideoStreamer(std::unique_ptr<IStreamStrategy> strategy)
    : streamStrategy(std::move(strategy)) {}

VideoStreamer::~VideoStreamer() {
    close();
}

bool VideoStreamer::open(const std::string& url) const
{
    if (streamStrategy) {
        return streamStrategy->open(url);
    }
    return false;
}

AVFrame* VideoStreamer::consumeFrame() const
{
    if (streamStrategy) {
        return streamStrategy->consumeFrame();
    }
    return nullptr;
}

void VideoStreamer::freeFrame(AVFrame* frame) const
{
    if (streamStrategy) {
        streamStrategy->freeFrame(frame);
    }
}

void VideoStreamer::close() const
{
    if (streamStrategy) {
        streamStrategy->close();
    }
}