#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <string>

namespace
{

void setParamPlain(PrismorAudioProcessor& proc, const juce::String& paramId, float plainValue)
{
    auto* param = proc.getAPVTS().getParameter(paramId);
    if (param == nullptr)
    {
        std::cerr << "FAIL: missing parameter ID: " << paramId << "\n";
        std::abort();
    }
    param->setValueNotifyingHost(param->convertTo0to1(plainValue));
}

float getParamPlain(PrismorAudioProcessor& proc, const juce::String& paramId)
{
    auto* param = proc.getAPVTS().getParameter(paramId);
    if (param == nullptr)
    {
        std::cerr << "FAIL: missing parameter ID: " << paramId << "\n";
        std::abort();
    }
    return param->convertFrom0to1(param->getValue());
}

void disableAllFxAndResetClean(PrismorAudioProcessor& proc, int waveTypeA = 0, double sr = 48000.0, int blockSize = 512)
{
    proc.prepareToPlay(sr, blockSize);
    proc.loadFactoryPreset(0);

    setParamPlain(proc, "filter1Enable", 0.0f);
    setParamPlain(proc, "filter2Enable", 0.0f);
    setParamPlain(proc, "filterDrive",   0.0f);
    setParamPlain(proc, "noiseEnable",   0.0f);
    setParamPlain(proc, "arpEnable",     0.0f);
    setParamPlain(proc, "prismEnable",   0.0f);
    setParamPlain(proc, "prismAmount",   0.0f);
    setParamPlain(proc, "pluckEnable",   0.0f);

    setParamPlain(proc, "blurEnable",    0.0f);
    setParamPlain(proc, "tremEnable",    0.0f);
    setParamPlain(proc, "distEnable",    0.0f);
    setParamPlain(proc, "phaserEnable",  0.0f);
    setParamPlain(proc, "chorusEnable",  0.0f);
    setParamPlain(proc, "eqEnable",      0.0f);
    setParamPlain(proc, "delayEnable",   0.0f);
    setParamPlain(proc, "reverbEnable",  0.0f);
    setParamPlain(proc, "compEnable",    0.0f);
    setParamPlain(proc, "ringEnable",    0.0f);
    setParamPlain(proc, "crushEnable",   0.0f);
    setParamPlain(proc, "flangerEnable", 0.0f);
    setParamPlain(proc, "bodeEnable",    0.0f);
    setParamPlain(proc, "timeModEnable", 0.0f);

    setParamPlain(proc, "oscAOct",       0.0f);
    setParamPlain(proc, "oscASemi",      0.0f);
    setParamPlain(proc, "oscAFine",      0.0f);
    setParamPlain(proc, "oscACoarse",    0.0f);
    setParamPlain(proc, "startPhase",    0.0f);
    setParamPlain(proc, "phaseRandom",   0.0f);
    setParamPlain(proc, "timbre",        0.0f);
    setParamPlain(proc, "warpMode",      0.0f);
    setParamPlain(proc, "warpAmount",    0.0f);
    setParamPlain(proc, "harmonicScale", 1.0f);
    setParamPlain(proc, "harmonicTilt",  0.0f);

    setParamPlain(proc, "abMix",         0.0f);
    setParamPlain(proc, "velLink",       0.0f);
    setParamPlain(proc, "partPan",       0.0f);
    setParamPlain(proc, "unisonVoices",  1.0f);
    setParamPlain(proc, "unisonDetune",  0.0f);
    setParamPlain(proc, "unisonStackMode", 0.0f);
    setParamPlain(proc, "vibDepth",      0.0f);
    setParamPlain(proc, "envAttack",     0.0f);
    setParamPlain(proc, "envHold",       0.0f);
    setParamPlain(proc, "envDecay",      0.0f);
    setParamPlain(proc, "envSustain",    1.0f);
    setParamPlain(proc, "envRelease",    0.05f);
    setParamPlain(proc, "limiterEnable", 0.0f);
    setParamPlain(proc, "masterGainDb",  0.0f);

    proc.getSynthEngine().resetFxChainOrder();
    proc.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(0, waveTypeA);
    proc.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(1, waveTypeA);
    proc.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Volume).clearToFlat(PrismorDSP::ArticulatorTarget::Volume);
    proc.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Pitch).clearToFlat(PrismorDSP::ArticulatorTarget::Pitch);
    proc.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Filter1Cutoff).clearToFlat(PrismorDSP::ArticulatorTarget::Filter1Cutoff);
    proc.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Timbre).clearToFlat(PrismorDSP::ArticulatorTarget::Timbre);
    proc.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Prism).clearToFlat(PrismorDSP::ArticulatorTarget::Prism);
    proc.getSynthEngine().resetAllVoices();
}

