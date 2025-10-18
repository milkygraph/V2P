#include "M3U8StreamStrategy.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

M3U8StreamStrategy::M3U8StreamStrategy()
    : formatContext(nullptr),
      codecContext(nullptr),
      videoStreamIndex(-1) {}

M3U8StreamStrategy::~M3U8StreamStrategy() {
    close();
}

bool M3U8StreamStrategy::open(const std::string& url) {
    formatContext = avformat_alloc_context();
    if (!formatContext) {
        std::cerr << "Could not allocate format context." << std::endl;
        return false;
    }

    if (avformat_open_input(&formatContext, url.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open stream URL: " << url << std::endl;
        avformat_free_context(formatContext); // Must free on failure
        formatContext = nullptr;
        return false;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        close();
        return false;
    }

    av_dump_format(formatContext, 0, url.c_str(), 0);

    const AVCodec* videoCodec = nullptr;
    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (videoStreamIndex < 0 || !videoCodec) {
        std::cerr << "Could not find a video stream or a suitable decoder." << std::endl;
        close();
        return false;
    }

    codecContext = avcodec_alloc_context3(videoCodec);
    if (!codecContext) {
        std::cerr << "Could not allocate codec context." << std::endl;
        close();
        return false;
    }

    AVCodecParameters* codecParams = formatContext->streams[videoStreamIndex]->codecpar;
    if (avcodec_parameters_to_context(codecContext, codecParams) < 0) {
        std::cerr << "Could not copy codec parameters to context." << std::endl;
        close();
        return false;
    }

    if (avcodec_open2(codecContext, videoCodec, nullptr) < 0) {
        std::cerr << "Could not open codec." << std::endl;
        close();
        return false;
    }

    std::cout << "M3U8 Stream Strategy opened successfully." << std::endl;
    return true;
}

AVFrame* M3U8StreamStrategy::consumeFrame() {
    if (!formatContext || !codecContext) {
        return nullptr;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    while (av_read_frame(formatContext, packet) >= 0) {
        if (packet->stream_index == videoStreamIndex) {
            if (avcodec_send_packet(codecContext, packet) == 0) {
                int ret = avcodec_receive_frame(codecContext, frame);
                if (ret == 0) {
                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    return frame;
                }
                if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    std::cerr << "Error while receiving a frame from the decoder." << std::endl;
                }
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&frame);
    return nullptr;
}

void M3U8StreamStrategy::close() {
    if (codecContext) {
        avcodec_free_context(&codecContext);
        codecContext = nullptr;
    }
    if (formatContext) {
        avformat_close_input(&formatContext);
        formatContext = nullptr;
    }
    videoStreamIndex = -1;
    std::cout << "M3U8 Stream Strategy closed." << std::endl;
}

void IStreamStrategy::freeFrame(AVFrame* frame) {
    if (frame) {
        av_frame_free(&frame);
    }
}