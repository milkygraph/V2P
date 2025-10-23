#include "VideoStreamer.h"
#include <iostream>
// No need to include AVFrame here anymore

VideoStreamer::VideoStreamer(std::unique_ptr<IStreamStrategy> strategy)
    : streamStrategy(std::move(strategy)),
      isRunning(false) {}

VideoStreamer::~VideoStreamer() {
    close();
}

bool VideoStreamer::open(const std::string& url) {
    if (streamStrategy) {
        if(!streamStrategy->open(url)){
            std::cerr << "Failed to open stream with URL: " << url << std::endl;
            return false;

        }
        streamStrategy->enableAudio();

        isRunning = true;
        thread = std::thread(&VideoStreamer::run, this); // Spawns the new thread
    }
    return false;
}

void VideoStreamer::run() {
    if (!streamStrategy) return;

    while (isRunning) {
        VideoFrame frame;
        PacketType packetType = streamStrategy->processNextFrame(frame);

        if (packetType == PacketType::VIDEO) {
            videoQueue.push(frame);
        }
        else if (packetType == PacketType::ERROR) {
            isRunning = false;
        }
    }
}

bool VideoStreamer::getNextVideoFrame(VideoFrame& outFrame)
{
    if (streamStrategy) {
        auto result = videoQueue.tryPop(outFrame);
        std::cout << "getNextVideoFrame result: " << result << std::endl;
        return true;
    }
    return true;
}

double VideoStreamer::getClock() const
{
    if (streamStrategy) {
        return streamStrategy->getClock();
    }
    return 0.0;
}

void VideoStreamer::close()
{
    std::cout << "Closing VideoStreamer..." << std::endl;
    if (streamStrategy) {
        streamStrategy->close();
    }

    isOpen = false;
}

void VideoStreamer::setAudioCallback(AudioCallback callback) const
{
    if (streamStrategy) {
        streamStrategy->setAudioCallback(std::move(callback));
    }
}