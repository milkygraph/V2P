#pragma once

#include <string>
#include <memory>
#include <SDL2/SDL.h>
#include <unordered_map>
#include <vector>

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

    SDL_AudioDeviceID m_audioDeviceID = 0;
    SDL_AudioSpec m_audioSpec = {};

    std::vector<std::unique_ptr<VideoStreamer>> m_streamers;
    std::unordered_map<VideoStreamer*, SDL_Texture*> m_videoTextures;

    Uint32 m_playbackStartTime = 0;
    bool m_keepWindowOpen = true;
};
