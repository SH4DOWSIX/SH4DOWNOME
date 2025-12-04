#pragma once

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QString>
#include <vector>
#include <atomic>

// MiniAudio (header-only)
#include "miniaudio.h"

// Pulse event info
struct AudioPulseEvent {
    int idx;
    bool accent;
    bool polyAccent;
    bool isBeat;
    bool playPulse;
    bool isRest = false;  // NEW: indicates if this pulse is a rest
    int gridColumn;
    int samplePosInBar = 0;
    bool startOfCycle = false;
};

using PulseCallback = std::function<void(const AudioPulseEvent&)>;

struct PCMBuffer {
    std::vector<float> data;
    int numChannels = 1;
    int sampleRate = 44100;
    bool valid = false;
    int startSample = 0;

    // Stores the original resource path so we can re-decode to a different device rate
    QString resourcePath;

    // Load and decode resource directly into floats at deviceRate (uses in-memory decoding)
    bool loadFromWavResource(const QString& resourcePath_, int deviceRate);

    // Reload from stored resourcePath to a different device rate (used when device rate changes)
    bool reloadForDevice(int deviceRate);

    // Legacy: resample in-place (keeps as fallback)
    void resampleTo(int dstRate);
};

struct ActiveSample {
    const std::vector<float>* data;
    int pos;    // current playback index in sample
    int outPos; // output buffer index to start mixing
    float volume;
};

struct CountInClick {
    int globalSamplePos;
    bool accent;
};

class AudioEngine : public QObject {
    Q_OBJECT
public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();


    bool initializeDevice(double bpm);

    bool start(double bpm);
    void stop();
    bool isRunning() const { return m_running.load(); }

    void playCountInClick(bool accent, int globalSamplePos);

    void schedulePulses(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);
    void setPulseCallback(PulseCallback cb);

    bool loadSample(const QString& name, const QString& resourcePath);
    void setAccentSound(const QString& name);
    void setClickSound(const QString& name);
    void setVolume(float vol);

    void setBpm(double bpm);
    void flushBarAndReset();
    int getSampleRate() const { return m_sampleRate; }

    // Buffer-aligned schedule/tempo change
    void scheduleTempoChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);
    void requestScheduleChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);

    bool wasFlushedRecently() const { return m_flushedRecently; }

    void flushAtNextBarBoundary();

signals:
    void pulseUiEvent(AudioPulseEvent ev);

private:
    std::atomic<bool> m_running{false};
    ma_device m_device;
    ma_device_config m_deviceConfig;
    bool m_deviceInitialized = false;
    

    int m_sampleRate = 44100;
    int m_bufferFrames = 256;
    float m_sinePhase = 0.0f;

    QMap<QString, PCMBuffer> m_samples;
    QString m_accentSample = "accent";
    QString m_clickSample = "click";
    float m_volume = 1.0f;

    bool m_scheduleChanged = false;

    bool m_pendingScheduleChange = false;
    std::vector<AudioPulseEvent> m_nextPulseSchedule;
    int m_nextScheduleLengthSamples = 0;
    int m_nextScheduleSampleRate = 44100;
    double m_nextBarLengthSeconds = 2.0;

    std::vector<AudioPulseEvent> m_pulseSchedule;
    int m_pulseIdx = 0;
    int m_scheduleLengthSamples = 0;
    int m_scheduleSampleRate = 44100;
    double m_barLengthSeconds = 2.0;
    int m_globalSamplePos = 0;

    PulseCallback m_pulseCallback = nullptr;
    bool m_flushedRecently = false;

    bool m_hasPendingSchedule = false;
    std::vector<AudioPulseEvent> m_pendingPulseSchedule;
    double m_pendingBarLengthSeconds = 0.0;
    int m_pendingSampleRate = 0;

    std::vector<ActiveSample> activeSamples;
    std::vector<CountInClick> m_countInClicks;

    static void miniAudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    int doAudioCallback(float* output, unsigned int nBufferFrames);

    PCMBuffer* currentSample(bool accent);
    void emitUiPulse(const AudioPulseEvent& ev);

    QMutex m_schedMutex;

    // Deduplication: swap position to skip duplicate pulse at boundary
    int m_pendingScheduleSwapSamplePos = -1;
    
    // NEW: Sample rate detection method (ADD THIS LINE)
    void detectSampleRateSafe();
    int detectBestSampleRate();

    // Protect access to m_samples and decoding/resampling operations
    QMutex m_sampleMutex;
};