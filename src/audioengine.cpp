#include "audioengine.h"
#include <QFile>
#include <QtEndian>
#include <cstring>
#include <qDebug>
#include <set>
#include <cmath>
#include <algorithm>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Helper: simple linear resampler (kept as fallback)
void resamplePCMBuffer(std::vector<float>& data, int srcRate, int dstRate) {
    if (srcRate == dstRate || data.empty()) return;

    double rateRatio = double(dstRate) / srcRate;
    size_t newLength = std::max<size_t>(1, size_t(data.size() * rateRatio));
    std::vector<float> resampled(newLength);

    for (size_t i = 0; i < newLength; ++i) {
        double srcPos = i / rateRatio;
        size_t idx0 = size_t(srcPos);
        size_t idx1 = std::min(idx0 + 1, data.size() - 1);
        double frac = srcPos - idx0;
        resampled[i] = float((1.0 - frac) * data[idx0] + frac * data[idx1]);
    }

    data.swap(resampled);
}

// Decode WAV (or other) resource bytes into float samples using miniaudio decoder.
// This decodes directly to the requested output sample rate and mono.
static bool decodeResourceToFloatMono(const QByteArray& wavData, std::vector<float>& outData, int& outSampleRate, int& outChannels, ma_decoder_config desiredConfig) {
    ma_decoder decoder;
    ma_result r = ma_decoder_init_memory(wavData.constData(), (ma_uint32)wavData.size(), &desiredConfig, &decoder);
    if (r != MA_SUCCESS) {
        return false;
    }

    outChannels = decoder.outputChannels;
    outSampleRate = decoder.outputSampleRate;

    ma_uint64 frameCount = 0;
    r = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (r != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);
        return false;
    }

    // Read as floats directly
    std::vector<float> temp;
    temp.resize(size_t(frameCount * outChannels));
    ma_uint64 framesRead = 0;
    r = ma_decoder_read_pcm_frames(&decoder, temp.data(), frameCount, &framesRead);
    ma_decoder_uninit(&decoder);

    if (r != MA_SUCCESS || framesRead == 0) {
        return false;
    }

    // If channel count > 1, mix to mono by averaging channels
    if (outChannels == 1) {
        outData = std::move(temp);
    } else {
        outData.clear();
        outData.resize(size_t(framesRead)); // 1 sample per frame
        for (ma_uint64 f = 0; f < framesRead; ++f) {
            double sum = 0.0;
            for (int c = 0; c < outChannels; ++c) {
                sum += temp[f * outChannels + c];
            }
            outData[size_t(f)] = float(sum / outChannels);
        }
        outChannels = 1;
    }
    return true;
}

// PCMBuffer::loadFromWavResource updated to store resourcePath and decode in-memory directly to device rate
bool PCMBuffer::loadFromWavResource(const QString& resourcePath_, int deviceRate) {
    resourcePath = resourcePath_;
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        valid = false;
        return false;
    }
    QByteArray wavData = file.readAll();
    file.close();

    // Preferred: decode to floats directly at deviceRate (mono)
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, deviceRate);
    std::vector<float> decoded;
    int decodedChannels = 0;
    int decodedRate = 0;
    bool ok = decodeResourceToFloatMono(wavData, decoded, decodedRate, decodedChannels, config);

    if (!ok) {
        // Fallback: try letting decoder choose sample rate then resample
        ma_decoder_config autoCfg = ma_decoder_config_init(ma_format_f32, 1, 0);
        ok = decodeResourceToFloatMono(wavData, decoded, decodedRate, decodedChannels, autoCfg);
        if (!ok) {
            valid = false;
            return false;
        }
        // resample to deviceRate
        if (decodedRate != deviceRate) {
            resamplePCMBuffer(decoded, decodedRate, deviceRate);
            decodedRate = deviceRate;
        }
    }

    // Fill PCMBuffer fields
    data = std::move(decoded);
    numChannels = 1;
    sampleRate = decodedRate;
    valid = !data.empty();

    // Find first nonzero sample (startSample)
    startSample = -1;
    const float EPS = 1e-5f;
    for (size_t i = 0; i < data.size(); ++i) {
        if (std::fabs(data[i]) > EPS) {
            startSample = int(i);
            break;
        }
    }
    if (startSample < 0) startSample = 0;
    return valid;
}