struct AudioStats
{
    bool allFinite = true;
    float peak = 0.0f;
    float rmsL = 0.0f;
    float rmsR = 0.0f;
    double meanL = 0.0;
    double meanR = 0.0;
};

AudioStats renderAudio(PrismorAudioProcessor& proc,
                       juce::AudioBuffer<float>& outBuffer,
                       const juce::MidiBuffer& scheduledMidi,
                       int totalSamples,
                       int blockSize,
                       int numChannels = 2)
{
    outBuffer.setSize(numChannels, totalSamples, false, true, true);
    outBuffer.clear();

    juce::AudioBuffer<float> block(numChannels, blockSize);
    juce::MidiBuffer blockMidi;
    AudioStats stats;
    double sumSqL = 0.0, sumSqR = 0.0;

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const int n = std::min(blockSize, totalSamples - offset);
        block.setSize(numChannels, n, false, false, true);
        block.clear();

        blockMidi.clear();
        for (const auto meta : scheduledMidi)
        {
            if (meta.samplePosition >= offset && meta.samplePosition < offset + n)
                blockMidi.addEvent(meta.getMessage(), meta.samplePosition - offset);
        }

        proc.processBlock(block, blockMidi);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* r = block.getReadPointer(ch);
            float* w = outBuffer.getWritePointer(ch);
            for (int i = 0; i < n; ++i)
            {
                const float s = r[i];
                if (!std::isfinite(s))
                    stats.allFinite = false;
                else
                    stats.peak = std::max(stats.peak, std::abs(s));

                w[offset + i] = s;
                if (ch == 0)
                {
                    sumSqL += static_cast<double>(s) * s;
                    stats.meanL += s;
                }
                else if (ch == 1)
                {
                    sumSqR += static_cast<double>(s) * s;
                    stats.meanR += s;
                }
            }
        }
    }

    if (totalSamples > 0)
    {
        stats.rmsL = static_cast<float>(std::sqrt(sumSqL / totalSamples));
        stats.rmsR = (numChannels > 1) ? static_cast<float>(std::sqrt(sumSqR / totalSamples)) : stats.rmsL;
        stats.meanL /= static_cast<double>(totalSamples);
        stats.meanR = (numChannels > 1) ? (stats.meanR / static_cast<double>(totalSamples)) : stats.meanL;
    }
    return stats;
}

float peakMagnitudeNearDb(const juce::AudioBuffer<float>& buf, int startSample, double sampleRate, double targetHz, double bandwidthHz = 18.0)
{
    constexpr int fftOrder = 14;
    constexpr int fftSize = 1 << fftOrder; // 16384 (~2.93 Hz/bin at 48 kHz)
    if (startSample + fftSize > buf.getNumSamples())
        return -200.0f;

    juce::dsp::FFT fft(fftOrder);
    std::vector<float> fftData(fftSize * 2, 0.0f);
    const float* ptr = buf.getReadPointer(0);

    double winSum = 0.0;
    for (int i = 0; i < fftSize; ++i)
    {
        const float w = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * static_cast<float>(i) / static_cast<float>(fftSize - 1)));
        fftData[static_cast<size_t>(i)] = ptr[startSample + i] * w;
        winSum += w;
    }

    fft.performFrequencyOnlyForwardTransform(fftData.data());

    const int minBin = std::max(1, static_cast<int>(std::floor((targetHz - bandwidthHz) * fftSize / sampleRate)));
    const int maxBin = std::min(fftSize / 2 - 1, static_cast<int>(std::ceil((targetHz + bandwidthHz) * fftSize / sampleRate)));

    float maxMag = 0.0f;
    for (int b = minBin; b <= maxBin; ++b)
        maxMag = std::max(maxMag, fftData[static_cast<size_t>(b)] / static_cast<float>(winSum * 0.5));

    return 20.0f * std::log10(std::max(maxMag, 1e-10f));
}

