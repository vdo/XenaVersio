#include "daisy_versio.h"
#include "DSSEngine.hpp"
#include <cmath>

using namespace daisy;

DaisyVersio hw;
DSSEngine dss;

float sampleRate;
float outputLevel = 0.8f;

// Gate state for edge detection
bool prevGate = false;

// Hard sync zero-crossing detection
float prevSyncIn = 0.0f;

// Simple LCG for stereo width randomness
uint32_t randState = 12345;
inline float fastRand() {
    randState = randState * 1664525 + 1013904223;
    return (randState >> 16) / 32768.0f - 1.0f; // -1 to 1
}

// Calibration
bool inCalibration = false;
const int CALIBRATION_MAX = 65536;
const int CALIBRATION_MIN = 63200;
uint16_t calibrationOffset = 64262;
uint16_t calibrationUnitsPerVolt = 12826;
const uint16_t CALIBRATION_THRESH = CALIBRATION_MAX - 200;

// Base frequencies for each range (C-based for Eurorack standard)
const float BASE_FREQ_LOW = 65.41f;   // C2
const float BASE_FREQ_MID = 261.63f;  // C4 (middle C)
const float BASE_FREQ_HIGH = 1046.50f; // C6

// Persistence
struct Settings {
    float calibrationOffset;
    float calibrationUnitsPerVolt;
    bool operator!=(const Settings& a) {
        return a.calibrationUnitsPerVolt != calibrationUnitsPerVolt;
    }
};

PersistentStorage<Settings> storage(hw.seed.qspi);

void SaveData()
{
    Settings& settings = storage.GetSettings();
    settings.calibrationOffset = calibrationOffset;
    settings.calibrationUnitsPerVolt = calibrationUnitsPerVolt;
    storage.Save();
}

void LoadData()
{
    Settings& settings = storage.GetSettings();
    calibrationOffset = settings.calibrationOffset;
    calibrationUnitsPerVolt = settings.calibrationUnitsPerVolt;
}

static void AudioCallback(AudioHandle::InputBuffer in,
                          AudioHandle::OutputBuffer out,
                          size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        float syncIn = IN_L[i];
        float ringIn = IN_R[i];

        // Hard sync: detect rising zero-crossing on IN_L
        if (prevSyncIn <= 0.0f && syncIn > 0.0f) {
            dss.Sync();
        }
        prevSyncIn = syncIn;

        // Generate DSS sample
        float sample = dss.Process() * outputLevel;

        // Ring modulation: multiply by IN_R (when patched, otherwise ringIn ≈ 0)
        float ringOut = sample * (1.0f + ringIn);  // Blend: dry when no input, ring mod when patched

        OUT_L[i] = sample;           // Dry output
        OUT_R[i] = ringOut;          // Ring mod output
    }
}

void WaitForButton()
{
    while (!hw.tap.RisingEdge()) {
        hw.tap.Debounce();
    }
    while (!hw.tap.FallingEdge()) {
        hw.tap.Debounce();
    }
    System::Delay(200);
}

void DoCalibration()
{
    inCalibration = true;
    const uint8_t NUM_SAMPLES = 10;

    // Step 0: Release button
    hw.tap.Debounce();
    hw.SetLed(0, 1, 1, 1);
    hw.SetLed(1, 1, 1, 1);
    hw.SetLed(2, 1, 1, 1);
    hw.SetLed(3, 1, 1, 1);
    hw.UpdateLeds();

    while (hw.tap.RawState()) {
        hw.tap.Debounce();
    }

    // Step 1: 1V reference
    hw.SetLed(0, 0, 1, 0);
    hw.SetLed(1, 0, 0, 0);
    hw.SetLed(2, 0, 0, 0);
    hw.SetLed(3, 0, 0, 0);
    hw.UpdateLeds();
    WaitForButton();
    float oneVoltValue = hw.knobs[0].GetRawValue();

    // Step 2: 2V reference
    hw.SetLed(0, 0, 0, 1);
    hw.SetLed(1, 0, 0, 1);
    hw.SetLed(2, 0, 0, 0);
    hw.SetLed(3, 0, 0, 0);
    hw.UpdateLeds();
    WaitForButton();

    float total = 0;
    for (uint8_t x = 0; x < NUM_SAMPLES; ++x) {
        hw.knobs[0].Process();
        total += hw.knobs[0].GetRawValue();
    }
    float twoVoltValue = total / NUM_SAMPLES;

    // Step 3: 3V reference
    hw.SetLed(0, 0, 1, 1);
    hw.SetLed(1, 0, 1, 1);
    hw.SetLed(2, 0, 1, 1);
    hw.SetLed(3, 0, 0, 0);
    hw.UpdateLeds();
    WaitForButton();

    total = 0;
    for (uint8_t x = 0; x < NUM_SAMPLES; ++x) {
        hw.knobs[0].Process();
        total += hw.knobs[0].GetRawValue();
    }
    float threeVoltValue = total / NUM_SAMPLES;

    // Calculate calibration values
    float firstEstimate = oneVoltValue - twoVoltValue;
    float secondEstimate = twoVoltValue - threeVoltValue;
    float avgEstimate = (firstEstimate + secondEstimate) / 2.0f;
    uint16_t offset = oneVoltValue + avgEstimate;

    // Save
    calibrationOffset = offset;
    calibrationUnitsPerVolt = static_cast<uint16_t>(avgEstimate + 0.5f);
    SaveData();

    inCalibration = false;
}

