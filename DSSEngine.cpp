#include "DSSEngine.hpp"

DSSEngine::DSSEngine()
    : sampleRate_(48000.0f)
    , phase_(0.0f)
    , phaseIncrement_(0.0f)
    , frequency_(220.0f)
    , durationStep_(0.1f)
    , amplitudeStep_(0.1f)
    , durationBarrier_(0.5f)
    , amplitudeBarrier_(0.9f)
    , breakpointCount_(8)
    , currentSegment_(0)
    , segmentPhase_(0.0f)
    , walkMode_(WalkMode::SECOND_ORDER)
    , seed_(12345u)
{
}

void DSSEngine::Init(float sampleRate)
{
    sampleRate_ = sampleRate;
    Reset();
}

void DSSEngine::Reset()
{
    phase_ = 0.0f;
    currentSegment_ = 0;
    segmentPhase_ = 0.0f;
    seed_ = 12345u;

    // Initialize breakpoints with varied starting positions
    for (int i = 0; i < MAX_BREAKPOINTS; ++i) {
        breakpoints_[i].durationVel = 0.0f;
        breakpoints_[i].amplitudeVel = 0.0f;
        breakpoints_[i].durationPos = RandomBipolar() * 0.3f;
        breakpoints_[i].amplitudePos = RandomBipolar() * 0.5f;
        breakpoints_[i].currentDuration = 1.0f;
        breakpoints_[i].currentAmplitude = breakpoints_[i].amplitudePos;
    }

    // First and last breakpoints should be at zero for smooth cycling
    breakpoints_[0].amplitudePos = 0.0f;
    breakpoints_[0].currentAmplitude = 0.0f;

    NormalizeDurations();
    SetFrequency(frequency_);
}

void DSSEngine::Sync()
{
    // Hard sync: reset phase to start of waveform, trigger walk update
    phase_ = 0.0f;
    currentSegment_ = 0;
    segmentPhase_ = 0.0f;
    UpdateWalks();
}

void DSSEngine::SetFrequency(float freq)
{
    frequency_ = freq;
    // Phase increment: we complete one waveform cycle at the desired frequency
    phaseIncrement_ = freq / sampleRate_;
}

void DSSEngine::SetDurationStep(float step)
{
    durationStep_ = step;
}

void DSSEngine::SetAmplitudeStep(float step)
{
    amplitudeStep_ = step;
}

void DSSEngine::SetDurationBarrier(float barrier)
{
    durationBarrier_ = barrier;
}

void DSSEngine::SetAmplitudeBarrier(float barrier)
{
    amplitudeBarrier_ = barrier;
}

void DSSEngine::SetBreakpointCount(int count)
{
    if (count < 2) count = 2;
    if (count > MAX_BREAKPOINTS) count = MAX_BREAKPOINTS;
    breakpointCount_ = count;
    NormalizeDurations();
}

void DSSEngine::SetWalkMode(WalkMode mode)
{
    walkMode_ = mode;
}

void DSSEngine::UpdateWalks()
{
    for (int i = 0; i < breakpointCount_; ++i) {
        Breakpoint& bp = breakpoints_[i];

        // Get random values
        float randDur = RandomBipolar();
        float randAmp;

        if (walkMode_ == WalkMode::CORRELATED) {
            randAmp = randDur; // Same random value
        } else {
            randAmp = RandomBipolar();
        }

        if (walkMode_ == WalkMode::FIRST_ORDER) {
            // First-order walk: directly modify position
            bp.durationPos += randDur * durationStep_;
            bp.amplitudePos += randAmp * amplitudeStep_;
        } else {
            // Second-order walk: modify velocity, then position
            bp.durationVel += randDur * durationStep_;
            bp.amplitudeVel += randAmp * amplitudeStep_;

            // Reflect velocity at barriers (scaled by step)
            float velBarrierDur = durationBarrier_ * 0.5f;
            float velBarrierAmp = amplitudeBarrier_ * 0.5f;
            Reflect(bp.durationVel, -velBarrierDur, velBarrierDur);
            Reflect(bp.amplitudeVel, -velBarrierAmp, velBarrierAmp);

            bp.durationPos += bp.durationVel;
            bp.amplitudePos += bp.amplitudeVel;
        }

        // Reflect positions at barriers
        Reflect(bp.durationPos, -durationBarrier_, durationBarrier_);
        Reflect(bp.amplitudePos, -amplitudeBarrier_, amplitudeBarrier_);

        // Update current values from positions
        bp.currentDuration = 1.0f + bp.durationPos;
        if (bp.currentDuration < 0.1f) bp.currentDuration = 0.1f;

        bp.currentAmplitude = bp.amplitudePos;
    }

    // Ensure waveform continuity: first/last amplitude at 0 or match
    breakpoints_[0].currentAmplitude = 0.0f;
    breakpoints_[0].amplitudePos = 0.0f;

    NormalizeDurations();
}

void DSSEngine::NormalizeDurations()
{
    // Calculate total duration and normalize so they sum to 1.0
    float total = 0.0f;
    for (int i = 0; i < breakpointCount_; ++i) {
        total += breakpoints_[i].currentDuration;
    }

    if (total > 0.0001f) {
        float scale = 1.0f / total;
        for (int i = 0; i < breakpointCount_; ++i) {
            breakpoints_[i].currentDuration *= scale;
        }
    }
}

float DSSEngine::LinearInterp(float a, float b, float t)
{
    return a + (b - a) * t;
}

float DSSEngine::Process()
{
    // Find which segment we're in based on accumulated durations
    float accumulated = 0.0f;
    currentSegment_ = 0;

    for (int i = 0; i < breakpointCount_; ++i) {
        float nextAccum = accumulated + breakpoints_[i].currentDuration;
        if (phase_ < nextAccum) {
            currentSegment_ = i;
            break;
        }
        accumulated = nextAccum;
        currentSegment_ = i;
    }

    // Calculate position within current segment
    float segmentDuration = breakpoints_[currentSegment_].currentDuration;
    if (segmentDuration > 0.0001f) {
        segmentPhase_ = (phase_ - accumulated) / segmentDuration;
    } else {
        segmentPhase_ = 0.0f;
    }

    // Clamp segment phase
    if (segmentPhase_ < 0.0f) segmentPhase_ = 0.0f;
    if (segmentPhase_ > 1.0f) segmentPhase_ = 1.0f;

    // Get current and next breakpoint amplitudes
    int nextSegment = (currentSegment_ + 1) % breakpointCount_;
    float currentAmp = breakpoints_[currentSegment_].currentAmplitude;
    float nextAmp = breakpoints_[nextSegment].currentAmplitude;

    // Linear interpolation
    float output = LinearInterp(currentAmp, nextAmp, segmentPhase_);

    // Advance phase
    phase_ += phaseIncrement_;

    // Check for cycle completion
    if (phase_ >= 1.0f) {
        phase_ -= 1.0f;
        UpdateWalks();
    }

    return output;
}