// Reload buffer from its stored resourcePath for a new device rate
bool PCMBuffer::reloadForDevice(int deviceRate)
{
    if (resourcePath.isEmpty()) {
        // No resource path stored: fallback to in-place resampling
        resampleTo(deviceRate);
        return valid;
    }
    return loadFromWavResource(resourcePath, deviceRate);
}

// PCMBuffer::resampleTo implementation (fallback)
void PCMBuffer::resampleTo(int dstRate)
{
    if (!valid || sampleRate == dstRate) return;

    int srcRate = sampleRate;
    int oldStart = startSample;

    // Perform resample
    resamplePCMBuffer(data, srcRate, dstRate);

    if (oldStart >= 0 && srcRate > 0) {
        double ratio = double(dstRate) / double(srcRate);
        startSample = int(std::round(oldStart * ratio));
        if (startSample < 0) startSample = 0;
        if (startSample >= int(data.size())) startSample = int(data.size()) - 1;
    } else {
        startSample = 0;
    }

    sampleRate = dstRate;
}

// ----- AUDIO ENGINE IMPLEMENTATION -----
AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
{
    // Initialize default values first
    m_bufferFrames = 256;
    m_volume = 0.8f;
    m_sinePhase = 0.0f;
    m_scheduleChanged = false;
    m_pendingScheduleChange = false;
    m_flushedRecently = false;
    m_hasPendingSchedule = false;
    m_pendingScheduleSwapSamplePos = -1;
    m_deviceInitialized = false;
    
    // Start with a default, but we'll detect the real rate when we initialize the device
    m_sampleRate = 44100;
    
    qDebug() << "AudioEngine: Initialized with default sample rate:" << m_sampleRate << "Hz (will auto-detect on start)";
}

bool AudioEngine::start(double bpm) {
    if (m_running.load()) return true;
    if (!m_deviceInitialized) {
        qWarning() << "AudioEngine: Device not initialized! Call initializeDevice() before start().";
        return false;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        return false;
    }
    m_running.store(true);
    setBpm(bpm);
    // Pre-roll: offset by one buffer period so the hardware has time to transition
    // from silence to active output before the first beat fires.  WASAPI (and some
    // other backends) produce a transient click on the very first samples written to
    // a freshly-opened audio session; this tiny delay keeps the first beat clear of it.
    m_globalSamplePos = -(int64_t)m_bufferFrames;
    m_pendingScheduleSwapSamplePos = -1;
    return true;
}

// Add this new detection method
int AudioEngine::detectBestSampleRate() {
    // Common sample rates in order of preference
    int preferredRates[] = {48000, 44100, 96000, 88200, 192000};
    int numRates = sizeof(preferredRates) / sizeof(preferredRates[0]);
    
    for (int i = 0; i < numRates; ++i) {
        ma_device_config testConfig = ma_device_config_init(ma_device_type_playback);
        testConfig.playback.format = ma_format_f32;
        testConfig.playback.channels = 1;
        testConfig.sampleRate = preferredRates[i];
        testConfig.dataCallback = nullptr; // No callback for test
        testConfig.pUserData = nullptr;
        
        ma_device testDevice;
        ma_result result = ma_device_init(nullptr, &testConfig, &testDevice);
        
        if (result == MA_SUCCESS) {
            int actualRate = testDevice.sampleRate;
            ma_device_uninit(&testDevice);
            
            // If the device accepted our preferred rate (or close to it), use it
            if (actualRate == preferredRates[i] || 
                (actualRate >= preferredRates[i] - 1000 && actualRate <= preferredRates[i] + 1000)) {
                return actualRate;
            }
        }
    }
    
    // If none of the preferred rates worked, try auto-detection
    ma_device_config autoConfig = ma_device_config_init(ma_device_type_playback);
    autoConfig.playback.format = ma_format_f32;
    autoConfig.playback.channels = 1;
    autoConfig.sampleRate = 0; // Let miniaudio choose
    autoConfig.dataCallback = nullptr;
    autoConfig.pUserData = nullptr;
    
    ma_device autoDevice;
    ma_result result = ma_device_init(nullptr, &autoConfig, &autoDevice);
    
    if (result == MA_SUCCESS) {
        int autoRate = autoDevice.sampleRate;
        ma_device_uninit(&autoDevice);
        return autoRate;
    }
    
    return -1; // Detection failed
}

