#pragma once

#include <string>
#include <libavutil/pixfmt.h>
#include <libavutil/rational.h>

// Forward-declare FFmpeg types
struct AVFrame;
struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;

class VideoWriter {
public:
    VideoWriter();
    ~VideoWriter();

    /**
    * @brief Opens and prepares a video file for writing.
    * @param filename The output filename (e.g., "output.mp4").
    * @param width The width of the video frames.
    * @param height The height of the video frames.
    * @param time_base The time base (framerate) of the video.
    * @param input_pix_fmt The pixel format of the incoming AVFrames.
    * @return True on success, false on failure.
    */
    bool open(const std::string& filename, int width, int height, AVRational time_base, AVPixelFormat input_pix_fmt);

    /**
    * @brief Encodes and writes a single frame to the video file.
    * @param frame The AVFrame to write. The frame's pixel format must match
    * the input_pix_fmt provided in open().
    * @return True on success, false on failure.
    */
    bool writeFrame(AVFrame* frame);

    /**
    * @brief Finalizes the video file, flushing any buffered frames.
    * @return True on success, false on failure.
    */
    bool close();

private:
    bool encode(const AVFrame* frame);

    AVFormatContext* formatContext;
    AVCodecContext* codecContext;
    SwsContext* swsContext;
    AVFrame* tmpFrame; // Used for pixel format conversion
    int64_t nextPts;
};