float GetVoctFrequency(float baseFreq)
{
    float rawCv = hw.knobs[hw.KNOB_0].GetRawValue();
    float volts;

    if (rawCv > CALIBRATION_MIN) {
        volts = 0.0f;
    } else {
        volts = (calibrationOffset - rawCv) / static_cast<float>(calibrationUnitsPerVolt);
        if (volts < 0.0f) volts = 0.0f;
        if (volts > 5.0f) volts = 5.0f;
    }

    return baseFreq * powf(2.0f, volts);
}

int main(void)
{
    // Initialize hardware
    hw.Init();
    hw.StartAdc();

    sampleRate = hw.seed.AudioSampleRate();

    // Initialize DSS engine
    dss.Init(sampleRate);

    // Initialize persistent storage
    Settings defaults;
    defaults.calibrationOffset = calibrationOffset;
    defaults.calibrationUnitsPerVolt = calibrationUnitsPerVolt;
    storage.Init(defaults);
    LoadData();

    // Validate calibration data
    if (calibrationUnitsPerVolt < 400 || calibrationUnitsPerVolt > 20000) {
        storage.RestoreDefaults();
        LoadData();
    }

    // Check for calibration mode: both switches right + button held
    hw.ProcessAllControls();
    hw.tap.Debounce();
    if (hw.sw[0].Read() == hw.sw->POS_RIGHT &&
        hw.sw[1].Read() == hw.sw->POS_RIGHT) {
        if (hw.tap.RawState()) {
            DoCalibration();
        }
    }

    // Start audio
    hw.StartAudio(AudioCallback);

    // LED brightness values for feedback
    float ledPhase = 0.0f;
    float ledDuration = 0.0f;
    float ledAmplitude = 0.0f;

    while (1) {
        hw.ProcessAllControls();
        hw.tap.Debounce();

        // Read top switch: Walk mode
        WalkMode walkMode;
        if (hw.sw[0].Read() == hw.sw->POS_LEFT) {
            walkMode = WalkMode::FIRST_ORDER;
        } else if (hw.sw[0].Read() == hw.sw->POS_CENTER) {
            walkMode = WalkMode::SECOND_ORDER;
        } else {
            walkMode = WalkMode::CORRELATED;
        }
        dss.SetWalkMode(walkMode);

        // Read bottom switch: Frequency range
        float baseFreq;
        if (hw.sw[1].Read() == hw.sw->POS_LEFT) {
            baseFreq = BASE_FREQ_LOW;
        } else if (hw.sw[1].Read() == hw.sw->POS_CENTER) {
            baseFreq = BASE_FREQ_MID;
        } else {
            baseFreq = BASE_FREQ_HIGH;
        }

        // KNOB_0: V/oct pitch (with CV)
        float freq = GetVoctFrequency(baseFreq);
        dss.SetFrequency(freq);

        // KNOB_1: Duration step (0.001 - 0.5) - CV via knob jack
        float durationStep = hw.GetKnobValue(DaisyVersio::KNOB_1);
        durationStep = 0.001f + durationStep * 0.499f;
        dss.SetDurationStep(durationStep);
        ledDuration = hw.GetKnobValue(DaisyVersio::KNOB_1);

        // KNOB_2: Amplitude step (0.001 - 0.5) - CV via knob jack
        float amplitudeStep = hw.GetKnobValue(DaisyVersio::KNOB_2);
        amplitudeStep = 0.001f + amplitudeStep * 0.499f;
        dss.SetAmplitudeStep(amplitudeStep);
        ledAmplitude = hw.GetKnobValue(DaisyVersio::KNOB_2);

        // KNOB_3: Duration barrier (0.1 - 1.0)
        float durationBarrier = hw.GetKnobValue(DaisyVersio::KNOB_3);
        durationBarrier = 0.1f + durationBarrier * 0.9f;
        dss.SetDurationBarrier(durationBarrier);

        // KNOB_4: Amplitude barrier (0.1 - 1.0)
        float amplitudeBarrier = hw.GetKnobValue(DaisyVersio::KNOB_4);
        amplitudeBarrier = 0.1f + amplitudeBarrier * 0.9f;
        dss.SetAmplitudeBarrier(amplitudeBarrier);

        // KNOB_5: Breakpoints (2-16)
        float bpKnob = hw.GetKnobValue(DaisyVersio::KNOB_5);
        int breakpoints = 2 + static_cast<int>(bpKnob * 14.0f);
        dss.SetBreakpointCount(breakpoints);

        // KNOB_6: Output level
        outputLevel = hw.GetKnobValue(DaisyVersio::KNOB_6);

        // Button or Gate: Reset walks
        bool gate = hw.Gate();
        if (hw.tap.RisingEdge() || (gate && !prevGate)) {
            dss.Reset();
        }
        prevGate = gate;

        // Update LEDs
        if (!inCalibration) {
            // LED_0: Phase indicator (cyan)
            ledPhase += 0.01f;
            if (ledPhase > 1.0f) ledPhase = 0.0f;
            hw.SetLed(hw.LED_0, 0, ledPhase * 0.5f, ledPhase * 0.5f);

            // LED_1: Duration activity (green)
            hw.SetLed(hw.LED_1, 0, ledDuration, 0);

            // LED_2: Amplitude activity (orange)
            hw.SetLed(hw.LED_2, ledAmplitude, ledAmplitude * 0.5f, 0);

            // LED_3: Output level (white)
            hw.SetLed(hw.LED_3, outputLevel, outputLevel, outputLevel);

            hw.UpdateLeds();
        }
    }
}