bool testSilenceInSilenceOut()
{
    std::cout << "[1/11] Silence-in / silence-out (all FX enabled)... ";
    PrismorAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    proc.loadFactoryPreset(0);

    setParamPlain(proc, "noiseEnable", 0.0f);
    setParamPlain(proc, "filter1Enable", 1.0f);
    setParamPlain(proc, "filter1Res", 0.85f);
    setParamPlain(proc, "filter2Enable", 1.0f);
    setParamPlain(proc, "blurEnable", 1.0f);
    setParamPlain(proc, "blurTime", 0.5f);
    setParamPlain(proc, "tremEnable", 1.0f);
    setParamPlain(proc, "tremDepth", 0.5f);
    setParamPlain(proc, "distEnable", 1.0f);
    setParamPlain(proc, "distDrive", 0.8f);
    setParamPlain(proc, "phaserEnable", 1.0f);
    setParamPlain(proc, "phaserMix", 0.5f);
    setParamPlain(proc, "chorusEnable", 1.0f);
    setParamPlain(proc, "eqEnable", 1.0f);
    setParamPlain(proc, "delayEnable", 1.0f);
    setParamPlain(proc, "reverbEnable", 1.0f);
    setParamPlain(proc, "reverbShimmer", 0.8f);
    setParamPlain(proc, "compEnable", 1.0f);
    setParamPlain(proc, "ringEnable", 1.0f);
    setParamPlain(proc, "crushEnable", 1.0f);
    setParamPlain(proc, "flangerEnable", 1.0f);
    setParamPlain(proc, "bodeEnable", 1.0f);
    setParamPlain(proc, "bodeShift", 250.0f);
    setParamPlain(proc, "bodeFeedback", 0.5f);
    setParamPlain(proc, "timeModEnable", 1.0f);

    juce::AudioBuffer<float> buf;
    juce::MidiBuffer midi;
    auto stats = renderAudio(proc, buf, midi, 48000, 512, 2);

    if (!stats.allFinite || stats.peak > 1e-6f)
    {
        std::cerr << "FAIL (finite=" << stats.allFinite << ", peak=" << stats.peak << ")\n";
        return false;
    }
    std::cout << "PASS (peak=" << stats.peak << ")\n";
    return true;
}

bool testVariableBuffersSampleRatesBuses()
{
    std::cout << "[2/11] Variable buffers, sample rates & mono/stereo buses... ";
    const int blockSizes[] = { 1, 3, 7, 17, 64, 129, 513, 1024, 2048 };
    const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };

    for (int chCount : { 2, 1 })
    {
        for (double sr : sampleRates)
        {
            for (int bs : blockSizes)
            {
                PrismorAudioProcessor proc;
                if (chCount == 1)
                {
                    if (!proc.setBusesLayout({ {}, { juce::AudioChannelSet::mono() } }))
                    {
                        std::cerr << "FAIL: setBusesLayout mono failed\n";
                        return false;
                    }
                }
                proc.prepareToPlay(sr, bs);
                proc.loadFactoryPreset(0);

                juce::MidiBuffer midi;
                midi.addEvent(juce::MidiMessage::noteOn(1, 48, 0.8f), 0);
                midi.addEvent(juce::MidiMessage::noteOn(1, 52, 0.8f), 0);
                midi.addEvent(juce::MidiMessage::noteOn(1, 55, 0.8f), 0);

                const int total = static_cast<int>(sr * 0.08);
                juce::AudioBuffer<float> buf;
                auto stats = renderAudio(proc, buf, midi, total, bs, chCount);
                if (!stats.allFinite || stats.rmsL < 0.005f || stats.peak > 1.05f)
                {
                    std::cerr << "FAIL (ch=" << chCount << ", sr=" << sr << ", bs=" << bs
                              << ", finite=" << stats.allFinite << ", rmsL=" << stats.rmsL
                              << ", peak=" << stats.peak << ")\n";
                    return false;
                }
            }
        }
    }
    std::cout << "PASS\n";
    return true;
}

