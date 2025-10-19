#include "M3U8StreamStrategy.h"
#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

M3U8StreamStrategy::M3U8StreamStrategy()
    : formatContext(nullptr),
    videoCodecCtx(nullptr),
    videoStream(nullptr),
    videoStreamIndex(-1),
    swsContext(nullptr),
    rgbaBuffer(nullptr),
    videoWidth(0),
    videoHeight(0),
    audioCodecCtx(nullptr),
    audioStream(nullptr),
    audioStreamIndex(-1),
    swrContext(nullptr),
    audioResampleBuffer(nullptr),
    audioResampleBufferSize(0),
    audioClock(0.0) {}

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
        close(); // Use our own close() to clean up
        return false;
    }

    av_dump_format(formatContext, 0, url.c_str(), 0);

    // --- Call our new helpers ---
    if (!initVideoStream()) {
        std::cerr << "Failed to initialize video stream." << std::endl;
        close();
        return false;
    }

    if (!initAudioStream()) {
        std::cerr << "Failed to initialize audio stream." << std::endl;
        close();
        return false;
    }

    std::cout << "M3U8 Stream Strategy opened successfully." << std::endl;
    return true;
}

void M3U8StreamStrategy::setAudioCallback(AudioCallback callback)
{
    audioCallback = std::move(callback);
}


bool M3U8StreamStrategy::initVideoStream() {
    const AVCodec* videoCodec = nullptr;
    videoStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);

    if (videoStreamIndex < 0 || !videoCodec) {
        std::cerr << "Could not find a video stream or a suitable decoder." << std::endl;
        return false;
    }

    videoStream = formatContext->streams[videoStreamIndex];
    videoCodecCtx = avcodec_alloc_context3(videoCodec);
    if (!videoCodecCtx) {
        std::cerr << "Could not allocate video codec context." << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(videoCodecCtx, videoStream->codecpar) < 0) {
        std::cerr << "Could not copy video codec parameters to context." << std::endl;
        return false;
    }

    if (avcodec_open2(videoCodecCtx, videoCodec, nullptr) < 0) {
        std::cerr << "Could not open video codec." << std::endl;
        return false;
    }

    // --- Setup the RGBA Converter ---
    videoWidth = videoCodecCtx->width;
    videoHeight = videoCodecCtx->height;

    swsContext = sws_getContext(
        videoWidth, videoHeight, videoCodecCtx->pix_fmt, // Source
        videoWidth, videoHeight, AV_PIX_FMT_RGBA,      // Destination
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!swsContext) {
        std::cerr << "Could not initialize SwsContext." << std::endl;
        return false;
    }

    rgbaFrame = av_frame_alloc();
    if (!rgbaFrame) {
        std::cerr << "Could not allocate RGBA frame." << std::endl;
        return false;
    }

    rgbaFrame->format = AV_PIX_FMT_RGBA;
    rgbaFrame->width = videoWidth;
    rgbaFrame->height = videoHeight;

    int rgbaBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);
    rgbaBuffer = (uint8_t*)av_malloc(rgbaBufferSize * sizeof(uint8_t));
    if (!rgbaBuffer) {
        std::cerr << "Could not allocate RGBA buffer." << std::endl;
        return false;
    }

    av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, rgbaBuffer,
                         AV_PIX_FMT_RGBA, videoWidth, videoHeight, 1);

    return true;
}

// Define our target audio format (you can move this to the top of the .cpp file)
#define TARGET_SAMPLE_RATE 44100
#define TARGET_SAMPLE_FORMAT AV_SAMPLE_FMT_S16

const AVChannelLayout TARGET_CHANNEL_LAYOUT = AV_CHANNEL_LAYOUT_STEREO;

bool M3U8StreamStrategy::initAudioStream() {
    const AVCodec* audioCodec = nullptr;
    audioStreamIndex = av_find_best_stream(formatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &audioCodec, 0);

    if (audioStreamIndex < 0 || !audioCodec) {
        std::cerr << "Could not find an audio stream or decoder." << std::endl;
        return false;
    }

    audioStream = formatContext->streams[audioStreamIndex];
    audioCodecCtx = avcodec_alloc_context3(audioCodec);
    if (!audioCodecCtx) {
        std::cerr << "Could not allocate audio codec context." << std::endl;
        return false;
    }

    if (avcodec_parameters_to_context(audioCodecCtx, audioStream->codecpar) < 0) {
        std::cerr << "Could not copy audio codec parameters." << std::endl;
        return false;
    }

    if (avcodec_open2(audioCodecCtx, audioCodec, nullptr) < 0) {
        std::cerr << "Could not open audio codec." << std::endl;
        return false;
    }

    // --- Setup Audio Resampler (SwrContext) ---
    swrContext = swr_alloc();
    if (!swrContext) {
        std::cerr << "Could not allocate resampler context." << std::endl;
        return false;
    }

    av_opt_set_chlayout(swrContext, "in_channel_layout",  &audioCodecCtx->ch_layout, 0);
    av_opt_set_sample_fmt(swrContext,     "in_sample_fmt",      audioCodecCtx->sample_fmt, 0);
    av_opt_set_int(swrContext,            "in_sample_rate",     audioCodecCtx->sample_rate, 0);

    av_opt_set_chlayout(swrContext, "out_channel_layout", &TARGET_CHANNEL_LAYOUT, 0);
    av_opt_set_sample_fmt(swrContext,     "out_sample_fmt",     TARGET_SAMPLE_FORMAT, 0);
    av_opt_set_int(swrContext,            "out_sample_rate",    TARGET_SAMPLE_RATE, 0);

    if (swr_init(swrContext) < 0) {
        std::cerr << "Failed to initialize the resampler context." << std::endl;
        return false;
    }

    audioResampleBufferSize = av_samples_get_buffer_size(nullptr, TARGET_CHANNEL_LAYOUT.nb_channels, 4096, TARGET_SAMPLE_FORMAT, 1);
    audioResampleBuffer = (uint8_t*)av_malloc(audioResampleBufferSize);
    if (!audioResampleBuffer) {
        std::cerr << "Could not allocate audio resample buffer." << std::endl;
        return false;
    }

    return true;
}


