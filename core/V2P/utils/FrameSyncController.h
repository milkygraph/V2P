#pragma once

class FrameSyncController {
public:
    struct SyncDecision {
        bool show = false;
        bool drop = false;
        double waitUntil = 0.0; // absolute timestamp (seconds)
    };

    SyncDecision evaluate(double frameTimestamp, double referenceClock, double now) {
        SyncDecision result;

        double diff = frameTimestamp - referenceClock; // how far ahead/behind we are

        constexpr double EARLY_THRESHOLD = 0.010; // 10ms ahead
        constexpr double LATE_THRESHOLD  = -0.050; // 50ms late

        if (diff > EARLY_THRESHOLD) {
            // Frame is too early → mark it for later
            result.waitUntil = now + diff;
        } else if (diff < LATE_THRESHOLD) {
            // Frame is too late → drop it
            result.drop = true;
        } else {
            // Frame is on time → show now
            result.show = true;
        }

        return result;
    }
};