bool testFullStateRoundTrip()
{
    std::cout << "[3/11] Full 171-param APVTS + engine state round-trip... ";
    PrismorAudioProcessor src;
    src.prepareToPlay(48000.0, 512);

    const auto& allIds = PrismorAudioProcessor::getAllParameterIds();
    if (allIds.size() != 171)
    {
        std::cerr << "FAIL: expected 171 param IDs, got " << allIds.size() << "\n";
        return false;
    }

    std::vector<float> expectedPlain(static_cast<size_t>(allIds.size()), 0.0f);
    for (int i = 0; i < allIds.size(); ++i)
    {
        auto* p = src.getAPVTS().getParameter(allIds[i]);
        if (p == nullptr)
        {
            std::cerr << "FAIL: parameter not in APVTS: " << allIds[i] << "\n";
            return false;
        }
        float normTarget = 0.0f;
        if (dynamic_cast<juce::AudioParameterBool*>(p) != nullptr)
            normTarget = (p->getDefaultValue() < 0.5f) ? 1.0f : 0.0f;
        else if (dynamic_cast<juce::AudioParameterInt*>(p) != nullptr || dynamic_cast<juce::AudioParameterChoice*>(p) != nullptr)
            normTarget = (i % 2 == 0) ? 0.75f : 0.25f;
        else
            normTarget = 0.15f + 0.004f * static_cast<float>(i % 150);

        p->setValueNotifyingHost(normTarget);
        expectedPlain[static_cast<size_t>(i)] = p->convertFrom0to1(p->getValue());
    }

    src.getSynthEngine().setActiveFxChain({ 12, 7, 2, 5 });
    src.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(0, 2); // Triangle
    src.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(1, 3); // Organ
    src.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Filter1Cutoff).applyPresetShape(2);
    const auto expectedArtNodes = src.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Filter1Cutoff).getNodesCopy();
    PrismorDSP::ArpStep customStep;
    customStep.enabled = false;
    customStep.velocity = 0.42f;
    customStep.octaveOffset = 2;
    customStep.semitoneTranspose = -5;
    src.getSynthEngine().setArpStep(3, customStep);

    juce::MemoryBlock stateBlock;
    src.getStateInformation(stateBlock);

    PrismorAudioProcessor dst;
    dst.prepareToPlay(48000.0, 512);
    dst.setStateInformation(stateBlock.getData(), static_cast<int>(stateBlock.getSize()));

    for (int i = 0; i < allIds.size(); ++i)
    {
        const float actual = getParamPlain(dst, allIds[i]);
        if (std::abs(actual - expectedPlain[static_cast<size_t>(i)]) > 1e-3f)
        {
            std::cerr << "FAIL: param mismatch for " << allIds[i]
                      << " expected=" << expectedPlain[static_cast<size_t>(i)] << " actual=" << actual << "\n";
            return false;
        }
    }

    const auto dstChain = dst.getSynthEngine().getActiveFxChain();
    if (dstChain != std::vector<int>({ 12, 7, 2, 5 }))
    {
        std::cerr << "FAIL: FX chain mismatch\n";
        return false;
    }
    if (dst.getSynthEngine().getResynthesisEngine().getActiveWaveType(0) != 2 ||
        dst.getSynthEngine().getResynthesisEngine().getActiveWaveType(1) != 3)
    {
        std::cerr << "FAIL: waveType mismatch\n";
        return false;
    }
    const auto actualArtNodes = dst.getSynthEngine().getArticulator(PrismorDSP::ArticulatorTarget::Filter1Cutoff).getNodesCopy();
    if (actualArtNodes.size() != expectedArtNodes.size())
    {
        std::cerr << "FAIL: articulator nodes count mismatch\n";
        return false;
    }
    const auto actualStep = dst.getSynthEngine().getArpStep(3);
    if (actualStep.enabled != customStep.enabled || actualStep.octaveOffset != customStep.octaveOffset ||
        actualStep.semitoneTranspose != customStep.semitoneTranspose || std::abs(actualStep.velocity - customStep.velocity) > 1e-3f)
    {
        std::cerr << "FAIL: arp step mismatch\n";
        return false;
    }

    std::cout << "PASS\n";
    return true;
}

bool testAhdsrTiming()
{
    std::cout << "[4/11] AHDSR envelope timing... ";
    PrismorAudioProcessor proc;
    disableAllFxAndResetClean(proc, 0, 48000.0, 256);

    // attackSec = 0.001 + 0.3^2 * 2.5 = 0.226 s
    // holdSec   = 0.3^2 * 1.5 = 0.135 s (ends at ~0.361 s)
    // decaySec  = 0.010 + 0.3^2 * 3.5 = 0.325 s
    // sustain   = 0.35
    // releaseSec = 0.012 + 0.2^1.8 * 2.8 = ~0.167 s
    setParamPlain(proc, "envAttack",  0.30f);
    setParamPlain(proc, "envHold",    0.30f);
    setParamPlain(proc, "envDecay",   0.30f);
    setParamPlain(proc, "envSustain", 0.35f);
    setParamPlain(proc, "envRelease", 0.20f);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 48000); // Release at 1.0 s

    juce::AudioBuffer<float> buf;
    auto stats = renderAudio(proc, buf, midi, static_cast<int>(48000 * 1.4), 256, 2);
    if (!stats.allFinite)
    {
        std::cerr << "FAIL: NaN/Inf\n";
        return false;
    }

    auto windowPeak = [&](double startSec, double durSec) {
        const int s0 = static_cast<int>(startSec * 48000.0);
        const int s1 = std::min(buf.getNumSamples(), s0 + static_cast<int>(durSec * 48000.0));
        float p = 0.0f;
        const float* ptr = buf.getReadPointer(0);
        for (int i = s0; i < s1; ++i)
            p = std::max(p, std::abs(ptr[i]));
        return p;
    };

    const float pEarlyAtk = windowPeak(0.02, 0.03);
    const float pHold     = windowPeak(0.24, 0.08);
    const float pSustain  = windowPeak(0.85, 0.10);
    const float pReleased = windowPeak(1.30, 0.08);

    if (!(pEarlyAtk < pHold * 0.6f && pSustain < pHold * 0.55f && pSustain > pHold * 0.15f && pReleased < pSustain * 0.1f))
    {
        std::cerr << "FAIL (earlyAtk=" << pEarlyAtk << ", hold=" << pHold
                  << ", sustain=" << pSustain << ", released=" << pReleased << ")\n";
        return false;
    }
    std::cout << "PASS (hold=" << pHold << ", sust=" << pSustain << ", rel=" << pReleased << ")\n";
    return true;
}

