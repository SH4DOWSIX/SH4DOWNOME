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
    emit pulseUiEvent(ev);
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

// ... rest of the doAudioCallback and other methods remain the same ...
int AudioEngine::doAudioCallback(float* output, unsigned int nBufferFrames) {
    for (unsigned int i = 0; i < nBufferFrames; ++i)
        output[i] = 0.0f;

    if (!m_running.load())
        return 0;

    QMutexLocker lock(&m_schedMutex);

    // RUNTIME DEVICE SAMPLE RATE CHANGE HANDLING:
    // If the underlying miniaudio device's sampleRate has changed after initialization,
    // reload buffers to device rate (preferred) or resample as fallback.
    if (m_deviceInitialized && m_device.sampleRate != m_sampleRate) {
        int oldRate = m_sampleRate;
        int newRate = m_device.sampleRate;
        if (oldRate > 0 && newRate > 0) {
            // Protect sample map while we reload
            QMutexLocker sampleLock(&m_sampleMutex);

            // Clear active samples to avoid pointer/position inconsistencies while we transform buffers
            activeSamples.clear();

            double ratio = double(newRate) / double(oldRate);

            // Reload or resample loaded sample buffers
            for (auto it = m_samples.begin(); it != m_samples.end(); ++it) {
                PCMBuffer &buf = it.value();
                if (buf.valid && buf.sampleRate != newRate) {
                    // Try to reload from original resource (better quality than in-place resample)
                    if (!buf.resourcePath.isEmpty()) {
                        bool reloadOk = buf.reloadForDevice(newRate);
                        if (!reloadOk) {
                            // Fallback to in-place resample
                            buf.resampleTo(newRate);
                        }
                    } else {
                        buf.resampleTo(newRate);
                    }
                }
            }

            // Scale schedule sample positions so they still point to the same musical times
            for (auto &ev : m_pulseSchedule) {
                ev.samplePosInBar = int(std::round(ev.samplePosInBar * ratio));
            }
            for (auto &ev : m_pendingPulseSchedule) {
                ev.samplePosInBar = int(std::round(ev.samplePosInBar * ratio));
            }

            // Adjust schedule length/sample rate counters
            m_scheduleLengthSamples = int(std::round(m_scheduleLengthSamples * ratio));
            m_scheduleSampleRate = newRate;
            m_pendingSampleRate = int(std::round(m_pendingSampleRate * ratio));
            m_globalSamplePos = int(std::round(m_globalSamplePos * ratio));

            // Adopt new sample rate
            m_sampleRate = newRate;
        } else {
            // If we can't compute ratio safely, just adopt new rate and clear scheduling to be safe
            QMutexLocker sampleLock(&m_sampleMutex);
            activeSamples.clear();
            m_pulseSchedule.clear();
            m_pendingPulseSchedule.clear();
            m_scheduleLengthSamples = 0;
            m_scheduleSampleRate = newRate;
            m_pendingSampleRate = newRate;
            m_globalSamplePos = 0;
            m_sampleRate = newRate;
        }
    }

    int64_t bufferStartSample = m_globalSamplePos;
    int64_t bufferEndSample   = bufferStartSample + int64_t(nBufferFrames);

    // --- SCHEDULE SWAP LOGIC ---
    if (m_flushedRecently && m_hasPendingSchedule) {
        m_pulseSchedule = m_pendingPulseSchedule;
        m_pulseIdx = 0;
        m_scheduleLengthSamples = int(m_pendingBarLengthSeconds * m_pendingSampleRate);
        m_scheduleSampleRate = m_pendingSampleRate;
        m_barLengthSeconds = m_pendingBarLengthSeconds;
        m_hasPendingSchedule = false;
        m_flushedRecently = false;
        activeSamples.clear();

        int schedulePulseCount = static_cast<int>(m_pulseSchedule.size());
        int64_t barStart = m_globalSamplePos / int64_t(m_scheduleLengthSamples);
        int64_t barSampleOffset = barStart * int64_t(m_scheduleLengthSamples);

        // Set swap position for deduplication at boundary
        m_pendingScheduleSwapSamplePos = m_globalSamplePos;

        for (int pulseIdx = 0; pulseIdx < schedulePulseCount; ++pulseIdx) {
            AudioPulseEvent& ev = m_pulseSchedule[pulseIdx];
            int64_t pulseGlobal = barSampleOffset + int64_t(ev.samplePosInBar);

            // Skip pulse at swap boundary
            if (pulseGlobal == m_pendingScheduleSwapSamplePos) {
                continue;
            }

            if (pulseGlobal < m_globalSamplePos || pulseGlobal >= m_globalSamplePos + int64_t(nBufferFrames))
                continue;
            if (ev.playPulse) {
                PCMBuffer* buf = currentSample(ev.accent);
                if (buf && buf->valid) {
                    int outPos = int(pulseGlobal - m_globalSamplePos);
                    activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                }
                emitUiPulse(ev);
            }
        }
        // Reset swap position after first buffer
        m_pendingScheduleSwapSamplePos = -1;
        m_globalSamplePos += nBufferFrames;
        return 0;
    }

    bool scheduleJustChanged = false;
    if (m_pendingScheduleChange && (bufferStartSample % nBufferFrames == 0)) {
        activeSamples.clear();
        m_pulseSchedule = m_nextPulseSchedule;
        m_pulseIdx = 0;
        m_scheduleLengthSamples = m_nextScheduleLengthSamples;
        m_scheduleSampleRate = m_nextScheduleSampleRate;
        m_barLengthSeconds = m_nextBarLengthSeconds;
        m_pendingScheduleChange = false;
        m_scheduleChanged = false;
        scheduleJustChanged = true;
    }

    int scheduleSamples = m_scheduleLengthSamples;
    int schedulePulseCount = static_cast<int>(m_pulseSchedule.size());

    // DETECT MODE: Check if this is polyrhythm or subdivision mode
    bool isPolyrhythmMode = false;
    bool hasCountIn = false;
    int countInEndSample = 0;
    double customPatternDuration = 0.0;
    
    // Check for polyrhythm mode (has gridColumn values)
    for (const auto& ev : m_pulseSchedule) {
        if (ev.gridColumn >= 0) {
            isPolyrhythmMode = true;
            break;
        }
    }
    
    // FIXED: Check for count-in events regardless of mode
    for (const auto& ev : m_pulseSchedule) {
        if (ev.idx < 0) {
            hasCountIn = true;
        } else if (ev.idx == -9999) {
            countInEndSample = ev.samplePosInBar;
            break;
        }
    }
    
    // Calculate custom pattern duration (only used in subdivision mode)
    if (!isPolyrhythmMode && hasCountIn) {
        customPatternDuration = m_barLengthSeconds - (countInEndSample / double(m_scheduleSampleRate));
    }

    // Simple offset calculation
    int offsetSamples = countInEndSample;

    if (scheduleJustChanged) {
        for (int pulseIdx = 0; pulseIdx < schedulePulseCount; ++pulseIdx) {
            AudioPulseEvent& ev = m_pulseSchedule[pulseIdx];
            int64_t pulseGlobal = int64_t(ev.samplePosInBar);

            if (pulseGlobal < bufferStartSample || pulseGlobal >= bufferEndSample)
                continue;

            if (m_pendingScheduleSwapSamplePos >= 0 && pulseGlobal == m_pendingScheduleSwapSamplePos) {
                m_pendingScheduleSwapSamplePos = -1;
                continue;
            }

            int outPos = int(pulseGlobal - bufferStartSample);
            if (ev.idx < 0 || ev.idx == -9999) {
                if (ev.playPulse) {
                    PCMBuffer* buf = currentSample(ev.accent);
                    if (buf && buf->valid)
                        activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                    emitUiPulse(ev);
                }
            } else {
                if (ev.samplePosInBar >= offsetSamples) {
                    if (ev.playPulse) {
                        PCMBuffer* buf = currentSample(ev.accent);
                        if (buf && buf->valid)
                            activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                        emitUiPulse(ev);
                    }
                }
            }
        }
    } else {
        // FIXED: Mode-specific cycle generation
        
        if (isPolyrhythmMode) {
            // POLYRHYTHM MODE: Uses time signature bar length, repeats entire schedule
            int barLengthSamples = int(m_barLengthSeconds * m_scheduleSampleRate);
            
            // FIXED: Use a set to track which sample positions we've already scheduled
            std::set<int64_t> scheduledSamples;
            
            for (int pulseIdx = 0; pulseIdx < schedulePulseCount; ++pulseIdx) {
                AudioPulseEvent& ev = m_pulseSchedule[pulseIdx];
                int basePosition = ev.samplePosInBar;
                
                // FIXED: Handle count-in events in polyrhythm mode
                if (ev.idx < 0) {
                    // Count-in events - only play once at the beginning
                    int64_t pulseGlobal = int64_t(basePosition);
                    if (pulseGlobal >= bufferStartSample && pulseGlobal < bufferEndSample && ev.playPulse) {
                        if (scheduledSamples.find(pulseGlobal) == scheduledSamples.end()) {
                            scheduledSamples.insert(pulseGlobal);
                            PCMBuffer* buf = currentSample(ev.accent);
                            if (buf && buf->valid) {
                                int outPos = int(pulseGlobal - bufferStartSample);
                                if (outPos >= 0 && outPos < int(nBufferFrames)) {
                                    activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                                }
                            }
                            emitUiPulse(ev);
                        }
                    }
                    continue; // Skip the repeating logic for count-in events
                }
                
                // Skip gap events
                if (ev.idx == -9999) {
                    continue;
                }
                
                // Compute starting bar index directly to avoid O(session_length) while-loop.
                // Use float-computed bar positions to prevent cumulative timing drift.
                int64_t baseOffset = hasCountIn ? int64_t(countInEndSample) : int64_t(0);
                int64_t relBufferStart = int64_t(bufferStartSample) - baseOffset - int64_t(basePosition);
                int64_t barIdx = (relBufferStart > 0) ? (relBufferStart / int64_t(barLengthSamples) - 1) : 0;
                if (barIdx < 0) barIdx = 0;

                for (int i = 0; i < 5; ++i, ++barIdx) {
                    // Float-computed bar start to prevent per-bar integer truncation accumulation
                    int64_t barStartSample = baseOffset + int64_t(std::round(double(barIdx) * m_barLengthSeconds * double(m_scheduleSampleRate)));
                    int64_t pulseGlobal = barStartSample + int64_t(basePosition);

                    if (pulseGlobal >= bufferEndSample) break;

                    if (pulseGlobal >= bufferStartSample && ev.playPulse) {
                        // FIXED: Check if we've already scheduled this sample position
                        if (scheduledSamples.find(pulseGlobal) == scheduledSamples.end()) {
                            scheduledSamples.insert(pulseGlobal);

                            PCMBuffer* buf = currentSample(ev.accent);
                            if (buf && buf->valid) {
                                int outPos = int(pulseGlobal - bufferStartSample);
                                if (outPos >= 0 && outPos < int(nBufferFrames)) {
                                    activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                                }
                            }
                            emitUiPulse(ev);
                        }
                    }
                }
            }
        } else {
            // SUBDIVISION MODE: Float-computed positions for drift-free timing.
            // Each cycle's start is derived from bar length in seconds, not cumulative
            // integer addition — this eliminates inter-bar drift over long sessions.
            double purePatternDuration_s = hasCountIn ? customPatternDuration : m_barLengthSeconds;
            // Guard: if count-in duration == bar duration, customPatternDuration is 0.
            // Fall back to the bar length so cycles repeat at the correct rate.
            if (purePatternDuration_s <= 0.0) purePatternDuration_s = m_barLengthSeconds;
            int purePatternSamples = int(std::round(purePatternDuration_s * double(m_scheduleSampleRate)));
            if (purePatternSamples <= 0) purePatternSamples = scheduleSamples;

            // Tight cycle window: only check cycles that could overlap this buffer
            int64_t startCycle = int64_t(bufferStartSample) / int64_t(purePatternSamples);
            if (startCycle > 0) startCycle--;
            int64_t endCycle = int64_t(bufferEndSample) / int64_t(purePatternSamples) + 2;

            for (int64_t cycle = startCycle; cycle <= endCycle; ++cycle) {
                int64_t cycleStartSample;

                if (cycle == 0) {
                    cycleStartSample = 0;
                } else if (hasCountIn) {
                    // Float-computed position from cycle start to eliminate cumulative integer drift
                    cycleStartSample = int64_t(countInEndSample) +
                        int64_t(std::round(double(cycle) * purePatternDuration_s * double(m_scheduleSampleRate)));
                } else {
                    // Float-computed absolute position for drift-free beat placement
                    cycleStartSample = int64_t(std::round(double(cycle) * m_barLengthSeconds * double(m_scheduleSampleRate)));
                }

                for (int pulseIdx = 0; pulseIdx < schedulePulseCount; ++pulseIdx) {
                    AudioPulseEvent& ev = m_pulseSchedule[pulseIdx];
                    int64_t pulseGlobal;

                    if (cycle == 0) {
                        pulseGlobal = int64_t(ev.samplePosInBar);
                    } else {
                        if (ev.idx < 0 || ev.idx == -9999) continue;
                        int64_t patternRelPos = hasCountIn ? int64_t(ev.samplePosInBar - countInEndSample)
                                                           : int64_t(ev.samplePosInBar);
                        pulseGlobal = cycleStartSample + patternRelPos;
                    }

                    if (pulseGlobal < bufferStartSample || pulseGlobal >= bufferEndSample)
                        continue;

                    if (m_pendingScheduleSwapSamplePos >= 0 && pulseGlobal == m_pendingScheduleSwapSamplePos) {
                        m_pendingScheduleSwapSamplePos = -1;
                        continue;
                    }

                    int outPos = int(pulseGlobal - bufferStartSample);

                    if (cycle == 0) {
                        if (ev.idx < 0) {
                            if (ev.playPulse) {
                                PCMBuffer* buf = currentSample(ev.accent);
                                if (buf && buf->valid)
                                    activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                                emitUiPulse(ev);
                            }
                        } else {
                            if (ev.samplePosInBar >= offsetSamples && ev.playPulse) {
                                PCMBuffer* buf = currentSample(ev.accent);
                                if (buf && buf->valid)
                                    activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                            }
                            emitUiPulse(ev);
                        }
                    } else {
                        if (ev.idx >= 0 && ev.idx != -9999 && ev.playPulse) {
                            PCMBuffer* buf = currentSample(ev.accent);
                            if (buf && buf->valid)
                                activeSamples.push_back({&buf->data, buf->startSample, outPos, m_volume});
                        }
                        emitUiPulse(ev);
                    }
                }
            }
        }
    }

    for (auto it = activeSamples.begin(); it != activeSamples.end(); ) {
        int samplesLeft = int(it->data->size()) - it->pos;
        int bufferSpace = int(nBufferFrames) - it->outPos;
        int samplesToMix = std::min(samplesLeft, bufferSpace);

        for (int k = 0; k < samplesToMix; ++k)
            output[it->outPos + k] += (*it->data)[it->pos + k] * it->volume;

        it->pos += samplesToMix;
        it->outPos += samplesToMix;

        if (it->pos >= int(it->data->size())) {
            it = activeSamples.erase(it);
        } else if (it->outPos >= int(nBufferFrames)) {
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
    // (if m_device.sampleRate != m_sampleRate) was always false — samples loaded at the
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