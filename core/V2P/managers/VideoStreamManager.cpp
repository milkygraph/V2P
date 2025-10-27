#include <V2P/stream/VideoStreamer.h>
#include <cstdint>

class VideoStreamManager {
    void addStream(std::unique_ptr<VideoStreamer> streamer) {
        streams.push_back(std::move(streamer));
    }

    void updateAll(uint32_t bufferedBytes, int bytesPerSecond) {
        for (auto& s : streams) {
            VideoFrame frame;
            if (s->updateFrame(frame, bufferedBytes, bytesPerSecond)) {
                // Store or forward frame for rendering
                latestFrames.push_back(std::move(frame));
            }
        }
    }

    const std::vector<VideoFrame>& getFrames() const { return latestFrames; }
private:
    std::vector<std::unique_ptr<VideoStreamer>> streams;
    std::vector<VideoFrame> latestFrames;
};
