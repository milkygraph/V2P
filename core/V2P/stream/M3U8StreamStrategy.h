#pragma once

#include "IStreamStrategy.h"
#include "VideoFrame.h" // <-- Include this

// Forward-declare FFmpeg types
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct SwsContext;
struct SwrContext;
struct AVFrame;
struct AVPacket;

/**
 * @brief A concrete strategy for handling HLS (.m3u8) streams.
 */
class M3U8StreamStrategy : public IStreamStrategy {
public:
    M3U8StreamStrategy();
    ~M3U8StreamStrategy() override;

    bool open(const std::string& url) override;

    void setAudioCallback(AudioCallback callback) override;

    // Updated method signature
    bool getNextVideoFrame(VideoFrame& outFrame) override;

    // Get the clock from audio stream
    double getClock() override;

    void close() override;

private:
    // Helpers for initialization and cleanup
    bool initVideoStream();
    bool initAudioStream();
    void closeVideoStream();
    void closeAudioStream();

    void handleAudioPacket(AVPacket* packet);

    // Video members
    AVFormatContext* formatContext;
    AVCodecContext* videoCodecCtx;
    AVStream* videoStream;
    int videoStreamIndex;

    SwsContext* swsContext;
    AVFrame* rgbaFrame;
    uint8_t* rgbaBuffer;
    int videoWidth;
    int videoHeight;

    // Audio members
    AVCodecContext* audioCodecCtx;
    AVStream* audioStream;
    int audioStreamIndex;
    SwrContext* swrContext;
    uint8_t* audioResampleBuffer; // A buffer to hold resampled audio
    int audioResampleBufferSize;
    double audioClock; // Tracks the timestamp of the last *decoded* audio frame

    AudioCallback audioCallback; // Callback for decoded audio data
};