AudioEngine::~AudioEngine() {
    stop();
    if (m_deviceInitialized) {
        ma_device_uninit(&m_device);
        m_deviceInitialized = false;
    }
}

void AudioEngine::setVolume(float vol) {
    m_volume = vol;
}

bool AudioEngine::loadSample(const QString& name, const QString& resourcePath) {
    PCMBuffer buf;
    // Prefer decoding directly to the current device sample rate (if device already known)
    int deviceRate = m_sampleRate;
    if (!buf.loadFromWavResource(resourcePath, deviceRate)) return false;
    QMutexLocker sampleLock(&m_sampleMutex);
    m_samples[name] = std::move(buf);
    return true;
}

void AudioEngine::setAccentSound(const QString& name) {
    m_accentSample = name;
}
void AudioEngine::setClickSound(const QString& name) {
    m_clickSample = name;
}

void AudioEngine::requestScheduleChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate) {
    QMutexLocker lock(&m_schedMutex);
    m_pendingPulseSchedule = pulses;
    m_pendingBarLengthSeconds = barLengthSeconds;
    m_pendingSampleRate = sampleRate;
    m_hasPendingSchedule = true;
}

void AudioEngine::schedulePulses(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate) {
    QMutexLocker lock(&m_schedMutex);
    m_pulseSchedule = pulses;
    m_pulseIdx = 0;
    m_scheduleLengthSamples = int(barLengthSeconds * sampleRate);
    m_scheduleSampleRate = sampleRate;
    m_barLengthSeconds = barLengthSeconds;
    m_scheduleChanged = true;

    m_flushedRecently = false;
    m_hasPendingSchedule = false;
}

void AudioEngine::scheduleTempoChange(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate) {
    requestScheduleChange(pulses, barLengthSeconds, sampleRate);
}

void AudioEngine::setPulseCallback(PulseCallback cb) {
    m_pulseCallback = cb;
}

void AudioEngine::emitUiPulse(const AudioPulseEvent& ev) {
    // Stamp current run ID so the receiver can discard signals from old sessions.
    AudioPulseEvent tagged = ev;
    tagged.runId = m_runId.load();
    emit pulseUiEvent(tagged);
    if (m_pulseCallback) {
        m_pulseCallback(ev);
    }
}

PCMBuffer* AudioEngine::currentSample(bool accent) {
    QString key = accent ? m_accentSample : m_clickSample;
    QMutexLocker sampleLock(&m_sampleMutex);
    if (!m_samples.contains(key)) return nullptr;
    return &m_samples[key];
}

void AudioEngine::stop() {
    if (!m_running.load()) return;
    m_running.store(false);
    if (m_deviceInitialized) {
        ma_device_stop(&m_device);
    }
    m_globalSamplePos = 0;
    activeSamples.clear();
    m_pendingScheduleSwapSamplePos = -1;
}

void AudioEngine::setBpm(double bpm) {
    // No-op, sample rate and scheduling handled elsewhere
}

void AudioEngine::miniAudioDataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount) {
    AudioEngine* engine = reinterpret_cast<AudioEngine*>(pDevice->pUserData);
    engine->doAudioCallback(reinterpret_cast<float*>(pOutput), frameCount);
}

// =============================================================================
// FREE FUNCTION: buildBarSchedule
// =============================================================================
// Compute the pulse schedule for one bar given engine params.
// isCountIn = true  â†’ produce count-in beats (idx < 0) at the bar tempo
// isCountIn = false â†’ produce normal subdivision / polyrhythm pulses
// All samplePosInBar values are relative (0 = bar start).
// This is the implementation for the declaration in audioengine.h.

static int bbs_lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int x = a, y = b;
    while (y != 0) { int t = y; y = x % y; x = t; }
    return (a / x) * b;
}