void M3U8StreamStrategy::closeVideoStream() {
    if (videoCodecCtx) {
        avcodec_free_context(&videoCodecCtx);
        videoCodecCtx = nullptr;
    }
    if (swsContext) {
        sws_freeContext(swsContext);
        swsContext = nullptr;
    }
    if (rgbaBuffer) {
        av_free(rgbaBuffer);
        rgbaBuffer = nullptr;
    }
    if (rgbaFrame) {
        av_frame_free(&rgbaFrame);
        rgbaFrame = nullptr;
    }
    videoStream = nullptr;
    videoStreamIndex = -1;
    videoWidth = 0;
    videoHeight = 0;
}

void M3U8StreamStrategy::closeAudioStream() {
    if (audioCodecCtx) {
        avcodec_free_context(&audioCodecCtx);
        audioCodecCtx = nullptr;
    }
    if (swrContext) {
        swr_free(&swrContext);
        swrContext = nullptr;
    }
    if (audioResampleBuffer) {
        av_free(audioResampleBuffer);
        audioResampleBuffer = nullptr;
    }
    audioStream = nullptr;
    audioStreamIndex = -1;
    audioResampleBufferSize = 0;
    audioClock = 0.0;
}


bool M3U8StreamStrategy::getNextVideoFrame(VideoFrame& outFrame) {
    if (!formatContext || !videoCodecCtx || !swsContext) {
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* yuvFrame = av_frame_alloc();

    // This loop now handles BOTH audio and video
    while (av_read_frame(formatContext, packet) >= 0) {

        if (packet->stream_index == videoStreamIndex) {
            // --- VIDEO PACKET ---
            if (avcodec_send_packet(videoCodecCtx, packet) == 0) {
                int ret = avcodec_receive_frame(videoCodecCtx, yuvFrame);

                if (ret == 0) {
                    // --- We have a video frame! ---
                    sws_scale(
                        swsContext,
                        yuvFrame->data, yuvFrame->linesize,
                        0, videoHeight,
                        rgbaFrame->data, rgbaFrame->linesize
                    );

                    outFrame.width = videoWidth;
                    outFrame.height = videoHeight;
                    outFrame.timestamp = (double)yuvFrame->pts * av_q2d(videoStream->time_base);

                    int bufferSize = videoWidth * videoHeight * 4; // RGBA
                    outFrame.data.resize(bufferSize);
                    memcpy(outFrame.data.data(), rgbaBuffer, bufferSize);

                    av_packet_unref(packet);
                    av_packet_free(&packet);
                    av_frame_free(&yuvFrame);
                    return true; // We return to the main loop to render
                }
            }
        }
        else if (packet->stream_index == audioStreamIndex) {
            handleAudioPacket(packet);
        }

        av_packet_unref(packet);
    }

    // End of stream or error
    av_packet_free(&packet);
    av_frame_free(&yuvFrame);
    return false;
}

void M3U8StreamStrategy::handleAudioPacket(AVPacket* packet) {
    if (!audioCodecCtx || !swrContext) {
        // Can't process audio if it wasn't initialized
        return;
    }

    // Send the raw packet to the decoder
    if (avcodec_send_packet(audioCodecCtx, packet) < 0) {
        std::cerr << "Error sending audio packet to decoder." << std::endl;
        return;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate audio frame." << std::endl;
        return;
    }

    // Receive all decoded frames from the packet
    while (avcodec_receive_frame(audioCodecCtx, frame) == 0) {
        // We have a decoded audio frame (likely in a planar format)

        // --- 1. Resample it to our target format (16-bit stereo) ---
        int resampled_data_size = swr_convert(
            swrContext,
            &audioResampleBuffer,       // [out] The buffer to fill
            audioResampleBufferSize,    // [in]  The max size of our buffer
            (const uint8_t**)frame->data, // [in]  The input audio data
            frame->nb_samples             // [in]  Number of input samples
        );

        if (resampled_data_size < 0) {
            std::cerr << "Error during audio resampling" << std::endl;
            continue; // Continue to next frame
        }

        // --- 2. Calculate the size in bytes of the resampled data ---
        int bytes_to_queue = resampled_data_size * TARGET_CHANNEL_LAYOUT.nb_channels * av_get_bytes_per_sample(TARGET_SAMPLE_FORMAT);

        // --- 3. Push the audio bytes into SDL's queue ---
        // This is the magic step. SDL will take this data and play it on its own thread.
        // The first argument '1' is the audio device ID, which SDLWindow opens.
        if (audioCallback(audioResampleBuffer, bytes_to_queue)) {
            std::cerr << "Audio callback failed to queue audio data." << std::endl;
        }

        // --- 4. Update the audio clock ---
        // This is crucial for A/V sync. We track the timestamp of the last
        // audio frame we successfully processed.
        if (frame->pts != AV_NOPTS_VALUE) {
            audioClock = (double)frame->pts * av_q2d(audioStream->time_base);
        }
    }

    av_frame_free(&frame);
}



void M3U8StreamStrategy::close() {
    closeVideoStream();
    closeAudioStream();

    if (formatContext) {
        avformat_close_input(&formatContext);
        avformat_free_context(formatContext);
        formatContext = nullptr;
    }

    std::cout << "M3U8 Stream Strategy closed." << std::endl;
}

double M3U8StreamStrategy::getClock() {
    return audioClock;
}
