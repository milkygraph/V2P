#include "SDLWindow.h"

#include <SDL_audio.h>
#include <iostream>
#include <V2P/stream/VideoStreamFactory.h>
#include <V2P/stream/VideoFrame.h>
#include <V2P/stream/IStreamStrategy.h>

SDLWindow::SDLWindow(const std::string& title, int minWidth, int minHeight) {
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "Failed to initialize the SDL2 library\n";
        return;
    }

    m_Window = SDL_CreateWindow("V2P", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1280, 720, (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));

    if(!m_Window) {
        std::cout << "Failed to create window\n";
        SDL_Quit();
        m_keepWindowOpen = false;
        return;
    }

    SDL_SetWindowMinimumSize(m_Window, minWidth, minHeight);

    m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!m_Renderer) {
        std::cout << "Failed to create m_Renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(m_Window);
        SDL_Quit();
        m_keepWindowOpen = false;
        return;
    }

    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);

    desiredSpec.freq = 44100;
    desiredSpec.format = AUDIO_S16SYS; // This is AV_SAMPLE_FMT_S16
    desiredSpec.channels = 2;          // This is AV_CH_LAYOUT_STEREO
    desiredSpec.samples = 4096;        // Buffer size
    desiredSpec.callback = nullptr;

    // Note: A "real" app would pass this ID to the engine
    m_audioDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, &m_audioSpec, 0);

    if (m_audioDeviceID == 0) {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        // Not fatal, just continue without sound
    } else {
        std::cout << "Audio device opened." << std::endl;
        SDL_PauseAudioDevice(m_audioDeviceID, 0); // Start playback
    }

    const std::string videoUrl = "https://fast-tailor.3catdirectes.cat/v1/channel/ccma-channel2/hls.m3u8";
    m_streamer = VideoStreamFactory::createVideoStreamer(videoUrl);

    if (!m_streamer) {
        std::cout << "Failed to create streamer for URL: " << videoUrl << std::endl;
        m_keepWindowOpen = false;
    } else {
        std::cout << "Streamer created for: " << videoUrl << std::endl;
    }

    m_streamer->setAudioCallback([this](uint8_t* data, int size) -> bool {
        // Queue audio data to SDL
        if (SDL_QueueAudio(m_audioDeviceID, data, size) < 0) {
            std::cerr << "Failed to queue audio data: " << SDL_GetError() << std::endl;
            return false;
        }
        return true;
    });
}

SDLWindow::~SDLWindow() {
    if (m_audioDeviceID > 0) {
        SDL_CloseAudioDevice(m_audioDeviceID);
    }

    if (m_videoTexture) {
        SDL_DestroyTexture(m_videoTexture);
    }
    if (m_Renderer) {
        SDL_DestroyRenderer(m_Renderer);
    }
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
    }
    SDL_Quit();
}

void SDLWindow::run() {
    while(m_keepWindowOpen) {
        handleEvents();
        updateFrame();
        render();
    }
}

void SDLWindow::handleEvents() {
    SDL_Event e;
    while(SDL_PollEvent(&e) > 0) {
        if (e.type == SDL_QUIT) {
            m_keepWindowOpen = false;
        }
    }
}

void SDLWindow::updateFrame() {
    if (!m_streamer) return;

    VideoFrame frame;
    // This function processes audio (via your callback) while
    // it searches for the next video frame.
    if (m_streamer->getNextVideoFrame(frame)) {
        double video_timestamp = frame.timestamp;
        if (video_timestamp == 0.0) return;

        double audio_clock = m_streamer->getClock();
        Uint32 buffered_bytes = SDL_GetQueuedAudioSize(m_audioDeviceID);
        int bytes_per_second = m_audioSpec.freq * m_audioSpec.channels * 2;
        double buffered_seconds = (double)buffered_bytes / (double)bytes_per_second;

        double actual_audio_time = audio_clock - buffered_seconds;
        double delay = video_timestamp - actual_audio_time;

        std::cout << "Video TS: " << video_timestamp
                  << " | Audio TS: " << actual_audio_time
                  << " | Delay: " << delay << " seconds." << std::endl;

        const double SMALL_EARLY_THRESHOLD = 0.010;
        const double DROP_LATE_THRESHOLD    = -0.050;
        const double MAX_WAIT_MS            = 50.0;

        if (delay > SMALL_EARLY_THRESHOLD) {
            // Wait for the audio to catch up
            double wait_ms = delay * 1000.0;
            if (wait_ms > MAX_WAIT_MS) wait_ms = MAX_WAIT_MS;
            SDL_Delay((Uint32)wait_ms);
        } else if (delay < -DROP_LATE_THRESHOLD) { // Video is > 100ms late?
            // This frame is too old, drop it and get the next one
            std::cout << "Dropping late video frame to catch up." << std::endl;
            return;
        }
        // If we're here, the frame is in sync (or only slightly late), so we render it.

        if (!m_videoTexture) {
            m_videoTexture = SDL_CreateTexture(m_Renderer,
                                             SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             frame.width, frame.height);
            if (m_videoTexture) {
                SDL_SetWindowSize(m_Window, frame.width, frame.height);
            }
        }

        if (m_videoTexture) {
            SDL_UpdateTexture(m_videoTexture, nullptr, frame.data.data(), frame.width * 4);
        }
    } else {
        m_keepWindowOpen = false; // Stream ended
    }
}

void SDLWindow::render() {
    SDL_SetRenderDrawColor(m_Renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_Renderer);

    if (m_videoTexture) {
        SDL_RenderCopy(m_Renderer, m_videoTexture, nullptr, nullptr);
    }

    SDL_RenderPresent(m_Renderer);
}