BarSchedule buildBarSchedule(const EngineParams& params, bool isCountIn, int sampleRate) {
    BarSchedule result;

    bool compound      = (params.denominator == 8) &&
                         (params.numerator % 3 == 0) &&
                         (params.numerator > 3);
    int  beatsPerBar   = compound ? (params.numerator / 3) : params.numerator;
    double secPerBeat  = 60.0 / params.bpm;
    int    smpPerBeat  = int(sampleRate * secPerBeat);
    double barLenSec   = beatsPerBar * secPerBeat;
    int    barLenSmp   = beatsPerBar * smpPerBeat;

    // â”€â”€ COUNT-IN bar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (isCountIn) {
        int pulseCount = beatsPerBar;      // compound: dotted-quarter beats; simple: quarter beats
        for (int i = 0; i < pulseCount; ++i) {
            AudioPulseEvent ev;
            ev.idx          = -1000 + i;
            ev.accent       = (i == 0);
            ev.polyAccent   = false;
            ev.isBeat       = true;
            ev.playPulse    = true;
            ev.isRest       = false;
            ev.gridColumn   = -1;
            ev.samplePosInBar = i * smpPerBeat;
            ev.startOfCycle = (i == 0);
            result.pulses.push_back(ev);
        }
        result.barLengthSamples = int64_t(pulseCount) * smpPerBeat;
        return result;
    }

    // â”€â”€ POLYRHYTHM bar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (params.polyrhythmEnabled &&
        params.polyMain > 0 && params.polySecondary > 0)
    {
        int mainBeats = params.polyMain;
        int polyBeats = params.polySecondary;
        int columns   = bbs_lcm(mainBeats, polyBeats);

        struct PI { double time; int col; bool isMain, isPoly; };
        std::vector<PI> pulses;
        for (int i = 0; i < mainBeats; ++i) {
            double t = i * barLenSec / mainBeats;
            int col  = int(std::round(t / barLenSec * columns));
            pulses.push_back({t, col, true, false});
        }
        for (int i = 0; i < polyBeats; ++i) {
            double t = i * barLenSec / polyBeats;
            int col  = int(std::round(t / barLenSec * columns));
            pulses.push_back({t, col, false, true});
        }
        std::sort(pulses.begin(), pulses.end(),
                  [](const PI& a, const PI& b){ return a.time < b.time; });

        // Merge coincident pulses
        std::vector<PI> merged;
        const double eps = 1e-6;
        for (const auto& pi : pulses) {
            if (!merged.empty() && std::abs(pi.time - merged.back().time) < eps) {
                merged.back().isMain |= pi.isMain;
                merged.back().isPoly |= pi.isPoly;
            } else {
                merged.push_back(pi);
            }
        }

        for (size_t idx = 0; idx < merged.size(); ++idx) {
            const auto& pi = merged[idx];
            AudioPulseEvent ev;
            ev.idx          = int(idx);
            ev.gridColumn   = pi.col;
            ev.isBeat       = (pi.isMain && idx == 0);
            ev.accent       = pi.isMain;
            ev.polyAccent   = pi.isPoly;
            ev.playPulse    = true;
            ev.isRest       = false;
            ev.samplePosInBar = int(std::round(pi.time * sampleRate));
            ev.startOfCycle = (idx == 0);
            result.pulses.push_back(ev);
        }
        result.barLengthSamples = barLenSmp;
        return result;
    }

    // â”€â”€ SUBDIVISION bar â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    const SubdivisionPattern& pat = params.subdivision;
    int subdivs = int(pat.pulses.size());
    if (subdivs == 0) subdivs = 1;

    double totalPatternDuration = 0.0;
    for (const auto& p : pat.pulses)
        totalPatternDuration += noteValueBeatFraction(p.noteValue, compound);

    const double TOLERANCE = 1e-3;
    bool isStandard = false;
    if (pat.category == SubdivisionCategory::Custom) {
        isStandard = false;
    } else if (compound && pat.pulses.size() == 1 &&
               std::abs(noteValueBeatFraction(pat.pulses[0].noteValue, compound) - 1.0) < TOLERANCE) {
        isStandard = true;
    } else {
        isStandard = (std::abs(totalPatternDuration - 1.0) < TOLERANCE);
    }

    if (isStandard) {
        // Standard: repeat pattern once per beat
        for (int b = 0; b < beatsPerBar; ++b) {
            double beatStartSec  = b * secPerBeat;
            double pulseOffsetSec = 0.0;
            for (int s = 0; s < subdivs; ++s) {
                AudioPulseEvent ev;
                ev.idx        = b * subdivs + s;
                ev.gridColumn = -1;
                ev.isBeat     = (s == 0);
                if (pat.category == SubdivisionCategory::Custom)
                    ev.accent = pat.pulses[s].accent;
                else
                    ev.accent = (s == 0 && int(params.accents.size()) > b && params.accents[b]);
                ev.polyAccent   = false;
                ev.isRest       = pat.pulses[s].isRest;
                ev.playPulse    = !ev.isRest;
                double dur      = secPerBeat * noteValueBeatFraction(pat.pulses[s].noteValue, compound);
                ev.samplePosInBar = int(std::round((beatStartSec + pulseOffsetSec) * sampleRate));
                ev.startOfCycle = (b == 0 && s == 0);
                pulseOffsetSec += dur;
                result.pulses.push_back(ev);
            }
        }
        result.barLengthSamples = barLenSmp;
    } else {
        // Custom / variable-length pattern fills one "bar"
        double actualDurSec = 0.0;
        for (const auto& p : pat.pulses)
            actualDurSec += noteValueBeatFraction(p.noteValue, compound) * secPerBeat;

        std::vector<double> beatStartTimes;
        for (int b = 0; b < beatsPerBar; ++b)
            beatStartTimes.push_back(b * secPerBeat);

        double pulseOffsetSec = 0.0;
        for (int s = 0; s < int(pat.pulses.size()); ++s) {
            const auto& p = pat.pulses[s];
            AudioPulseEvent ev;
            ev.idx        = s;
            ev.gridColumn = -1;
            ev.isBeat     = false;
            double pulseStartSec = pulseOffsetSec;
            for (int b = 0; b < beatsPerBar; ++b) {
                if (std::abs(pulseStartSec - beatStartTimes[b]) < 1e-6) {
                    ev.isBeat = true;
                    break;
                }
            }
            ev.accent       = p.accent;
            ev.isBeat       = (s == 0) ? true : ev.isBeat;
            ev.polyAccent   = false;
            ev.isRest       = p.isRest;
            ev.playPulse    = !ev.isRest;
            double dur      = secPerBeat * noteValueBeatFraction(p.noteValue, compound);
            ev.samplePosInBar = int(std::round(pulseOffsetSec * sampleRate));
            ev.startOfCycle = (s == 0);
            pulseOffsetSec += dur;
            result.pulses.push_back(ev);
        }
        result.barLengthSamples = int64_t(std::round(actualDurSec * sampleRate));
    }
    return result;
}

