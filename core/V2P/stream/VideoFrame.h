#pragma once

#include <vector>
#include <cstdint>

/**
 * @brief A simple, self-contained struct to hold one decoded video frame.
 *
 * This struct owns its own data, making it safe to pass
 * from the engine thread to the UI thread (with a mutex).
 * The engine is expected to convert all video to RGBA for
 * easy rendering by clients like SDL.
 */
struct VideoFrame {
    std::vector<uint8_t> data;

    int width = 0;
    int height = 0;

    // Presentation timestamp in seconds
    double timestamp = 0.0;
};
