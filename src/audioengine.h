#pragma once

#include <QObject>
#include <QMutex>
#include <QMap>
#include <QString>
#include <vector>
#include <atomic>
#include <cstdint>
#include <functional>

// MiniAudio (header-only)
#include "miniaudio.h"
#include "subdivisionpattern.h"

// Pulse event info
struct AudioPulseEvent {
    int idx;
    bool accent;
    bool polyAccent;
    bool isBeat;
    bool playPulse;
    bool isRest = false;
    int gridColumn;
    int samplePosInBar = 0;
    bool startOfCycle = false;
    // New fields for the bar-advance state machine
    int  barNumber    = 0;   // which bar (0-indexed, resets per tempo)
    int  barsPerStep  = 1;   // speed trainer bars-per-step (for display "Bar X/Y")
    bool isFirstInBar = false; // true for the first pulse of every bar
    int  newTempo     = 0;   // non-zero only on first pulse of a new stepped-up tempo
    int  runId        = 0;   // incremented each startWithParams(); stale signals have old IDs
};

using PulseCallback = std::function<void(const AudioPulseEvent&)>;

struct PCMBuffer {
    std::vector<float> data;
    int numChannels = 1;
    int sampleRate = 44100;
    bool valid = false;
    int startSample = 0;

    QString resourcePath;

    bool loadFromWavResource(const QString& resourcePath_, int deviceRate);
    bool reloadForDevice(int deviceRate);
    void resampleTo(int dstRate);
};

struct ActiveSample {
    const std::vector<float>* data;
    int pos;
    int outPos;
    float volume;
};

struct CountInClick {
    int globalSamplePos;
    bool accent;
};

// ── Bar schedule (produced by buildBarSchedule, consumed by AudioEngine) ──
struct BarSchedule {
    std::vector<AudioPulseEvent> pulses;  // samplePosInBar = relative positions within bar
    int64_t barLengthSamples = 0;
};

// ── Parameters snapshot passed from MetronomeEngine → AudioEngine ──
struct EngineParams {
    int bpm        = 120;
    int numerator  = 4;
    int denominator = 4;
    bool polyrhythmEnabled = false;
    int  polyMain      = 3;
    int  polySecondary = 2;
    SubdivisionPattern subdivision;
    std::vector<bool>  accents;
    bool countInEnabled = false;
    // Speed trainer
    bool speedEnabled = false;
    int  barsPerStep  = 4;
    int  tempoStep    = 2;
    int  maxTempo     = 180;
    int  startTempo   = 120;
};

// ── Bar provider: called by audio thread to build each bar on demand ──
// Implemented as a free function in metronomeengine.cpp, captured by value.
BarSchedule buildBarSchedule(const EngineParams& params, bool isCountIn, int sampleRate);

class AudioEngine : public QObject {
    Q_OBJECT
public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine();

    bool initializeDevice(double bpm);

    bool start(double bpm);   // legacy — kept for compat, use setEngineParams then start()
    void stop();
    bool isRunning() const { return m_running.load(); }
    int  currentRunId() const { return m_runId.load(); }

    // ── New state-machine API ──────────────────────────────────────────────
    // Set/update engine params.  Safe to call from main thread at any time.
    // Changes take effect at the next bar boundary.
    void setEngineParams(const EngineParams& p);

    // Start playback with the given params (stops first if already running).
    void startWithParams(const EngineParams& p, bool withCountIn);
    // ──────────────────────────────────────────────────────────────────────

    void playCountInClick(bool accent, int globalSamplePos);

    void schedulePulses(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);
    void setPulseCallback(PulseCallback cb);

    bool loadSample(const QString& name, const QString& resourcePath);
    void setAccentSound(const QString& name);
    void setClickSound(const QString& name);
    void setVolume(float vol);

    void setBpm(double bpm);
    void flushBarAndReset();
    void scheduleNow(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);
    int  getSampleRate() const { return m_sampleRate; }

    void scheduleTempoChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);
    void requestScheduleChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate);

    bool wasFlushedRecently() const { return m_flushedRecently; }
    void flushAtNextBarBoundary();

signals:
    void pulseUiEvent(AudioPulseEvent ev);
    void tempoSteppedUp(int newTempo);   // emitted from audio thread (queued)

private:
    std::atomic<bool> m_running{false};
    std::atomic<int>  m_runId{0};   // incremented on every startWithParams()
    ma_device m_device;
    ma_device_config m_deviceConfig;
    bool m_deviceInitialized = false;

    int   m_sampleRate   = 44100;
    int   m_bufferFrames = 256;
    float m_sinePhase    = 0.0f;

    QMap<QString, PCMBuffer> m_samples;
    QString m_accentSample = "accent";
    QString m_clickSample  = "click";
    float m_volume = 1.0f;

    // ── Legacy scheduling fields (used by old paths, kept for compat) ──
    bool m_scheduleChanged    = false;
    bool m_pendingScheduleChange = false;
    std::vector<AudioPulseEvent> m_nextPulseSchedule;
    int    m_nextScheduleLengthSamples = 0;
    int    m_nextScheduleSampleRate    = 44100;
    double m_nextBarLengthSeconds      = 2.0;
    std::vector<AudioPulseEvent> m_pulseSchedule;
    int    m_pulseIdx              = 0;
    int    m_scheduleLengthSamples = 0;
    int    m_scheduleSampleRate    = 44100;
    double m_barLengthSeconds      = 2.0;
    bool   m_flushedRecently       = false;
    bool   m_hasPendingSchedule    = false;
    std::vector<AudioPulseEvent> m_pendingPulseSchedule;
    double m_pendingBarLengthSeconds = 0.0;
    int    m_pendingSampleRate       = 0;
    int64_t m_pendingScheduleSwapSamplePos = -1;
    // ──────────────────────────────────────────────────────────────────

    int64_t m_globalSamplePos = 0;

    PulseCallback m_pulseCallback = nullptr;

    std::vector<ActiveSample> activeSamples;
    std::vector<CountInClick> m_countInClicks;

    // ── New state-machine fields ──────────────────────────────────────
    enum class EnginePlayState { Idle, CountIn, Playing };
    EnginePlayState m_playState         = EnginePlayState::Idle;
    EngineParams    m_engineParams;
    bool            m_paramsChanged     = false;
    int             m_currentTempo      = 120;  // tracks current stepped-up BPM
    int             m_countInBarsLeft   = 0;
    int64_t         m_playingBarSamplesAccum = 0; // accumulated playing samples for speed-trainer bar counting
    int64_t         m_nextBarStart      = 0;    // absolute sample of next bar to generate
    // Scheduled pulses sorted by absolute sample position
    struct ScheduledPulse {
        int64_t   samplePos;  // absolute
        AudioPulseEvent ev;
    };
    std::vector<ScheduledPulse> m_scheduledPulses;
    int m_barNumberForUi          = 0; // bar counter emitted in AudioPulseEvent.barNumber
    int m_pendingStepUpTempoForTag = 0; // non-zero: tag next bar's first pulse with this
    // ──────────────────────────────────────────────────────────────────

    static void miniAudioDataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    int doAudioCallback(float* output, unsigned int nBufferFrames);

    // State machine helpers
    void advanceNextBar();    // generate next bar, handle step-up/count-in transitions
    void resetStateMachine(const EngineParams& p, bool withCountIn);

    PCMBuffer* currentSample(bool accent);
    void emitUiPulse(const AudioPulseEvent& ev);

    QMutex m_schedMutex;
    void detectSampleRateSafe();
    int  detectBestSampleRate();
    QMutex m_sampleMutex;
};
