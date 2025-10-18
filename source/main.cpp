#include <iostream>
#include <vector>
#include <libavutil/frame.h>

#include "source/stream/VideoStreamFactory.h"
#include "source/writer/VideoWriter.h"

int main()
{
    auto streamer = VideoStreamFactory::createVideoStreamer(
        "https://fast-tailor.3catdirectes.cat/v1/channel/ccma-channel2/hls.m3u8");
    auto frames = std::vector<AVFrame*>();

    for (int i = 0; i < 100; ++i)
    {
        AVFrame* frame = streamer->consumeFrame();
        if (!frame)
        {
            std::cerr << "null frame" << std::endl;
            break; // End of stream or error
        }

        frames.push_back(frame);
    }

    AVFrame* first_frame = frames[0];
    auto writer = VideoWriter();
    writer.open("output.mp4", first_frame->width, first_frame->height, AVRational{1, 25}, static_cast<AVPixelFormat>(first_frame->format));

    for (auto& frame : frames)
    {
        writer.writeFrame(frame);
        streamer->freeFrame(frame);
    }
}