// =============================================================================
// NEW BAR-ADVANCE STATE MACHINE
// =============================================================================

// resetStateMachine â€” called under m_schedMutex from startWithParams().
void AudioEngine::resetStateMachine(const EngineParams& p, bool withCountIn)
{
    m_engineParams         = p;
    m_currentTempo         = p.bpm;
    m_paramsChanged        = false;
    m_scheduledPulses.clear();
    activeSamples.clear();
    m_barNumberForUi          = 0;
    m_playingBarSamplesAccum  = 0;
    m_pendingStepUpTempoForTag = 0;  // never carry a stale step-up into a new session

    // Pre-roll: one buffer period of silence so the hardware audio session
    // has time to open cleanly before the first beat fires.
    m_globalSamplePos = -(int64_t)m_bufferFrames;
    m_nextBarStart    = 0;  // first bar starts at sample 0

    if (withCountIn) {
        m_playState       = EnginePlayState::CountIn;
        m_countInBarsLeft = 1;
    } else {
        m_playState       = EnginePlayState::Playing;
        m_countInBarsLeft = 0;
    }

    // Pre-generate the first bar so events are ready before playback starts.
    advanceNextBar();
}

// advanceNextBar â€” called under m_schedMutex.
// Generates the NEXT bar's events and appends them to m_scheduledPulses.
// Also applies the post-generation state transition (count-inâ†’playing,
// speed-trainer step-up) that affects the bar AFTER the one just generated.
void AudioEngine::advanceNextBar()
{
    if (m_playState == EnginePlayState::Idle) return;

    bool isCountIn = (m_playState == EnginePlayState::CountIn);

    // Build the bar schedule using current tempo.
    EngineParams p = m_engineParams;
    p.bpm = m_currentTempo;
    BarSchedule bar = buildBarSchedule(p, isCountIn, m_sampleRate);
    if (bar.barLengthSamples <= 0) return;

    // Tag and append events with absolute sample positions.
    bool firstPulse = true;
    for (AudioPulseEvent ev : bar.pulses) {
        ev.barNumber    = m_barNumberForUi;
        ev.barsPerStep  = m_engineParams.barsPerStep;
        ev.isFirstInBar = firstPulse;
        // Tag the new tempo on the very first pulse of the first post-step-up bar.
        // This fires on the main thread exactly when that audio plays — not 2 s early.
        if (firstPulse && m_pendingStepUpTempoForTag > 0) {
            ev.newTempo = m_pendingStepUpTempoForTag;
            m_pendingStepUpTempoForTag = 0;
        }
        firstPulse      = false;
        int64_t absPos  = m_nextBarStart + int64_t(ev.samplePosInBar);
        m_scheduledPulses.push_back({absPos, ev});
    }

    m_nextBarStart += bar.barLengthSamples;
    m_barNumberForUi++;

    // â”€â”€ Post-generation state transition â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (isCountIn) {
        m_countInBarsLeft--;
        if (m_countInBarsLeft <= 0) {
            m_playState              = EnginePlayState::Playing;
            m_playingBarSamplesAccum = 0;
            m_barNumberForUi         = 0;  // bar counter restarts when main playing begins
        }
    } else {
        // Playing bar just generated -- check speed trainer step-up.
        bool cpd2    = (p.denominator == 8) && (p.numerator % 3 == 0) && (p.numerator > 3);
        int  bpbFull = cpd2 ? (p.numerator / 3) : p.numerator;

        bool isCustomBarPath = (!p.polyrhythmEnabled &&
                                p.subdivision.category == SubdivisionCategory::Custom);

        int64_t target;
        if (isCustomBarPath) {
            // For custom patterns: count playthroughs. One full cycle of the
            // beat indicator (bpb playthroughs) = 1 bar for speed trainer.
            // barsPerStep=1 means all 4 big circles complete once before step-up.
            m_playingBarSamplesAccum++;
            target = int64_t(m_engineParams.barsPerStep) * int64_t(bpbFull);
        } else {
            // Standard/polyrhythm: accumulate actual sample duration.
            m_playingBarSamplesAccum += bar.barLengthSamples;
            int64_t smpPerFullBar = int64_t(bpbFull) *
                                    int64_t(double(m_sampleRate) * 60.0 / double(p.bpm));
            target = int64_t(m_engineParams.barsPerStep) * smpPerFullBar;
        }

        if (m_engineParams.speedEnabled
            && m_currentTempo < m_engineParams.maxTempo
            && target > 0
            && m_playingBarSamplesAccum >= target)
        {
            int newTempo = qMin(m_currentTempo + m_engineParams.tempoStep,
                                m_engineParams.maxTempo);
            m_currentTempo           = newTempo;
            m_playingBarSamplesAccum = 0;
            m_barNumberForUi         = 0;

            // Tag the new tempo on the FIRST PULSE of the next bar so the main
            // thread learns about the step-up exactly when that audio plays.
            m_pendingStepUpTempoForTag = newTempo;

            if (m_engineParams.countInEnabled) {
                m_playState       = EnginePlayState::CountIn;
                m_countInBarsLeft = 1;
            }
        }
    }
}

