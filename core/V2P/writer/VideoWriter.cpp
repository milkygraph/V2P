#include "VideoWriter.h"

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

VideoWriter::VideoWriter()
    : formatContext(nullptr),
      codecContext(nullptr),
      swsContext(nullptr),
      tmpFrame(nullptr),
      nextPts(0) {}

VideoWriter::~VideoWriter() {
    close();
}

bool VideoWriter::open(const std::string& filename, int width, int height, AVRational time_base, AVPixelFormat input_pix_fmt) {
    // 1. Setup Muxer
    avformat_alloc_output_context2(&formatContext, nullptr, nullptr, filename.c_str());
    if (!formatContext) {
        std::cerr << "Could not create output context." << std::endl;
        return false;
    }

    // 2. Find and setup Encoder
    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        std::cerr << "Codec 'libx264' not found." << std::endl;
        return false;
    }

    AVStream* stream = avformat_new_stream(formatContext, codec);
    if (!stream) {
        std::cerr << "Failed to allocate stream." << std::endl;
        return false;
    }

    codecContext = avcodec_alloc_context3(codec);
    if (!codecContext) {
        std::cerr << "Failed to allocate codec context." << std::endl;
        return false;
    }

    // 3. Configure Codec Context
    codecContext->width = width;
    codecContext->height = height;
    codecContext->time_base = time_base;
    stream->time_base = time_base;
    codecContext->framerate = av_inv_q(time_base);
    // H.264 requires YUV420p. We will convert frames to this format.
    codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    codecContext->bit_rate = 400000; // Example bitrate
    av_opt_set(codecContext->priv_data, "preset", "slow", 0);

    if (formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
        std::cerr << "Could not open codec." << std::endl;
        return false;
    }

    if (avcodec_parameters_from_context(stream->codecpar, codecContext) < 0) {
        std::cerr << "Could not copy codec parameters to stream." << std::endl;
        return false;
    }

    // 4. Open output file for writing
    if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file: " << filename << std::endl;
            return false;
        }
    }

    // 5. Write file header
    if (avformat_write_header(formatContext, nullptr) < 0) {
        std::cerr << "Error occurred when opening output file." << std::endl;
        return false;
    }

    // 6. Setup pixel format converter if needed
    if (input_pix_fmt != codecContext->pix_fmt) {
        swsContext = sws_getContext(width, height, input_pix_fmt,
                                    width, height, codecContext->pix_fmt,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!swsContext) {
            std::cerr << "Could not create pixel format converter." << std::endl;
            return false;
        }
        tmpFrame = av_frame_alloc();
        tmpFrame->width = width;
        tmpFrame->height = height;
        tmpFrame->format = codecContext->pix_fmt;
        if (av_frame_get_buffer(tmpFrame, 0) < 0) {
             std::cerr << "Could not allocate temp frame buffer." << std::endl;
            return false;
        }
    }

    return true;
}

bool VideoWriter::writeFrame(AVFrame* frame) {
    AVFrame* frame_to_encode = frame;

    // If pixel formats don't match, we need to convert
    if (swsContext) {
        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                  tmpFrame->data, tmpFrame->linesize);
        frame_to_encode = tmpFrame;
    }

    // Set the presentation timestamp (PTS)
    frame_to_encode->pts = nextPts++;

    return encode(frame_to_encode);
}

bool VideoWriter::close() {
    // Flush the encoder
    encode(nullptr);

    // Write the trailer
    av_write_trailer(formatContext);

    // Clean up
    if (codecContext) {
        avcodec_free_context(&codecContext);
    }
    if (formatContext) {
        if (!(formatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&formatContext->pb);
        }
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }
    if(swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if(tmpFrame) {
        av_frame_free(&tmpFrame);
        tmpFrame = nullptr;
    }

    nextPts = 0;
    return true;
}


bool VideoWriter::encode(const AVFrame* frame) {
    if (avcodec_send_frame(codecContext, frame) < 0) {
        std::cerr << "Error sending a frame for encoding." << std::endl;
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    while (true) {
        int ret = avcodec_receive_packet(codecContext, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break; // Need more input or encoding is finished
        } else if (ret < 0) {
            std::cerr << "Error during encoding." << std::endl;
            av_packet_free(&packet);
            return false;
        }

        // Rescale timestamps
        av_packet_rescale_ts(packet, codecContext->time_base, formatContext->streams[0]->time_base);
        packet->stream_index = 0;

        // Write the compressed frame to the media file
        if (av_interleaved_write_frame(formatContext, packet) < 0) {
             std::cerr << "Error while writing packet." << std::endl;
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);
    return true;
}