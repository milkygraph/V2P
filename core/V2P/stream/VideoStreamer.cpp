#include "VideoStreamer.h"
#include <iostream>
// No need to include AVFrame here anymore

VideoStreamer::VideoStreamer(std::unique_ptr<IStreamStrategy> strategy)
    : streamStrategy(std::move(strategy)),
      isRunning(false) {}

VideoStreamer::~VideoStreamer() {
    isRunning = false;
    thread.join();
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
        return true;
    }
    return true;
}

bool VideoStreamer::updateFrame(VideoFrame& outFrame, uint32_t bufferedBytes, int bytesPerSecond)
{
    if (!streamStrategy)
        return false;

    // Try to get a video frame
    if (!videoQueue.tryPop(outFrame))
        return false;

    // --- Timing & Synchronization ---
    double videoTimestamp = outFrame.timestamp;
    if (videoTimestamp == 0.0)
        return false;

    double audioClock = streamStrategy->getClock();
    double bufferedSeconds = static_cast<double>(bufferedBytes) / static_cast<double>(bytesPerSecond);

    double actualAudioTime = audioClock - bufferedSeconds;
    double delay = videoTimestamp - actualAudioTime;

    // --- Sync thresholds ---
    constexpr double SMALL_EARLY_THRESHOLD = 0.010;
    constexpr double DROP_LATE_THRESHOLD   = -0.050;
    constexpr double MAX_WAIT_MS           = 50.0;

    // Debug info (optional)
    std::cout << "Video TS: " << videoTimestamp
              << " | Audio TS: " << actualAudioTime
              << " | Delay: " << delay << " seconds." << std::endl;

    // --- Sync logic ---
    if (delay > SMALL_EARLY_THRESHOLD) {
        return true;
    } else if (delay < DROP_LATE_THRESHOLD) {
        std::cout << "Dropping late frame.\n";
        return true;
    }

    return true; // frame ready for rendering
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
