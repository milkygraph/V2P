#include "VideoStreamer.h"
// No need to include AVFrame here anymore

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

// Updated implementation
bool VideoStreamer::getNextVideoFrame(VideoFrame& outFrame) const
{
    if (streamStrategy) {
        return streamStrategy->getNextVideoFrame(outFrame);
    }
    return false;
}

double VideoStreamer::getClock() const
{
    if (streamStrategy) {
        return streamStrategy->getClock();
    }
    return 0.0;
}

void VideoStreamer::close() const
{
    if (streamStrategy) {
        streamStrategy->close();
    }
}

void VideoStreamer::setAudioCallback(AudioCallback callback) const
{
    if (streamStrategy) {
        streamStrategy->setAudioCallback(std::move(callback));
    }
}