// setEngineParams â€” safe to call from main thread while running.
// Changes take effect at the next bar boundary.
void AudioEngine::setEngineParams(const EngineParams& p)
{
    QMutexLocker lock(&m_schedMutex);
    m_engineParams  = p;
    m_paramsChanged = true;
    // Sync m_currentTempo so tempo changes take effect at the next bar
    // when the speed trainer is not actively stepping up.
    if (!p.speedEnabled)
        m_currentTempo = p.bpm;
}

// startWithParams â€” stops any current playback and restarts with new params.
void AudioEngine::startWithParams(const EngineParams& p, bool withCountIn)
{
    // Stop the device while we reset state (outside m_schedMutex to avoid deadlock).
    if (m_running.load()) {
        m_running.store(false);
        if (m_deviceInitialized)
            ma_device_stop(&m_device);
    }

    {
        QMutexLocker lock(&m_schedMutex);
        resetStateMachine(p, withCountIn);
    }

    // Increment run ID so any queued pulseUiEvent signals from the old session
    // carry a stale ID and will be discarded by MetronomeEngine::onAudioPulse.
    m_runId.fetch_add(1);

    if (!m_deviceInitialized) return;
    m_running.store(true);
    ma_device_start(&m_device);
}

// =============================================================================
// AUDIO CALLBACK  (core mixing loop â€” runs on audio thread, new state machine)
// =============================================================================
int AudioEngine::doAudioCallback(float* output, unsigned int nBufferFrames) {
    for (unsigned int i = 0; i < nBufferFrames; ++i)
        output[i] = 0.0f;

    if (!m_running.load())
        return 0;

    QMutexLocker lock(&m_schedMutex);

    // â”€â”€ Runtime device sample-rate change â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (m_deviceInitialized && m_device.sampleRate != m_sampleRate) {
        int oldRate = m_sampleRate;
        int newRate = m_device.sampleRate;
        if (oldRate > 0 && newRate > 0) {
            double ratio = double(newRate) / double(oldRate);
            {
                QMutexLocker sampleLock(&m_sampleMutex);
                activeSamples.clear();
                for (auto it = m_samples.begin(); it != m_samples.end(); ++it) {
                    PCMBuffer &buf = it.value();
                    if (buf.valid && buf.sampleRate != newRate) {
                        if (!buf.resourcePath.isEmpty())
                            buf.reloadForDevice(newRate);
                        else
                            buf.resampleTo(newRate);
                    }
                }
            }
            // Rescale state-machine positions
            for (auto& sp : m_scheduledPulses)
                sp.samplePos = int64_t(std::round(sp.samplePos * ratio));
            m_nextBarStart    = int64_t(std::round(m_nextBarStart * ratio));
            m_globalSamplePos = int64_t(std::round(m_globalSamplePos * ratio));
            m_sampleRate = newRate;
        } else {
            QMutexLocker sampleLock(&m_sampleMutex);
            activeSamples.clear();
            m_scheduledPulses.clear();
            m_globalSamplePos = 0;
            m_sampleRate = newRate;
        }
    }

    int64_t bufferStart = m_globalSamplePos;
    int64_t bufferEnd   = bufferStart + int64_t(nBufferFrames);

    // â”€â”€ Pre-generate bars to maintain a 2-second lookahead â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    if (m_playState != EnginePlayState::Idle) {
        int64_t lookahead = bufferEnd + int64_t(m_sampleRate) * 2;
        while (m_nextBarStart < lookahead) {
            int64_t prevNext = m_nextBarStart;
            advanceNextBar();
            if (m_nextBarStart == prevNext) break;  // guard against zero-length bar
            if (m_playState == EnginePlayState::Idle) break;
        }
    }

    // â”€â”€ Fire scheduled pulses in this buffer window â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (const ScheduledPulse& sp : m_scheduledPulses) {
        if (sp.samplePos < bufferStart || sp.samplePos >= bufferEnd) continue;
        int outPos = int(sp.samplePos - bufferStart);
        if (sp.ev.playPulse) {
            PCMBuffer* buf = currentSample(sp.ev.accent);
            if (buf && buf->valid)
                activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
        }
        emitUiPulse(sp.ev);
    }

    // â”€â”€ Prune events that have already been delivered â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    m_scheduledPulses.erase(
        std::remove_if(m_scheduledPulses.begin(), m_scheduledPulses.end(),
            [bufferEnd](const ScheduledPulse& sp) { return sp.samplePos < bufferEnd; }),
        m_scheduledPulses.end());

    // â”€â”€ Mix active samples â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    for (auto it = activeSamples.begin(); it != activeSamples.end(); ) {
        int samplesLeft  = int(it->data->size()) - it->pos;
        int bufferSpace  = int(nBufferFrames) - it->outPos;
        int samplesToMix = std::min(samplesLeft, bufferSpace);
        for (int k = 0; k < samplesToMix; ++k)
            output[it->outPos + k] += (*it->data)[it->pos + k] * it->volume;
        it->pos    += samplesToMix;
        it->outPos += samplesToMix;
        if (it->pos >= int(it->data->size()))
            it = activeSamples.erase(it);
        else if (it->outPos >= int(nBufferFrames)) {
            it->outPos = 0;
            ++it;
        } else {
            ++it;
        }
    }

    m_globalSamplePos += nBufferFrames;
    return 0;
}