bool testPhaseControls()
{
    std::cout << "[5/11] startPhase / oscBPhase / phaseRandom... ";
    const int nSamp = 512;

    auto renderSinePhase = [&](bool usePartB, float phaseVal, float randVal) {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 6, 48000.0, 256); // 6 = Sine
        setParamPlain(proc, "abMix", usePartB ? 1.0f : 0.0f);
        setParamPlain(proc, usePartB ? "oscBPhase" : "startPhase", phaseVal);
        setParamPlain(proc, "phaseRandom", randVal);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0); // 220 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, nSamp, 256, 2);
        return buf;
    };

    auto corr = [&](const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b) {
        double dot = 0.0, na = 0.0, nb = 0.0;
        for (int i = 48; i < nSamp; ++i) // skip initial 1ms attack ramp near 0
        {
            const double x = a.getSample(0, i);
            const double y = b.getSample(0, i);
            dot += x * y;
            na += x * x;
            nb += y * y;
        }
        return dot / (std::sqrt(na * nb) + 1e-12);
    };

    auto bufA0 = renderSinePhase(false, 0.0f, 0.0f);
    auto bufA5 = renderSinePhase(false, 0.5f, 0.0f);
    const double cA = corr(bufA0, bufA5);
    if (cA > -0.8)
    {
        std::cerr << "FAIL: startPhase 0 vs 0.5 corr=" << cA << "\n";
        return false;
    }

    auto bufB0 = renderSinePhase(true, 0.0f, 0.0f);
    auto bufB5 = renderSinePhase(true, 0.5f, 0.0f);
    const double cB = corr(bufB0, bufB5);
    if (cB > -0.8)
    {
        std::cerr << "FAIL: oscBPhase 0 vs 0.5 corr=" << cB << "\n";
        return false;
    }

    // phaseRandom = 1.0 across two sequential notes in the same engine
    PrismorAudioProcessor procR;
    disableAllFxAndResetClean(procR, 6, 48000.0, 256);
    setParamPlain(procR, "phaseRandom", 1.0f);
    setParamPlain(procR, "envRelease", 0.01f);
    juce::AudioBuffer<float> n1, n2;
    {
        juce::MidiBuffer m1;
        m1.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
        renderAudio(procR, n1, m1, nSamp, 256, 2);
    }
    procR.getSynthEngine().resetAllVoices();
    {
        juce::MidiBuffer m2;
        m2.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
        renderAudio(procR, n2, m2, nSamp, 256, 2);
    }
    double maxDiff = 0.0;
    for (int i = 48; i < nSamp; ++i)
        maxDiff = std::max(maxDiff, static_cast<double>(std::abs(n1.getSample(0, i) - n2.getSample(0, i))));
    if (maxDiff < 0.01)
    {
        std::cerr << "FAIL: phaseRandom produced identical waveform diff=" << maxDiff << "\n";
        return false;
    }

    std::cout << "PASS (corrA=" << cA << ", corrB=" << cB << ")\n";
    return true;
}

bool testUnisonTriadModes()
{
    std::cout << "[6/11] Unison Stack Modes 6 & 7 (Minor / Major Triad)... ";

    auto measureMode = [&](int mode, double& pRoot, double& pMin3, double& pMaj3, double& pFifth) {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 6, 48000.0, 512); // Sine
        setParamPlain(proc, "unisonVoices", 6.0f);
        setParamPlain(proc, "unisonDetune", 0.0f);
        setParamPlain(proc, "unisonSpread", 0.0f); // center pan so L channel captures all
        setParamPlain(proc, "unisonBlend",  1.0f);
        setParamPlain(proc, "unisonStackMode", static_cast<float>(mode));

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0); // A3 = 220 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, 24576, 512, 2);

        pRoot  = peakMagnitudeNearDb(buf, 8192, 48000.0, 220.0, 6.0);
        pMin3  = peakMagnitudeNearDb(buf, 8192, 48000.0, 220.0 * std::pow(2.0, 3.0 / 12.0), 4.5);
        pMaj3  = peakMagnitudeNearDb(buf, 8192, 48000.0, 220.0 * std::pow(2.0, 4.0 / 12.0), 4.5);
        pFifth = peakMagnitudeNearDb(buf, 8192, 48000.0, 220.0 * std::pow(2.0, 7.0 / 12.0), 6.0);
    };

    double r6, min6, maj6, fif6;
    measureMode(6, r6, min6, maj6, fif6);
    if (!(min6 > maj6 + 15.0 && fif6 > maj6 + 15.0 && r6 > maj6 + 15.0))
    {
        std::cerr << "FAIL Minor Triad (root=" << r6 << ", min3=" << min6 << ", maj3=" << maj6 << ", fifth=" << fif6 << ")\n";
        return false;
    }

    double r7, min7, maj7, fif7;
    measureMode(7, r7, min7, maj7, fif7);
    if (!(maj7 > min7 + 15.0 && fif7 > min7 + 15.0 && r7 > min7 + 15.0))
    {
        std::cerr << "FAIL Major Triad (root=" << r7 << ", min3=" << min7 << ", maj3=" << maj7 << ", fifth=" << fif7 << ")\n";
        return false;
    }

    std::cout << "PASS (min3=" << min6 << "dB, maj3=" << maj7 << "dB)\n";
    return true;
}

