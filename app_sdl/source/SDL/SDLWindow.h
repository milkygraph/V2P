#pragma once

#include <string>
#include <memory>
#include <SDL2/SDL.h>

class VideoStreamer;
struct VideoFrame;

class SDLWindow{
public:
    SDLWindow(const std::string& title, int minWidth, int minHeight);
    ~SDLWindow();

    // The main loop
    void run();

private:
    // Helper functions for the main loop
    void handleEvents();
    void updateFrame();
    void render();

    // SDL members
    SDL_Window* m_Window = nullptr;
    SDL_Renderer* m_Renderer = nullptr;
    SDL_Texture* m_videoTexture = nullptr;
    SDL_AudioDeviceID m_audioDeviceID = 0; // <-- NEW

    // Engine/State members
    std::unique_ptr<VideoStreamer> m_streamer; // <-- NEW
    Uint32 m_playbackStartTime = 0;
    bool m_keepWindowOpen = true;
};