void AudioEngine::flushBarAndReset() {
    QMutexLocker lock(&m_schedMutex);
    m_globalSamplePos = 0;
    activeSamples.clear();
    m_scheduleChanged = true;
    m_flushedRecently = true;
    m_pendingScheduleSwapSamplePos = -1;
}

// Immediately replaces the schedule and resets playback to sample 0.
// Unlike flushBarAndReset+requestScheduleChange, this avoids the pending-swap
// path entirely so beat 1 of the new schedule fires without deduplication.
void AudioEngine::scheduleNow(const std::vector<AudioPulseEvent>& pulses, double barLengthSeconds, int sampleRate) {
    QMutexLocker lock(&m_schedMutex);
    m_pulseSchedule             = pulses;
    m_pulseIdx                  = 0;
    m_scheduleLengthSamples     = int(barLengthSeconds * sampleRate);
    m_scheduleSampleRate        = sampleRate;
    m_barLengthSeconds          = barLengthSeconds;
    // Start 1 sample past zero: beat-1 (samplePosInBar=0) is now in the "past"
    // for the first buffer, so it won't re-fire after the old beat-1 click.
    // Do NOT clear activeSamples â€” old click should decay naturally.
    m_globalSamplePos           = 1;
    m_scheduleChanged           = true;
    m_flushedRecently           = false;   // cancel any pending flushAtNextBarBoundary
    m_hasPendingSchedule        = false;   // cancel any pending requestScheduleChange
    m_pendingScheduleSwapSamplePos = -1;   // no deduplication â€” beat 1 fires normally
}