bool testPrismAndHarmonicScale()
{
    std::cout << "[7/11] Prism Hz Shift & harmonicScale inharmonicity... ";
    // Case A: Prism Mode 1 (Hz Shift), prismAmount = 0.5 -> +130 Hz on k >= 1 (440 -> 570 Hz)
    {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 0, 48000.0, 512); // Saw
        setParamPlain(proc, "prismEnable", 1.0f);
        setParamPlain(proc, "prismMode",   1.0f); // Hz Shift
        setParamPlain(proc, "prismAmount", 0.5f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0); // 220 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, 24576, 512, 2);

        const float p440 = peakMagnitudeNearDb(buf, 8192, 48000.0, 440.0, 14.0);
        const float p570 = peakMagnitudeNearDb(buf, 8192, 48000.0, 570.0, 14.0);
        if (p570 <= p440 + 12.0f)
        {
            std::cerr << "FAIL Prism Hz Shift (440Hz=" << p440 << "dB, 570Hz=" << p570 << "dB)\n";
            return false;
        }
    }

    // Case B: harmonicScale = 1.5 -> 2nd partial at 220 * (1 + 1.5) = 550 Hz
    {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 0, 48000.0, 512); // Saw
        setParamPlain(proc, "harmonicScale", 1.5f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0); // 220 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, 24576, 512, 2);

        const float p440 = peakMagnitudeNearDb(buf, 8192, 48000.0, 440.0, 14.0);
        const float p550 = peakMagnitudeNearDb(buf, 8192, 48000.0, 550.0, 14.0);
        if (p550 <= p440 + 12.0f)
        {
            std::cerr << "FAIL harmonicScale (440Hz=" << p440 << "dB, 550Hz=" << p550 << "dB)\n";
            return false;
        }
    }

    std::cout << "PASS\n";
    return true;
}

bool testDynamicFxReorderAndEq()
{
    std::cout << "[8/11] Dynamic FX chain reorder, ParametricEQ & slot removal... ";
    PrismorAudioProcessor proc;
    disableAllFxAndResetClean(proc, 6, 48000.0, 512); // Sine
    proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::ParametricEQ) });
    setParamPlain(proc, "eqEnable", 1.0f);
    setParamPlain(proc, "eqMidFreq", 440.0f);
    setParamPlain(proc, "eqMidGain", 0.0f);
    setParamPlain(proc, "eqMidQ",    2.0f);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 69, 1.0f), 0); // 440 Hz

    juce::AudioBuffer<float> bufFlat;
    auto stFlat = renderAudio(proc, bufFlat, midi, 16384, 512, 2);

    proc.getSynthEngine().resetAllVoices();
    setParamPlain(proc, "eqMidGain", 12.0f);
    juce::AudioBuffer<float> bufBoost;
    auto stBoost = renderAudio(proc, bufBoost, midi, 16384, 512, 2);

    const float boostDb = 20.0f * std::log10((stBoost.rmsL + 1e-9f) / (stFlat.rmsL + 1e-9f));
    if (boostDb < 7.0f)
    {
        std::cerr << "FAIL ParametricEQ +12dB gain increase was only " << boostDb << " dB\n";
        return false;
    }

    // Remove EQ slot -> +12 dB setting must now have zero effect
    proc.getSynthEngine().removeFxSlot(0);
    proc.getSynthEngine().resetAllVoices();
    juce::AudioBuffer<float> bufRemoved;
    auto stRemoved = renderAudio(proc, bufRemoved, midi, 16384, 512, 2);
    if (std::abs(stRemoved.rmsL - stFlat.rmsL) > 0.01f)
    {
        std::cerr << "FAIL removeFxSlot did not bypass EQ (flat=" << stFlat.rmsL << ", removed=" << stRemoved.rmsL << ")\n";
        return false;
    }

    // Reorder test: {Distortion, EQ} vs {EQ, Distortion} with heavy low-mid boost + hard clip
    auto renderOrder = [&](std::vector<int> chain) {
        PrismorAudioProcessor p;
        disableAllFxAndResetClean(p, 0, 48000.0, 512);
        p.getSynthEngine().setActiveFxChain(chain);
        setParamPlain(p, "distEnable", 1.0f);
        setParamPlain(p, "distType",   2.0f); // Hard Clip
        setParamPlain(p, "distDrive",  0.85f);
        setParamPlain(p, "distMix",    1.0f);
        setParamPlain(p, "eqEnable",   1.0f);
        setParamPlain(p, "eqMidFreq",  1200.0f);
        setParamPlain(p, "eqMidGain",  14.0f);
        juce::AudioBuffer<float> b;
        juce::MidiBuffer m;
        m.addEvent(juce::MidiMessage::noteOn(1, 57, 1.0f), 0);
        renderAudio(p, b, m, 8192, 512, 2);
        return b;
    };

    auto bDistEq = renderOrder({ static_cast<int>(PrismorDSP::FxModuleId::Distortion),
                               static_cast<int>(PrismorDSP::FxModuleId::ParametricEQ) });
    auto bEqDist = renderOrder({ static_cast<int>(PrismorDSP::FxModuleId::ParametricEQ),
                               static_cast<int>(PrismorDSP::FxModuleId::Distortion) });
    double diffSq = 0.0;
    for (int i = 2048; i < 8192; ++i)
    {
        const double d = bDistEq.getSample(0, i) - bEqDist.getSample(0, i);
        diffSq += d * d;
    }
    const double diffRms = std::sqrt(diffSq / (8192 - 2048));
    if (diffRms < 0.01)
    {
        std::cerr << "FAIL FX reorder produced nearly identical signal (diffRms=" << diffRms << ")\n";
        return false;
    }

    std::cout << "PASS (eqBoost=" << boostDb << "dB, orderDiffRms=" << diffRms << ")\n";
    return true;
}

