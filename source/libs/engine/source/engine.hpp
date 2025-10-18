#include <iostream>

// Use extern "C" to include C headers in C++
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

extern "C" {
    int check_ffmpeg();
    int read_test_stream(int test_packets);
}