#pragma once
#include <chrono>

class Timer {
private:
    std::chrono::steady_clock::time_point startTime;
    int durationMs = 0;
    bool running = false;

public:
    inline void start(int ms) {
        durationMs = ms;
        startTime = std::chrono::steady_clock::now();
        running = true;
    }

    inline bool isDone() {
        if (!running) return true;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (elapsed >= durationMs) {
            running = false;
            return true;
        }
        return false;
    }

    inline bool isRunning() const {
        return running;
    }

    inline void restart() {
        startTime = std::chrono::steady_clock::now();
    }

    inline void stop() {
        running = false;
    }

    inline void reset() {
        durationMs = 0;
        running = false;
    }

    inline float progress() const {
        if (!running || durationMs <= 0) return 1.0f;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        return std::min(1.0f, static_cast<float>(elapsed) / durationMs);
    }
};
