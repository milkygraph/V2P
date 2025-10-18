#include "engine.hpp"

int check_ffmpeg()
{
    std::cout << "FFmpeg avformat version: "
        << LIBAVFORMAT_VERSION_MAJOR << std::endl;

    std::cout << "Hello, FFmpeg!" << std::endl;
    return 0;
}

AVFormatContext* test_formatContext;

int read_test_stream(int test_packets)
{
    // 1. PASTE THE STREAM URL YOU COPIED FROM THE M3U FILE HERE
    const char* streamUrl = "https://fast-tailor.3catdirectes.cat/v1/channel/ccma-channel2/hls.m3u8";

    AVFormatContext* test_formatContext = avformat_alloc_context();
    if (!test_formatContext)
    {
        // Error: Could not allocate context
        return -1;
    }

    AVDictionary* options = nullptr;


    // 1. Open the stream
    // This will now open the HLS stream
    int ret = avformat_open_input(&test_formatContext, streamUrl, nullptr, &options);
    av_dict_free(&options); // Free the options dictionary
    if (ret < 0)
    {
        // Error: Could not open stream
        avformat_free_context(test_formatContext);
        return -1;
    }

    // 2. Find stream info (populates formatContext->streams)
    if (avformat_find_stream_info(test_formatContext, nullptr) < 0)
    {
        // Error: Could not find stream info
        avformat_close_input(&test_formatContext);
        return -1;
    }

    // Optional: Dump format info to console for debugging
    av_dump_format(test_formatContext, 0, streamUrl, 0);

    // ...
    // The rest of your code (finding stream, codec, and the read loop)
    // will work exactly as it did before.
    // ...

    // (Find video stream, open codec, etc...)

    int videoStreamIndex = -1;
    AVCodecParameters* videoCodecParams = nullptr;
    const AVCodec* videoCodec = nullptr;

    for (unsigned int i = 0; i < test_formatContext->nb_streams; ++i)
    {
        if (test_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            videoCodecParams = test_formatContext->streams[i]->codecpar;
            videoCodec = avcodec_find_decoder(videoCodecParams->codec_id);
            if (!videoCodec) { break; }
            break;
        }
    }

    if (videoStreamIndex == -1 || !videoCodec)
    {
        avformat_close_input(&test_formatContext);
        return -1;
    }

    AVCodecContext* codecContext = avcodec_alloc_context3(videoCodec);
    avcodec_parameters_to_context(codecContext, videoCodecParams);

    if (avcodec_open2(codecContext, videoCodec, nullptr) < 0)
    {
        avcodec_free_context(&codecContext);
        avformat_close_input(&test_formatContext);
        return -1;
    }

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();

    // Test the frame
    for (int i = 0; av_read_frame(test_formatContext, packet) >= 0 && i < test_packets; ++i)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            if (avcodec_send_packet(codecContext, packet) == 0)
            {
                while (avcodec_receive_frame(codecContext, frame) == 0)
                {
                    std::cout << "Received a frame" << std::endl;
                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    // --- Cleanup ---
    av_packet_free(&packet);
    av_frame_free(&frame);
    avcodec_free_context(&codecContext);
    avformat_close_input(&test_formatContext);
    avformat_network_deinit(); // Clean up network
    return 0;
}