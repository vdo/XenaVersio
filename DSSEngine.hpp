#pragma once

#include <cstdint>

static constexpr int MAX_BREAKPOINTS = 16;

enum class WalkMode {
    FIRST_ORDER,    // Classic random walks
    SECOND_ORDER,   // GENDY3 style with velocity
    CORRELATED      // Same random values for duration/amplitude
};

struct Breakpoint {
    float durationVel;      // Primary walk (velocity for second-order)
    float durationPos;      // Secondary walk (position)
    float amplitudeVel;     // Primary walk (velocity for second-order)
    float amplitudePos;     // Secondary walk (position)
    float currentDuration;  // Normalized duration for this segment
    float currentAmplitude; // Current amplitude value [-1, 1]
};

class DSSEngine {
public:
    DSSEngine();

    void Init(float sampleRate);
    void Reset();
    void Sync();  // Hard sync - reset phase without reinitializing breakpoints
    float Process();

    // Parameter setters
    void SetFrequency(float freq);
    void SetDurationStep(float step);
    void SetAmplitudeStep(float step);
    void SetDurationBarrier(float barrier);
    void SetAmplitudeBarrier(float barrier);
    void SetBreakpointCount(int count);
    void SetWalkMode(WalkMode mode);

private:
    // Fast LCG random number generator
    inline uint32_t FastRandom() {
        seed_ = seed_ * 1664525u + 1013904223u;
        return seed_;
    }

    // Get random float in range [-1, 1]
    inline float RandomBipolar() {
        return (static_cast<float>(FastRandom()) / 2147483648.0f) - 1.0f;
    }

    // Elastic barrier reflection
    inline void Reflect(float& val, float min, float max) {
        while (val < min || val > max) {
            if (val < min) val = min + (min - val);
            if (val > max) val = max - (val - max);
        }
    }

    void UpdateWalks();
    void NormalizeDurations();
    float LinearInterp(float a, float b, float t);

    Breakpoint breakpoints_[MAX_BREAKPOINTS];

    float sampleRate_;
    float phase_;
    float phaseIncrement_;
    float frequency_;

    float durationStep_;
    float amplitudeStep_;
    float durationBarrier_;
    float amplitudeBarrier_;

    int breakpointCount_;
    int currentSegment_;
    float segmentPhase_;

    WalkMode walkMode_;
    uint32_t seed_;
};