bool testBodeFrequencyShifter()
{
    std::cout << "[9/11] Bode Frequency Shifter SSB suppression & DC safety... ";
    // Case A: +250 Hz shift on 880 Hz (A5) -> 1130 Hz wanted, 630 Hz & 880 Hz suppressed > 45 dB
    {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 6, 48000.0, 512); // Sine
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Bode) });
        setParamPlain(proc, "bodeEnable",   1.0f);
        setParamPlain(proc, "bodeShift",    250.0f);
        setParamPlain(proc, "bodeFeedback", 0.0f);
        setParamPlain(proc, "bodeMix",      1.0f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 81, 1.0f), 0); // A5 = 880 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, 28672, 512, 2);

        const float wanted   = peakMagnitudeNearDb(buf, 8192, 48000.0, 1130.0, 18.0);
        const float image    = peakMagnitudeNearDb(buf, 8192, 48000.0, 630.0,  18.0);
        const float carrier  = peakMagnitudeNearDb(buf, 8192, 48000.0, 880.0,  18.0);
        if ((wanted - image) < 45.0f || (wanted - carrier) < 45.0f)
        {
            std::cerr << "FAIL +250Hz SSB (wanted=" << wanted << "dB, img=" << image << "dB, car=" << carrier << "dB)\n";
            return false;
        }
    }

    // Case B: -300 Hz shift on 880 Hz -> 580 Hz wanted, 1180 Hz suppressed > 45 dB
    {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 6, 48000.0, 512);
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Bode) });
        setParamPlain(proc, "bodeEnable",   1.0f);
        setParamPlain(proc, "bodeShift",   -300.0f);
        setParamPlain(proc, "bodeFeedback", 0.0f);
        setParamPlain(proc, "bodeMix",      1.0f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 81, 1.0f), 0); // 880 Hz
        juce::AudioBuffer<float> buf;
        renderAudio(proc, buf, midi, 28672, 512, 2);

        const float wanted   = peakMagnitudeNearDb(buf, 8192, 48000.0, 580.0,  18.0);
        const float image    = peakMagnitudeNearDb(buf, 8192, 48000.0, 1180.0, 18.0);
        if ((wanted - image) < 45.0f)
        {
            std::cerr << "FAIL -300Hz SSB (wanted=" << wanted << "dB, img=" << image << "dB)\n";
            return false;
        }
    }

    // Case C: High feedback + limiter safety
    {
        PrismorAudioProcessor proc;
        disableAllFxAndResetClean(proc, 0, 48000.0, 512);
        setParamPlain(proc, "limiterEnable", 1.0f);
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Bode) });
        setParamPlain(proc, "bodeEnable",   1.0f);
        setParamPlain(proc, "bodeShift",    250.0f);
        setParamPlain(proc, "bodeFeedback", 0.75f);
        setParamPlain(proc, "bodeMix",      0.85f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.0f), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 48000);
        juce::AudioBuffer<float> buf;
        auto stats = renderAudio(proc, buf, midi, 96000, 512, 2);
        if (!stats.allFinite || std::abs(stats.meanL) > 0.005 || std::abs(stats.meanR) > 0.005 || stats.peak > 1.05f)
        {
            std::cerr << "FAIL Bode feedback DC/safety (meanL=" << stats.meanL << ", peak=" << stats.peak << ")\n";
            return false;
        }
    }

    std::cout << "PASS\n";
    return true;
}