void AudioEngine::flushAtNextBarBoundary() {
    QMutexLocker lock(&m_schedMutex);
    m_scheduleChanged = true;
    m_flushedRecently = true;
    // Do NOT reset m_globalSamplePos!
    // Do NOT clear activeSamples!
}

bool AudioEngine::initializeDevice(double bpm) {
    if (m_deviceInitialized) return true;

    int detectedRate = detectBestSampleRate();
    if (detectedRate > 0) {
        m_sampleRate = detectedRate;
        qDebug() << "AudioEngine: Auto-detected sample rate:" << m_sampleRate << "Hz (init only)";
    } else {
        qDebug() << "AudioEngine: Auto-detection failed, using default:" << m_sampleRate << "Hz";
    }

    m_deviceConfig = ma_device_config_init(ma_device_type_playback);
    m_deviceConfig.playback.format   = ma_format_f32;
    m_deviceConfig.playback.channels = 1;
    m_deviceConfig.sampleRate        = m_sampleRate;
    m_deviceConfig.dataCallback      = &AudioEngine::miniAudioDataCallback;
    m_deviceConfig.pUserData         = this;

    if (ma_device_init(nullptr, &m_deviceConfig, &m_device) != MA_SUCCESS) {
        return false;
    }

    // ALWAYS reload samples whose stored rate doesn't match the actual device rate.
    // This must be unconditional because detectBestSampleRate() pre-assigns m_sampleRate
    // to the detected rate before we get here, so the old guard
    // (if m_device.sampleRate != m_sampleRate) was always false â€” samples loaded at the
    // constructor default of 44100 would silently stay at 44100 even when the device runs
    // at 48000, causing a pitch shift on first play.
    {
        int deviceRate = (int)m_device.sampleRate;
        QMutexLocker sampleLock(&m_sampleMutex);
        for (auto it = m_samples.begin(); it != m_samples.end(); ++it) {
            PCMBuffer &buf = it.value();
            if (buf.valid && buf.sampleRate != deviceRate) {
                qDebug() << "AudioEngine: Reloading sample" << it.key()
                         << "from" << buf.sampleRate << "Hz to" << deviceRate << "Hz";
                if (!buf.resourcePath.isEmpty()) {
                    bool ok = buf.reloadForDevice(deviceRate);
                    if (!ok) buf.resampleTo(deviceRate);
                } else {
                    buf.resampleTo(deviceRate);
                }
            }
        }
        if (m_sampleRate != deviceRate)
            qDebug() << "AudioEngine: Device actual rate:" << deviceRate << "Hz";
        m_sampleRate = deviceRate;
    }

    m_deviceInitialized = true;
    // Capture the actual device period size so the pre-roll in start() is exactly right
    if (m_device.playback.internalPeriodSizeInFrames > 0)
        m_bufferFrames = (int)m_device.playback.internalPeriodSizeInFrames;
    m_globalSamplePos = 0; // Defensive
    return true;
}
