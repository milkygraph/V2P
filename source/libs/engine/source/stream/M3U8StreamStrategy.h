#pragma once

#include "IStreamStrategy.h"

// Forward-declare FFmpeg types
struct AVFormatContext;
struct AVCodecContext;

/**
 * @brief A concrete strategy for handling HLS (.m3u8) streams.
 */
class M3U8StreamStrategy : public IStreamStrategy {
public:
    M3U8StreamStrategy();
    ~M3U8StreamStrategy() override;

    bool open(const std::string& url) override;
    AVFrame* consumeFrame() override;
    void close() override;

private:
    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    int videoStreamIndex;
};