bool testReverbShimmerDcSafety()
{
    std::cout << "[10/11] Reverb Shimmer DC & stability safety... ";
    PrismorAudioProcessor proc;
    disableAllFxAndResetClean(proc, 0, 48000.0, 512);
    setParamPlain(proc, "limiterEnable", 1.0f);
    proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Reverb) });
    setParamPlain(proc, "reverbEnable",  1.0f);
    setParamPlain(proc, "reverbDecay",   0.90f);
    setParamPlain(proc, "reverbShimmer", 0.90f);
    setParamPlain(proc, "reverbMix",     0.70f);

    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, 0.9f), 0);
    midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.9f), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 48000);
    midi.addEvent(juce::MidiMessage::noteOff(1, 64, 0.0f), 48000);
    midi.addEvent(juce::MidiMessage::noteOff(1, 67, 0.0f), 48000);

    juce::AudioBuffer<float> buf;
    auto stats = renderAudio(proc, buf, midi, static_cast<int>(48000 * 2.5), 512, 2);
    if (!stats.allFinite || std::abs(stats.meanL) > 0.005 || std::abs(stats.meanR) > 0.005 || stats.peak > 1.05f || stats.rmsL < 0.001f)
    {
        std::cerr << "FAIL (finite=" << stats.allFinite << ", meanL=" << stats.meanL
                  << ", meanR=" << stats.meanR << ", peak=" << stats.peak << ", rmsL=" << stats.rmsL << ")\n";
        return false;
    }

    std::cout << "PASS (meanL=" << stats.meanL << ", peak=" << stats.peak << ")\n";
    return true;
}

bool testAllFactoryPresetsAndPolyStress()
{
    std::cout << "[11/11] All 332 factory presets & Preset 39 polyphonic stress... ";
    PrismorAudioProcessor proc;
    proc.prepareToPlay(48000.0, 512);
    const int numPresets = proc.getNumPrograms();
    if (numPresets != 332)
    {
        std::cerr << "FAIL: expected 332 presets, got " << numPresets << "\n";
        return false;
    }

    juce::MidiBuffer noteMidi;
    noteMidi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.85f), 0);
    noteMidi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 7200);

    juce::AudioBuffer<float> buf;
    for (int i = 0; i < numPresets; ++i)
    {
        proc.loadFactoryPreset(i);
        proc.getSynthEngine().resetAllVoices();
        auto stats = renderAudio(proc, buf, noteMidi, 12000, 512, 2);
        if (!stats.allFinite || stats.peak > 1.05f)
        {
            std::cerr << "FAIL on factory preset " << i << " (" << proc.getProgramName(i)
                      << "): finite=" << stats.allFinite << ", peak=" << stats.peak << "\n";
            return false;
        }
    }

    // Preset 39 8-note overlapping polyphonic stress
    proc.loadFactoryPreset(39);
    proc.getSynthEngine().resetAllVoices();
    juce::MidiBuffer polyMidi;
    const int notes[] = { 48, 50, 52, 53, 55, 57, 59, 60 };
    for (int idx = 0; idx < 8; ++idx)
    {
        polyMidi.addEvent(juce::MidiMessage::noteOn(1, notes[idx], 0.9f), idx * 480);
        polyMidi.addEvent(juce::MidiMessage::noteOff(1, notes[idx], 0.0f), 72000);
    }
    auto p39Stats = renderAudio(proc, buf, polyMidi, 96000, 512, 2);
    if (!p39Stats.allFinite || p39Stats.peak > 1.05f || p39Stats.rmsL < 0.01f)
    {
        std::cerr << "FAIL Preset 39 poly stress (finite=" << p39Stats.allFinite
                  << ", peak=" << p39Stats.peak << ", rmsL=" << p39Stats.rmsL << ")\n";
        return false;
    }

    std::cout << "PASS (332 presets + Preset 39 peak=" << p39Stats.peak << ")\n";
    return true;
}

} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    std::cout << std::unitbuf;
    std::cout << "=== Prismor Headless DSP & State Test Suite ===\n";

    if (!testSilenceInSilenceOut())              return 1;
    if (!testVariableBuffersSampleRatesBuses())  return 2;
    if (!testFullStateRoundTrip())               return 3;
    if (!testAhdsrTiming())                      return 4;
    if (!testPhaseControls())                    return 5;
    if (!testUnisonTriadModes())                 return 6;
    if (!testPrismAndHarmonicScale())            return 7;
    if (!testDynamicFxReorderAndEq())            return 8;
    if (!testBodeFrequencyShifter())             return 9;
    if (!testReverbShimmerDcSafety())            return 10;
    if (!testAllFactoryPresetsAndPolyStress())   return 11;

    std::cout << "ALL 11 TESTS PASSED.\n";
    return 0;
}
