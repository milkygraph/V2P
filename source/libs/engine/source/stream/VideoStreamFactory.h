#pragma once
#include "VideoStreamer.h"

class VideoStreamFactory
{
public:
    static std::unique_ptr<VideoStreamer> createVideoStreamer(const std::string& url);
};