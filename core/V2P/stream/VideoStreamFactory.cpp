#include "VideoStreamFactory.h"
#include "M3U8StreamStrategy.h"
#include <iostream>

std::unique_ptr<VideoStreamer> VideoStreamFactory::createVideoStreamer(const std::string& url)
{
    std::unique_ptr <VideoStreamer> streamer = nullptr;

    // For simplicity, we assume all URLs ending with .m3u8 are HLS streams.
    if (url.find(".m3u8") != std::string::npos)
    {
        streamer = std::make_unique<VideoStreamer>(
            std::make_unique<M3U8StreamStrategy>());
    }


    if (streamer != nullptr)
    {
        std::cout << "Opening stream with URL: " << url << std::endl;
        streamer->open(url);
        return streamer;
    }
    return nullptr; // Unsupported URL
}
