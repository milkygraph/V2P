#pragma once

#include <string>

// Forward-declare FFmpeg types
struct AVFrame;

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
     * @brief Reads and decodes the next video frame from the stream.
     * @return A pointer to a new AVFrame on success, nullptr on error or end-of-stream.
     * @note The caller is responsible for freeing the AVFrame via av_frame_free().
     */
    virtual AVFrame* consumeFrame() = 0;

    /**
     * @brief Closes the stream and releases all resources.
     */
    virtual void close() = 0;

    /**
     * @brief Frees an AVFrame allocated by the strategy.
     * @param frame The AVFrame to free.
     */
    virtual void freeFrame(AVFrame* frame);
};