#pragma once

#include <string>
#include <functional>

#include "V2P/stream/Packet.h"
#include "V2P/stream/VideoFrame.h"
#include "V2P/utils/ThreadSafeFrameQueue.h"


using AudioCallback = std::function<bool(uint8_t* data, int size)>;

/**
 * @brief Defines the interface for a streaming strategy.
 * Each concrete strategy (e.g., for HLS, RTMP, files) will implement this.
 */
class IStreamStrategy {
public:
    virtual ~IStreamStrategy() = default;

    /**
     * @brief Opens a stream from the given URL.
     * @param url The direct URL of the media stream.
     * @return True on success, false on failure.
     */
    virtual bool open(const std::string& url) = 0;

    /**
     * @brief Sets the audio callback function to handle decoded audio data.
     * @param callback The function to be called with audio data.
     */
    virtual void setAudioCallback(AudioCallback callback) = 0;

    /**
     * @brief Reads, decodes, and converts the next video frame from the stream.
     * @param outFrame [out] The VideoFrame struct to be filled with data.
     * @return True on success, false on error or end-of-stream.
     */
    virtual PacketType processNextFrame(VideoFrame& outFrame) = 0;

    /**
     * @brief Gets the current playback clock in milliseconds.
     * @return The current playback time in milliseconds.
     */
    virtual double getClock() = 0;

    /**
     * @brief Closes the stream and releases all resources.
     */
    virtual void close() = 0;

    void enableAudio() { isAudioEnabled = true; }
    void disableAudio() { isAudioEnabled = false; }
private:
    ThreadSafeFrameQueue frameQueue;

protected:
    bool isAudioEnabled = false;
};
