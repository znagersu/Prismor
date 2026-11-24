#pragma once
#include <JuceHeader.h>
#include "DspUtils.h"
#include "SplineCurve.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

namespace PrismorDSP
{

class FxRack
{
public:
    static constexpr int kMaxChunkSamples = 512;
    static constexpr int kMaxFxChainLen   = 16;

    FxRack()
    {
        initSineLUT();
        allocateBuffers(44100.0);
    }

    void prepare(double newSampleRate, int /*maxSamplesPerBlock*/)
    {
        sampleRate    = std::max(22050.0, newSampleRate);
        invSampleRate = 1.0f / static_cast<float>(sampleRate);

        allocateBuffers(sampleRate);

        driveOversampler.reset();
        driveOversampler.initProcessing(static_cast<size_t>(kMaxChunkSamples));

        distOversampler.reset();
        distOversampler.initProcessing(static_cast<size_t>(kMaxChunkSamples));

        osScratch.setSize(2, kMaxChunkSamples, false, true, true);

        const double rampFast = 0.015;
        const double rampTime = 0.045;
        smoothFilterDrive.reset(sampleRate, rampFast);
        smoothDistDrive.reset(sampleRate, rampFast);
        smoothDistMix.reset(sampleRate, rampFast);
        smoothTremDepth.reset(sampleRate, rampFast);
        smoothPhaserMix.reset(sampleRate, rampFast);
        smoothChorusMix.reset(sampleRate, rampFast);
        smoothDelayTime.reset(sampleRate, rampTime);
        smoothDelayMix.reset(sampleRate, rampFast);
        smoothReverbMix.reset(sampleRate, rampFast);
        smoothCompMix.reset(sampleRate, rampFast);
        smoothRingFreq.reset(sampleRate, rampFast);
        smoothRingMix.reset(sampleRate, rampFast);
        smoothCrushMix.reset(sampleRate, rampFast);
        smoothFlangerMix.reset(sampleRate, rampFast);
        smoothBodeShift.reset(sampleRate, rampFast);
        smoothBodeFeedback.reset(sampleRate, rampFast);
        smoothBodeMix.reset(sampleRate, rampFast);
        smoothTimeModMix.reset(sampleRate, rampFast);
        smoothMasterGainDb.reset(sampleRate, rampFast);

        driveDcL.prepare(sampleRate, 12.0f);
        driveDcR.prepare(sampleRate, 12.0f);
        distDcL.prepare(sampleRate, 20.0f);
        distDcR.prepare(sampleRate, 20.0f);
        revShimDcL.prepare(sampleRate, 15.0f);
        revShimDcR.prepare(sampleRate, 15.0f);
        bodeDcL.prepare(sampleRate, 12.0f);
        bodeDcR.prepare(sampleRate, 12.0f);

        reset();
    }

    void reset() noexcept
    {
        pinkB0 = pinkB1 = pinkB2 = 0.0f;
        brownStateL = brownStateR = 0.0f;
        bitHoldPhase = 0.0f;
        bitHoldSampleL = bitHoldSampleR = 0.0f;
        airBandL = airLowL = airBandR = airLowR = 0.0f;
        noiseLpStateL = noiseLpStateR = 0.0f;
        noiseHpStateL = noiseHpStateR = 0.0f;

        driveOversampler.reset();
        distOversampler.reset();
        driveDcL.reset(); driveDcR.reset();
        distDcL.reset();  distDcR.reset();
        revShimDcL.reset(); revShimDcR.reset();
        bodeDcL.reset();  bodeDcR.reset();

        phaserLfoPhase = 0.0f;
        tremMasterPhase = 0.0f;
        distToneStateL = distToneStateR = 0.0f;
        phaserApStateL.fill(0.0f);
        phaserApStateR.fill(0.0f);

        chorusPhase = 0.0f;
        chorusWritePos = 0;
        std::fill(chorusBufL.begin(), chorusBufL.end(), 0.0f);
        std::fill(chorusBufR.begin(), chorusBufR.end(), 0.0f);

        eqLowShelf.reset();
        eqMidPeak.reset();
        eqHighShelf.reset();
        lastEqLowFreq = -1.0f;

        delayWritePos = 0;
        delayLpStateL = delayLpStateR = 0.0f;
        std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);

        revPrePos = revPos0 = revPos1 = revPos2 = revPos3 = revApPos0 = revApPos1 = 0;
        revLpL0 = revLpR0 = revLpL1 = revLpR1 = 0.0f;
        revLpL2 = revLpR2 = revLpL3 = revLpR3 = 0.0f;
        std::fill(revPreBufL.begin(), revPreBufL.end(), 0.0f);
        std::fill(revPreBufR.begin(), revPreBufR.end(), 0.0f);
        std::fill(revCombBufL0.begin(), revCombBufL0.end(), 0.0f);
        std::fill(revCombBufR0.begin(), revCombBufR0.end(), 0.0f);
        std::fill(revCombBufL1.begin(), revCombBufL1.end(), 0.0f);
        std::fill(revCombBufR1.begin(), revCombBufR1.end(), 0.0f);
        std::fill(revCombBufL2.begin(), revCombBufL2.end(), 0.0f);
        std::fill(revCombBufR2.begin(), revCombBufR2.end(), 0.0f);
        std::fill(revCombBufL3.begin(), revCombBufL3.end(), 0.0f);
        std::fill(revCombBufR3.begin(), revCombBufR3.end(), 0.0f);
        std::fill(revApBufL0.begin(), revApBufL0.end(), 0.0f);
        std::fill(revApBufR0.begin(), revApBufR0.end(), 0.0f);
        std::fill(revApBufL1.begin(), revApBufL1.end(), 0.0f);
        std::fill(revApBufR1.begin(), revApBufR1.end(), 0.0f);

        compEnvSingle = 0.0001f;
        compEnvBand.fill(0.0001f);
        compLpLowL = compLpLowR = compLpMidL = compLpMidR = 0.0f;

        ringCarrierPhase = 0.0f;
        ringLfoPhase = 0.0f;
        ringHilbertL.reset();
        ringHilbertR.reset();

        crushHoldPhase = 0.0f;
        crushHeldL = crushHeldR = 0.0f;
        crushLpL = crushLpR = 0.0f;

        flangerPhase = 0.0f;
        flangerWritePos = 0;
        flangerFbL = flangerFbR = 0.0f;
        std::fill(flangerBufL.begin(), flangerBufL.end(), 0.0f);
        std::fill(flangerBufR.begin(), flangerBufR.end(), 0.0f);

        bodeCarrierPhase = 0.0f;
        bodeFbL = bodeFbR = 0.0f;
        bodeHilbertL.reset();
        bodeHilbertR.reset();

        timeModWritePos = 0;
        timeModBarPhase = 0.0f;
        timeModPrevOffsetSamples = 0.0f;
        timeModLpL = timeModLpR = timeModHpL = timeModHpR = 0.0f;
        std::fill(timeModBufL.begin(), timeModBufL.end(), 0.0f);
        std::fill(timeModBufR.begin(), timeModBufR.end(), 0.0f);

        limiterEnv = 1.0f;
    }

    void process(float* outL,
                 float* outR,
                 int numSamples,
                 const SynthParameters& params,
                 const std::array<int, kMaxFxChainLen>& chain,
                 int chainCount,
                 float voiceEnvGain,
                 float activePitchHz,
                 int activeVoiceCount,
                 SplineCurve& timeModCurve,
                 VisualizerSnapshot& visOut)
    {
        juce::ScopedNoDenormals noDenormals;

        // Update smoother targets cleanly once per block
        smoothFilterDrive.setTargetValue(juce::jlimit(0.0f, 1.0f, params.filterDrive));
        smoothDistDrive.setTargetValue(juce::jlimit(0.0f, 1.0f, params.distDrive));
        smoothDistMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.distMix));
        smoothTremDepth.setTargetValue(juce::jlimit(0.0f, 1.0f, params.tremDepth));
        smoothPhaserMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.phaserMix));
        smoothChorusMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.chorusMix));
        smoothDelayTime.setTargetValue(juce::jlimit(0.0f, 1.0f, params.delayTime));
        smoothDelayMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.delayMix));
        smoothReverbMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.reverbMix));
        smoothCompMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.compMix));
        smoothRingFreq.setTargetValue(juce::jlimit(20.0f, 2000.0f, params.ringFreq));
        smoothRingMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.ringMix));
        smoothCrushMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.crushMix));
        smoothFlangerMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.flangerMix));
        smoothBodeShift.setTargetValue(juce::jlimit(-5000.0f, 5000.0f, params.bodeShift));
        smoothBodeFeedback.setTargetValue(juce::jlimit(-0.88f, 0.88f, params.bodeFeedback));
        smoothBodeMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.bodeMix));
        smoothTimeModMix.setTargetValue(juce::jlimit(0.0f, 1.0f, params.timeModMix));
        smoothMasterGainDb.setTargetValue(juce::jlimit(-24.0f, 12.0f, params.masterGainDb));

        float maxPeakL = 0.0f, maxPeakR = 0.0f;
        double sumSqL = 0.0, sumSqR = 0.0;
        float maxLimGr = 0.0f;
        float maxCompGr = 0.0f;

        int offset = 0;
        while (offset < numSamples)
        {
            const int chunk = std::min(kMaxChunkSamples, numSamples - offset);
            float* cL = outL + offset;
            float* cR = outR + offset;

            float chunkCompGr = 0.0f;
            float chunkLimGr  = 0.0f;

            processChunk(cL, cR, chunk, params, chain, chainCount, voiceEnvGain,
                         activePitchHz, activeVoiceCount, timeModCurve, visOut,
                         chunkCompGr, chunkLimGr);

            maxCompGr = std::max(maxCompGr, chunkCompGr);
            maxLimGr  = std::max(maxLimGr, chunkLimGr);

            for (int i = 0; i < chunk; ++i)
            {
                const float l = cL[i];
                const float r = cR[i];
                maxPeakL = std::max(maxPeakL, std::abs(l));
                maxPeakR = std::max(maxPeakR, std::abs(r));
                sumSqL += static_cast<double>(l) * l;
                sumSqR += static_cast<double>(r) * r;
            }
            offset += chunk;
        }

        const float invN = 1.0f / static_cast<float>(std::max(1, numSamples));
        visOut.peakLeft  = juce::jlimit(0.0f, 1.2f, maxPeakL);
        visOut.peakRight = juce::jlimit(0.0f, 1.2f, maxPeakR);
        visOut.rmsLeft   = juce::jlimit(0.0f, 1.2f, static_cast<float>(std::sqrt(sumSqL * invN)));
        visOut.rmsRight  = juce::jlimit(0.0f, 1.2f, static_cast<float>(std::sqrt(sumSqR * invN)));
        visOut.compGainReductionDb    = juce::jlimit(0.0f, 24.0f, maxCompGr);
        visOut.limiterGainReductionDb = juce::jlimit(0.0f, 24.0f, maxLimGr);
    }

private:
    static constexpr int kSineLutSize = 8192;

    void initSineLUT()
    {
        for (int i = 0; i < kSineLutSize; ++i)
        {
            const double angle = (static_cast<double>(i) / static_cast<double>(kSineLutSize)) * juce::MathConstants<double>::twoPi;
            sineLUT[static_cast<size_t>(i)] = static_cast<float>(std::sin(angle));
        }
    }

    inline float fastSin(float rad) const noexcept
    {
        const uint32_t ph = static_cast<uint32_t>(static_cast<int64_t>(rad * 683565275.5764316));
        return sineLUT[ph >> 19];
    }

    inline float fastCos(float rad) const noexcept
    {
        const uint32_t ph = static_cast<uint32_t>(static_cast<int64_t>(rad * 683565275.5764316)) + 0x40000000u;
        return sineLUT[ph >> 19];
    }

    void allocateBuffers(double sr)
    {
        const int delayLen   = nextPowerOfTwo(std::max(131072, static_cast<int>(std::ceil(sr * 2.2))));
        const int chorusLen  = nextPowerOfTwo(std::max(4096,   static_cast<int>(std::ceil(sr * 0.06))));
        const int flangerLen = nextPowerOfTwo(std::max(2048,   static_cast<int>(std::ceil(sr * 0.03))));
        const int revPreLen  = nextPowerOfTwo(std::max(4096,   static_cast<int>(std::ceil(sr * 0.15))));
        const int tmLen      = nextPowerOfTwo(std::max(262144, static_cast<int>(std::ceil(sr * 5.5))));

        delayBufSize   = delayLen;
        chorusBufSize  = chorusLen;
        flangerBufSize = flangerLen;
        revPreBufSize  = revPreLen;
        timeModBufSize = tmLen;

        delayBufL.assign(static_cast<size_t>(delayBufSize), 0.0f);
        delayBufR.assign(static_cast<size_t>(delayBufSize), 0.0f);
        chorusBufL.assign(static_cast<size_t>(chorusBufSize), 0.0f);
        chorusBufR.assign(static_cast<size_t>(chorusBufSize), 0.0f);
        flangerBufL.assign(static_cast<size_t>(flangerBufSize), 0.0f);
        flangerBufR.assign(static_cast<size_t>(flangerBufSize), 0.0f);
        revPreBufL.assign(static_cast<size_t>(revPreBufSize), 0.0f);
        revPreBufR.assign(static_cast<size_t>(revPreBufSize), 0.0f);
        timeModBufL.assign(static_cast<size_t>(timeModBufSize), 0.0f);
        timeModBufR.assign(static_cast<size_t>(timeModBufSize), 0.0f);

        const double srScale = sr / 44100.0;
        const int maxCombCap = std::max(2048, static_cast<int>(std::ceil(1800.0 * srScale)) + 64);
        revCombBufL0.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufR0.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufL1.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufR1.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufL2.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufR2.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufL3.assign(static_cast<size_t>(maxCombCap), 0.0f);
        revCombBufR3.assign(static_cast<size_t>(maxCombCap), 0.0f);

        revApLen0 = std::max(64, static_cast<int>(std::round(347.0 * srScale)));
        revApLen1 = std::max(32, static_cast<int>(std::round(113.0 * srScale)));
        revApBufL0.assign(static_cast<size_t>(revApLen0 + 8), 0.0f);
        revApBufR0.assign(static_cast<size_t>(revApLen0 + 8), 0.0f);
        revApBufL1.assign(static_cast<size_t>(revApLen1 + 8), 0.0f);
        revApBufR1.assign(static_cast<size_t>(revApLen1 + 8), 0.0f);
    }

    void processChunk(float* outL,
                      float* outR,
                      int numSamples,
                      const SynthParameters& params,
                      const std::array<int, kMaxFxChainLen>& chain,
                      int chainCount,
                      float voiceEnvGain,
                      float activePitchHz,
                      int activeVoiceCount,
                      SplineCurve& timeModCurve,
                      VisualizerSnapshot& visOut,
                      float& outCompGr,
                      float& outLimGr)
    {
        // 1. Noise Generator
        if (params.noiseEnable && params.noiseLevel > 0.002f && activeVoiceCount > 0)
        {
            const float nGain = params.noiseLevel * voiceEnvGain * 0.45f;
            const float nPan  = juce::jlimit(-1.0f, 1.0f, params.noisePan);
            const float panL  = std::cos((nPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            const float panR  = std::sin((nPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
            const float stereoDecorrel = params.noisePhase;

            const float bitHoldRateHz = (250.0f + params.noisePitch * 14000.0f)
                                      * std::pow(std::max(40.0f, activePitchHz) / 261.63f, params.noiseKeyTrack);
            const float bitHoldStep = std::min(1.0f, bitHoldRateHz * invSampleRate);

            const float filtAmt = juce::jlimit(-1.0f, 1.0f, params.noiseFilter);
            const float lpCutoffHz = (filtAmt < 0.0f) ? (120.0f + std::pow(1.0f + filtAmt, 2.5f) * 16000.0f) : 18000.0f;
            const float hpCutoffHz = (filtAmt > 0.0f) ? (40.0f + filtAmt * filtAmt * 6500.0f) : 10.0f;
            const float lpCoeff = sampleRateScaledAlpha(lpCutoffHz, sampleRate);
            const float hpCoeff = sampleRateScaledAlpha(hpCutoffHz, sampleRate);

            for (int i = 0; i < numSamples; ++i)
            {
                const float wL = rng.nextFloat() * 2.0f - 1.0f;
                const float wR = (1.0f - stereoDecorrel) * wL + stereoDecorrel * (rng.nextFloat() * 2.0f - 1.0f);
                float sigL = wL;
                float sigR = wR;

                switch (params.noiseType)
                {
                    case 0: break; // White
                    case 1: // Pink
                        pinkB0 = 0.99765f * pinkB0 + wL * 0.0990460f;
                        pinkB1 = 0.96300f * pinkB1 + wL * 0.2965164f;
                        pinkB2 = 0.57000f * pinkB2 + wL * 1.0526913f;
                        sigL = (pinkB0 + pinkB1 + pinkB2 + wL * 0.1848f) * 0.25f;
                        sigR = sigL * (1.0f - 0.35f * stereoDecorrel) + wR * 0.35f * stereoDecorrel;
                        break;
                    case 2: // Brown
                        brownStateL = juce::jlimit(-1.0f, 1.0f, (brownStateL + 0.045f * wL) * 0.985f);
                        brownStateR = juce::jlimit(-1.0f, 1.0f, (brownStateR + 0.045f * wR) * 0.985f);
                        sigL = brownStateL * 1.8f;
                        sigR = brownStateR * 1.8f;
                        break;
                    case 3: // Vinyl Crackle
                    {
                        brownStateL = (brownStateL + 0.025f * wL) * 0.98f;
                        const float thresh = 0.9965f - params.noisePitch * 0.008f;
                        const float popL = (rng.nextFloat() > thresh) ? (wL * 2.4f) : 0.0f;
                        const float popR = (rng.nextFloat() > thresh) ? (wR * 2.4f) : 0.0f;
                        sigL = brownStateL * 0.65f + popL;
                        sigR = brownStateL * 0.65f + popR;
                        break;
                    }
                    case 4: // Digital Bitnoise
                        bitHoldPhase += bitHoldStep;
                        if (bitHoldPhase >= 1.0f)
                        {
                            bitHoldPhase -= 1.0f;
                            bitHoldSampleL = (wL >= 0.0f ? 0.8f : -0.8f) + 0.2f * wL;
                            bitHoldSampleR = (wR >= 0.0f ? 0.8f : -0.8f) + 0.2f * wR;
                        }
                        sigL = bitHoldSampleL;
                        sigR = bitHoldSampleR;
                        break;
                    case 5: // Breath / Air
                    default:
                    {
                        const float airCut = sampleRateScaledAlpha(600.0f + params.noisePitch * 5500.0f, sampleRate);
                        airBandL += airCut * (wL - airBandL - 0.35f * airLowL);
                        airLowL  += airCut * airBandL;
                        airBandR += airCut * (wR - airBandR - 0.35f * airLowR);
                        airLowR  += airCut * airBandR;
                        sigL = airBandL * 1.8f;
                        sigR = airBandR * 1.8f;
                        break;
                    }
                }

                noiseLpStateL += lpCoeff * (sigL - noiseLpStateL);
                noiseLpStateR += lpCoeff * (sigR - noiseLpStateR);
                noiseHpStateL += hpCoeff * (noiseLpStateL - noiseHpStateL);
                noiseHpStateR += hpCoeff * (noiseLpStateR - noiseHpStateR);

                const float fOutL = (filtAmt > 0.0f) ? (noiseLpStateL - noiseHpStateL) : noiseLpStateL;
                const float fOutR = (filtAmt > 0.0f) ? (noiseLpStateR - noiseHpStateR) : noiseLpStateR;

                outL[i] += fOutL * nGain * panL;
                outR[i] += fOutR * nGain * panR;
            }
        }

        // 2. 4x Oversampled Filter Drive (Zero aliasing on odd/even harmonic saturation)
        if (params.filterDrive > 0.005f || smoothFilterDrive.isSmoothing())
        {
            for (int i = 0; i < numSamples; ++i)
            {
                osScratch.setSample(0, i, sanitizeSample(outL[i]));
                osScratch.setSample(1, i, sanitizeSample(outR[i]));
            }

            juce::dsp::AudioBlock<float> baseBlock(osScratch);
            auto subBlock = baseBlock.getSubBlock(0, static_cast<size_t>(numSamples));
            auto upBlock  = driveOversampler.processSamplesUp(subBlock);

            const size_t osN = upBlock.getNumSamples();
            float* upL = upBlock.getChannelPointer(0);
            float* upR = upBlock.getChannelPointer(1);

            const float stepAmt = (smoothFilterDrive.getTargetValue() - smoothFilterDrive.getCurrentValue())
                                / static_cast<float>(std::max<size_t>(1, osN));
            float curDriveAmt = smoothFilterDrive.getCurrentValue();
            smoothFilterDrive.skip(numSamples);

            for (size_t i = 0; i < osN; ++i)
            {
                const float amt = juce::jlimit(0.0f, 1.0f, curDriveAmt);
                const float preGain = 1.0f + amt * 3.5f;
                const float comp    = 1.0f / (1.0f + amt * 1.15f);
                const float asym    = 1.0f + 0.20f * amt;

                const float dryL = upL[i];
                const float dryR = upR[i];
                const float xL = juce::jlimit(-12.0f, 12.0f, dryL * preGain);
                const float xR = juce::jlimit(-12.0f, 12.0f, dryR * preGain);
                const float satL = (xL >= 0.0f ? std::tanh(xL * asym) : std::tanh(xL / asym)) * comp;
                const float satR = (xR >= 0.0f ? std::tanh(xR * asym) : std::tanh(xR / asym)) * comp;

                upL[i] = dryL * (1.0f - amt) + satL * amt;
                upR[i] = dryR * (1.0f - amt) + satR * amt;
                curDriveAmt += stepAmt;
            }

            driveOversampler.processSamplesDown(subBlock);
            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] = driveDcL.process(osScratch.getSample(0, i));
                outR[i] = driveDcR.process(osScratch.getSample(1, i));
            }
        }
        else
        {
            smoothFilterDrive.skip(numSamples);
        }

        // 3. Time Modulation Engine
        processTimeModulation(outL, outR, numSamples, params, timeModCurve, visOut);

        // 4. Dynamic Reorderable Multi-FX Chain (13 Modules)
        for (int slot = 0; slot < chainCount; ++slot)
        {
            const int modId = chain[static_cast<size_t>(slot)];
            processSingleFxModule(static_cast<FxModuleId>(modId), outL, outR, numSamples, params, outCompGr);
        }

        // 5. Master Gain (Smoothed) + Studio Brickwall Limiter
        const float ceilingLinear = juce::Decibels::decibelsToGain(juce::jlimit(-18.0f, 0.0f, params.limiterThreshDb));
        const float attCoeff = 1.0f - std::exp(-invSampleRate / 0.002f);
        const float relCoeff = 1.0f - std::exp(-invSampleRate / 0.080f);

        for (int i = 0; i < numSamples; ++i)
        {
            const float gainLin = juce::Decibels::decibelsToGain(smoothMasterGainDb.getNextValue());
            float sL = sanitizeSample(outL[i]) * gainLin;
            float sR = sanitizeSample(outR[i]) * gainLin;

            if (params.limiterEnable)
            {
                const float absMax = std::max(std::abs(sL), std::abs(sR));
                const float targetLimGain = (absMax > ceilingLinear && absMax > 1e-5f)
                    ? (ceilingLinear / absMax)
                    : 1.0f;

                if (targetLimGain < limiterEnv)
                    limiterEnv += attCoeff * (targetLimGain - limiterEnv);
                else
                    limiterEnv += relCoeff * (targetLimGain - limiterEnv);

                sL *= limiterEnv;
                sR *= limiterEnv;

                sL = softClipC1(sL, ceilingLinear * 0.90f, ceilingLinear);
                sR = softClipC1(sR, ceilingLinear * 0.90f, ceilingLinear);

                const float grDb = (limiterEnv < 0.999f)
                    ? -juce::Decibels::gainToDecibels(std::max(0.01f, limiterEnv))
                    : 0.0f;
                outLimGr = std::max(outLimGr, grDb);
            }
            else
            {
                sL = softClipC1(sL, 0.90f, 1.0f);
                sR = softClipC1(sR, 0.90f, 1.0f);
            }

            outL[i] = juce::jlimit(-1.0f, 1.0f, sL);
            outR[i] = juce::jlimit(-1.0f, 1.0f, sR);
        }
    }

    void processSingleFxModule(FxModuleId modId,
                               float* outL,
                               float* outR,
                               int numSamples,
                               const SynthParameters& params,
                               float& outCompGr)
    {
        switch (modId)
        {
            case FxModuleId::Blur:
                break; // Per-voice spectral time/harmonic blur handled directly inside SpectralSynthEngine

            case FxModuleId::Tremolo:
            {
                if (!params.tremEnable || (params.tremDepth <= 0.005f && !smoothTremDepth.isSmoothing()))
                {
                    smoothTremDepth.skip(numSamples);
                    break;
                }
                const float tremInc = (params.tremSpeed * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float tremStereoOffset = params.tremGap * juce::MathConstants<float>::pi;
                for (int i = 0; i < numSamples; ++i)
                {
                    const float depth = smoothTremDepth.getNextValue();
                    tremMasterPhase += tremInc;
                    if (tremMasterPhase >= juce::MathConstants<float>::twoPi)
                        tremMasterPhase -= juce::MathConstants<float>::twoPi;

                    const float gL = 1.0f - depth * 0.85f * (0.5f + 0.5f * fastSin(tremMasterPhase));
                    const float gR = 1.0f - depth * 0.85f * (0.5f + 0.5f * fastSin(tremMasterPhase + tremStereoOffset));
                    outL[i] *= gL;
                    outR[i] *= gR;
                }
                break;
            }

            case FxModuleId::Distortion:
            {
                if (!params.distEnable || (params.distMix <= 0.005f && !smoothDistMix.isSmoothing()))
                {
                    smoothDistDrive.skip(numSamples);
                    smoothDistMix.skip(numSamples);
                    break;
                }

                for (int i = 0; i < numSamples; ++i)
                {
                    osScratch.setSample(0, i, sanitizeSample(outL[i]));
                    osScratch.setSample(1, i, sanitizeSample(outR[i]));
                }

                juce::dsp::AudioBlock<float> baseBlock(osScratch);
                auto subBlock = baseBlock.getSubBlock(0, static_cast<size_t>(numSamples));
                auto upBlock  = distOversampler.processSamplesUp(subBlock);

                const size_t osN = upBlock.getNumSamples();
                float* upL = upBlock.getChannelPointer(0);
                float* upR = upBlock.getChannelPointer(1);

                const float invOsN = 1.0f / static_cast<float>(std::max<size_t>(1, osN));
                float curD = smoothDistDrive.getCurrentValue();
                const float stepD = (smoothDistDrive.getTargetValue() - curD) * invOsN;
                float curM = smoothDistMix.getCurrentValue();
                const float stepM = (smoothDistMix.getTargetValue() - curM) * invOsN;
                smoothDistDrive.skip(numSamples);
                smoothDistMix.skip(numSamples);

                const double osSr = sampleRate * 4.0;
                const float toneHz = 450.0f + std::pow(juce::jlimit(0.0f, 1.0f, params.distTone), 1.8f) * 17500.0f;
                const float toneCoeff = sampleRateScaledAlpha(toneHz, osSr);

                for (size_t i = 0; i < osN; ++i)
                {
                    const float drvAmt = juce::jlimit(0.0f, 1.0f, curD);
                    const float mix    = juce::jlimit(0.0f, 1.0f, curM);
                    const float drive  = 1.0f + drvAmt * 9.5f;
                    const float comp   = 1.0f / (1.0f + drvAmt * 1.4f);

                    const float dryL = upL[i];
                    const float dryR = upR[i];
                    const float xL = dryL * drive;
                    const float xR = dryR * drive;
                    float wL = xL;
                    float wR = xR;

                    if (params.distType == 0) // Tube
                    {
                        const float asym = 1.0f + 0.25f * drvAmt;
                        wL = (xL >= 0.0f ? std::tanh(xL * asym) : std::tanh(xL / asym)) * comp;
                        wR = (xR >= 0.0f ? std::tanh(xR * asym) : std::tanh(xR / asym)) * comp;
                    }
                    else if (params.distType == 1) // Tape
                    {
                        wL = (2.0f / juce::MathConstants<float>::pi) * std::atan(xL * 1.35f) * comp;
                        wR = (2.0f / juce::MathConstants<float>::pi) * std::atan(xR * 1.35f) * comp;
                    }
                    else if (params.distType == 2) // Hard Clip
                    {
                        wL = juce::jlimit(-0.85f, 0.85f, xL) * comp;
                        wR = juce::jlimit(-0.85f, 0.85f, xR) * comp;
                    }
                    else if (params.distType == 3) // Wavefold
                    {
                        wL = fastSin(xL * 1.6f) * comp;
                        wR = fastSin(xR * 1.6f) * comp;
                    }
                    else if (params.distType == 4) // Bitcrush Fuzz
                    {
                        const float levels = std::max(3.0f, 24.0f * (1.0f - drvAmt * 0.82f));
                        wL = (std::round(std::tanh(xL) * levels) / levels) * comp * 1.15f;
                        wR = (std::round(std::tanh(xR) * levels) / levels) * comp * 1.15f;
                    }
                    else // Asymmetric Scream
                    {
                        wL = ((xL >= 0.0f) ? std::tanh(xL * 1.6f) : (std::atan(xL * 0.8f) * 0.7f)) * comp * 1.2f;
                        wR = ((xR >= 0.0f) ? std::tanh(xR * 1.6f) : (std::atan(xR * 0.8f) * 0.7f)) * comp * 1.2f;
                    }

                    wL = juce::jlimit(-1.5f, 1.5f, wL);
                    wR = juce::jlimit(-1.5f, 1.5f, wR);

                    distToneStateL += toneCoeff * (wL - distToneStateL);
                    distToneStateR += toneCoeff * (wR - distToneStateR);

                    upL[i] = dryL * (1.0f - mix) + distToneStateL * mix;
                    upR[i] = dryR * (1.0f - mix) + distToneStateR * mix;

                    curD += stepD;
                    curM += stepM;
                }

                distOversampler.processSamplesDown(subBlock);
                for (int i = 0; i < numSamples; ++i)
                {
                    outL[i] = distDcL.process(osScratch.getSample(0, i));
                    outR[i] = distDcR.process(osScratch.getSample(1, i));
                }
                break;
            }

            case FxModuleId::Phaser:
            {
                if (!params.phaserEnable || (params.phaserMix <= 0.005f && !smoothPhaserMix.isSmoothing()))
                {
                    smoothPhaserMix.skip(numSamples);
                    break;
                }
                const float phaserInc = (params.phaserSpeed * invSampleRate) * juce::MathConstants<float>::twoPi;
                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothPhaserMix.getNextValue();
                    phaserLfoPhase += phaserInc;
                    if (phaserLfoPhase >= juce::MathConstants<float>::twoPi)
                        phaserLfoPhase -= juce::MathConstants<float>::twoPi;

                    const float modL = 0.5f + 0.5f * fastSin(phaserLfoPhase + params.phaserOffset * juce::MathConstants<float>::twoPi);
                    const float modR = 0.5f + 0.5f * fastCos(phaserLfoPhase + params.phaserOffset * juce::MathConstants<float>::twoPi);

                    const float minF = 180.0f;
                    const float maxF = 4200.0f * (0.3f + 0.7f * params.phaserWidth);
                    const float fL = minF + (maxF - minF) * modL;
                    const float fR = minF + (maxF - minF) * modR;

                    const float tL = std::tan(juce::MathConstants<float>::pi * std::min(0.45f, fL * invSampleRate));
                    const float tR = std::tan(juce::MathConstants<float>::pi * std::min(0.45f, fR * invSampleRate));
                    const float aL = (1.0f - tL) / (1.0f + tL);
                    const float aR = (1.0f - tR) / (1.0f + tR);

                    float yL = outL[i];
                    float yR = outR[i];
                    for (size_t st = 0; st < 6; ++st)
                    {
                        const float outApL = -aL * yL + phaserApStateL[st];
                        phaserApStateL[st] = yL + aL * outApL;
                        yL = outApL;

                        const float outApR = -aR * yR + phaserApStateR[st];
                        phaserApStateR[st] = yR + aR * outApR;
                        yR = outApR;
                    }

                    outL[i] += mix * 0.75f * yL;
                    outR[i] += mix * 0.75f * yR;
                }
                break;
            }

            case FxModuleId::Chorus:
            {
                if (!params.chorusEnable || (params.chorusMix <= 0.005f && !smoothChorusMix.isSmoothing()))
                {
                    smoothChorusMix.skip(numSamples);
                    break;
                }
                const int voicesCount = juce::jlimit(2, 6, params.chorusVoices);
                const float rateInc = (params.chorusRate * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float baseDelaySamples = static_cast<float>(sampleRate) * 0.014f;
                const float depthSamples = static_cast<float>(sampleRate) * (0.001f + params.chorusDepth * 0.011f);
                const float spread = juce::jlimit(0.0f, 1.0f, params.chorusSpread);
                const float vNorm = 1.0f / std::sqrt(static_cast<float>(voicesCount));
                const int bufMask = chorusBufSize - 1;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothChorusMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];

                    chorusBufL[static_cast<size_t>(chorusWritePos)] = inL;
                    chorusBufR[static_cast<size_t>(chorusWritePos)] = inR;

                    chorusPhase += rateInc;
                    if (chorusPhase >= juce::MathConstants<float>::twoPi)
                        chorusPhase -= juce::MathConstants<float>::twoPi;

                    float wetL = 0.0f, wetR = 0.0f;
                    for (int v = 0; v < voicesCount; ++v)
                    {
                        const float vPhase = chorusPhase + (static_cast<float>(v) / static_cast<float>(voicesCount)) * juce::MathConstants<float>::twoPi;
                        const float dSamp = baseDelaySamples + fastSin(vPhase) * depthSamples;
                        float readIdx = static_cast<float>(chorusWritePos) - dSamp;
                        while (readIdx < 0.0f)
                            readIdx += static_cast<float>(chorusBufSize);

                        const int r0 = static_cast<int>(readIdx) & bufMask;
                        const int r1 = (r0 + 1) & bufMask;
                        const float frac = readIdx - std::floor(readIdx);

                        const float tapL = chorusBufL[static_cast<size_t>(r0)] * (1.0f - frac) + chorusBufL[static_cast<size_t>(r1)] * frac;
                        const float tapR = chorusBufR[static_cast<size_t>(r0)] * (1.0f - frac) + chorusBufR[static_cast<size_t>(r1)] * frac;

                        const float panV = (v & 1) ? spread : -spread;
                        wetL += tapL * (0.5f - 0.5f * panV) + tapR * (0.5f + 0.5f * panV);
                        wetR += tapR * (0.5f + 0.5f * panV) + tapL * (0.5f - 0.5f * panV);
                    }

                    outL[i] = inL * (1.0f - mix) + (wetL * vNorm) * mix;
                    outR[i] = inR * (1.0f - mix) + (wetR * vNorm) * mix;

                    chorusWritePos = (chorusWritePos + 1) & bufMask;
                }
                break;
            }

            case FxModuleId::ParametricEQ:
            {
                if (!params.eqEnable)
                    break;
                if (std::abs(params.eqLowGain) < 0.02f && std::abs(params.eqMidGain) < 0.02f && std::abs(params.eqHighGain) < 0.02f)
                    break;

                if (std::abs(params.eqLowFreq - lastEqLowFreq) > 0.1f
                    || std::abs(params.eqLowGain - lastEqLowGain) > 0.02f
                    || std::abs(params.eqMidFreq - lastEqMidFreq) > 0.5f
                    || std::abs(params.eqMidGain - lastEqMidGain) > 0.02f
                    || std::abs(params.eqMidQ    - lastEqMidQ)    > 0.01f
                    || std::abs(params.eqHighFreq - lastEqHighFreq) > 1.0f
                    || std::abs(params.eqHighGain - lastEqHighGain) > 0.02f)
                {
                    eqLowShelf.setLowShelf(sampleRate, params.eqLowFreq, params.eqLowGain);
                    eqMidPeak.setPeak(sampleRate, params.eqMidFreq, params.eqMidQ, params.eqMidGain);
                    eqHighShelf.setHighShelf(sampleRate, params.eqHighFreq, params.eqHighGain);

                    lastEqLowFreq = params.eqLowFreq;   lastEqLowGain = params.eqLowGain;
                    lastEqMidFreq = params.eqMidFreq;   lastEqMidGain = params.eqMidGain;
                    lastEqMidQ    = params.eqMidQ;
                    lastEqHighFreq = params.eqHighFreq; lastEqHighGain = params.eqHighGain;
                }

                for (int i = 0; i < numSamples; ++i)
                {
                    float sL = outL[i];
                    float sR = outR[i];
                    eqLowShelf.processSample(sL, sR);
                    eqMidPeak.processSample(sL, sR);
                    eqHighShelf.processSample(sL, sR);
                    outL[i] = sL;
                    outR[i] = sR;
                }
                break;
            }

            case FxModuleId::Delay:
            {
                if (!params.delayEnable || (params.delayMix <= 0.005f && !smoothDelayMix.isSmoothing()))
                {
                    smoothDelayTime.skip(numSamples);
                    smoothDelayMix.skip(numSamples);
                    break;
                }
                const float beatSec = 60.0f / static_cast<float>(juce::jlimit(40.0, 260.0, params.hostBpm));
                const float fb = juce::jlimit(0.0f, 0.86f, params.delayFeedback);
                const float pp = juce::jlimit(0.0f, 1.0f, params.delayPingPong);
                const float dampHz = 350.0f + std::pow(juce::jlimit(0.0f, 1.0f, params.delayFilter), 1.8f) * 16500.0f;
                const float dampCoeff = sampleRateScaledAlpha(dampHz, sampleRate);
                const int bufMask = delayBufSize - 1;
                const float maxDelay = static_cast<float>(delayBufSize - 8);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float dNorm = smoothDelayTime.getNextValue();
                    const float mix   = smoothDelayMix.getNextValue();

                    const float delaySecL = juce::jlimit(0.015f, 1.9f, beatSec * (0.125f + dNorm * 0.875f));
                    const float delaySecR = juce::jlimit(0.015f, 1.9f, delaySecL * (1.0f + pp * 0.25f));
                    const float dSampL = juce::jlimit(16.0f, maxDelay, delaySecL * static_cast<float>(sampleRate));
                    const float dSampR = juce::jlimit(16.0f, maxDelay, delaySecR * static_cast<float>(sampleRate));

                    auto readInterp = [&](const std::vector<float>& buf, float dSamp) -> float
                    {
                        float rp = static_cast<float>(delayWritePos) - dSamp;
                        while (rp < 0.0f) rp += static_cast<float>(delayBufSize);
                        const int r0 = static_cast<int>(rp) & bufMask;
                        const int r1 = (r0 + 1) & bufMask;
                        const float frac = rp - std::floor(rp);
                        return sanitizeSample(buf[static_cast<size_t>(r0)] * (1.0f - frac) + buf[static_cast<size_t>(r1)] * frac);
                    };

                    const float tapL = readInterp(delayBufL, dSampL);
                    const float tapR = readInterp(delayBufR, dSampR);

                    delayLpStateL += dampCoeff * (tapL - delayLpStateL);
                    delayLpStateR += dampCoeff * (tapR - delayLpStateR);

                    const float inL = outL[i];
                    const float inR = outR[i];
                    const float fbInL = delayLpStateL * (1.0f - pp * 0.7f) + delayLpStateR * (pp * 0.7f);
                    const float fbInR = delayLpStateR * (1.0f - pp * 0.7f) + delayLpStateL * (pp * 0.7f);

                    delayBufL[static_cast<size_t>(delayWritePos)] = std::tanh((inL + fbInL * fb) * 0.75f) * 1.33f;
                    delayBufR[static_cast<size_t>(delayWritePos)] = std::tanh((inR + fbInR * fb) * 0.75f) * 1.33f;

                    outL[i] = inL + delayLpStateL * mix;
                    outR[i] = inR + delayLpStateR * mix;

                    delayWritePos = (delayWritePos + 1) & bufMask;
                }
                break;
            }

            case FxModuleId::Reverb:
            {
                if (!params.reverbEnable || (params.reverbMix <= 0.005f && !smoothReverbMix.isSmoothing()))
                {
                    smoothReverbMix.skip(numSamples);
                    break;
                }

                const double srScale = sampleRate / 44100.0;
                const float fb = juce::jlimit(0.40f, 0.88f, 0.50f + params.reverbDecay * 0.38f);
                const float dampHz = 600.0f + std::pow(1.0f - juce::jlimit(0.0f, 1.0f, params.reverbDamping), 1.8f) * 15000.0f;
                const float dampCoeff = sampleRateScaledAlpha(dampHz, sampleRate);
                const float shimmerAmt = params.reverbShimmer * 0.32f;
                const int preDelaySamples = juce::jlimit(1, revPreBufSize - 4, static_cast<int>(params.reverbPreDelay * 0.085f * static_cast<float>(sampleRate)));
                const float sizeScale = (0.55f + params.reverbSize * 0.45f) * static_cast<float>(srScale);
                const int preMask = revPreBufSize - 1;

                const int cLen0 = std::max(64, std::min(static_cast<int>(revCombBufL0.size()) - 4, static_cast<int>(1116.0f * sizeScale)));
                const int cLen1 = std::max(64, std::min(static_cast<int>(revCombBufL1.size()) - 4, static_cast<int>(1356.0f * sizeScale)));
                const int cLen2 = std::max(64, std::min(static_cast<int>(revCombBufL2.size()) - 4, static_cast<int>(1491.0f * sizeScale)));
                const int cLen3 = std::max(64, std::min(static_cast<int>(revCombBufL3.size()) - 4, static_cast<int>(1617.0f * sizeScale)));

                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothReverbMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];

                    revPreBufL[static_cast<size_t>(revPrePos)] = inL;
                    revPreBufR[static_cast<size_t>(revPrePos)] = inR;
                    const int preRead = (revPrePos - preDelaySamples + revPreBufSize) & preMask;
                    const float preL = revPreBufL[static_cast<size_t>(preRead)];
                    const float preR = revPreBufR[static_cast<size_t>(preRead)];
                    revPrePos = (revPrePos + 1) & preMask;

                    // DC-blocked shimmer harmonic injection
                    auto runComb = [&](std::vector<float>& bufL, std::vector<float>& bufR,
                                       int& pos, int len, float& lpL, float& lpR, float& sumL, float& sumR)
                    {
                        if (pos >= len) pos %= len;
                        const float yL = bufL[static_cast<size_t>(pos)];
                        const float yR = bufR[static_cast<size_t>(pos)];
                        lpL += dampCoeff * (yL - lpL);
                        lpR += dampCoeff * (yR - lpR);
                        if (!std::isfinite(lpL)) lpL = 0.0f;
                        if (!std::isfinite(lpR)) lpR = 0.0f;

                        // Asymmetric soft rectification + DC blocker creates octave-up shimmer without DC buildup
                        const float shimL = revShimDcL.process(std::abs(lpL)) * shimmerAmt;
                        const float shimR = revShimDcR.process(std::abs(lpR)) * shimmerAmt;

                        bufL[static_cast<size_t>(pos)] = std::tanh((preL * 0.25f + (lpL + shimL) * fb) * 0.75f) * 1.33f;
                        bufR[static_cast<size_t>(pos)] = std::tanh((preR * 0.25f + (lpR + shimR) * fb) * 0.75f) * 1.33f;
                        pos = (pos + 1) % len;
                        sumL += yL;
                        sumR += yR;
                    };

                    float tankL = 0.0f, tankR = 0.0f;
                    runComb(revCombBufL0, revCombBufR0, revPos0, cLen0, revLpL0, revLpR0, tankL, tankR);
                    runComb(revCombBufL1, revCombBufR1, revPos1, cLen1, revLpL1, revLpR1, tankL, tankR);
                    runComb(revCombBufL2, revCombBufR2, revPos2, cLen2, revLpL2, revLpR2, tankL, tankR);
                    runComb(revCombBufL3, revCombBufR3, revPos3, cLen3, revLpL3, revLpR3, tankL, tankR);

                    if (revApPos0 >= revApLen0) revApPos0 %= revApLen0;
                    const float apOutL0 = -0.5f * tankL + revApBufL0[static_cast<size_t>(revApPos0)];
                    const float apOutR0 = -0.5f * tankR + revApBufR0[static_cast<size_t>(revApPos0)];
                    revApBufL0[static_cast<size_t>(revApPos0)] = tankL + 0.5f * apOutL0;
                    revApBufR0[static_cast<size_t>(revApPos0)] = tankR + 0.5f * apOutR0;
                    revApPos0 = (revApPos0 + 1) % revApLen0;

                    if (revApPos1 >= revApLen1) revApPos1 %= revApLen1;
                    const float apOutL1 = -0.5f * apOutL0 + revApBufL1[static_cast<size_t>(revApPos1)];
                    const float apOutR1 = -0.5f * apOutR0 + revApBufR1[static_cast<size_t>(revApPos1)];
                    revApBufL1[static_cast<size_t>(revApPos1)] = apOutL0 + 0.5f * apOutL1;
                    revApBufR1[static_cast<size_t>(revApPos1)] = apOutR0 + 0.5f * apOutR1;
                    revApPos1 = (revApPos1 + 1) % revApLen1;

                    outL[i] = inL * (1.0f - mix * 0.45f) + apOutL1 * mix * 0.65f;
                    outR[i] = inR * (1.0f - mix * 0.45f) + apOutR1 * mix * 0.65f;
                }
                break;
            }

            case FxModuleId::Compressor:
            {
                if (!params.compEnable || (params.compMix <= 0.005f && !smoothCompMix.isSmoothing()))
                {
                    smoothCompMix.skip(numSamples);
                    break;
                }

                const float threshDb = juce::jlimit(-48.0f, 0.0f, params.compThreshold);
                const float ratio    = juce::jlimit(1.0f, 20.0f, params.compRatio);
                const float attSec   = std::max(0.0001f, params.compAttack * 0.001f);
                const float relSec   = std::max(0.005f,  params.compRelease * 0.001f);
                const float attCoeff = 1.0f - std::exp(-invSampleRate / attSec);
                const float relCoeff = 1.0f - std::exp(-invSampleRate / relSec);
                const float makeup   = juce::Decibels::decibelsToGain(params.compMakeup);

                const float xLowCoeff = sampleRateScaledAlpha(160.0f, sampleRate);
                const float xMidCoeff = sampleRateScaledAlpha(2500.0f, sampleRate);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothCompMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];
                    float wetL = inL;
                    float wetR = inR;

                    if (params.compMode == 1) // Multiband OTT
                    {
                        compLpLowL += xLowCoeff * (inL - compLpLowL);
                        compLpLowR += xLowCoeff * (inR - compLpLowR);
                        compLpMidL += xMidCoeff * (inL - compLpMidL);
                        compLpMidR += xMidCoeff * (inR - compLpMidR);

                        const float bandL[3] = { compLpLowL, compLpMidL - compLpLowL, inL - compLpMidL };
                        const float bandR[3] = { compLpLowR, compLpMidR - compLpLowR, inR - compLpMidR };
                        static constexpr float bandMakeup[3] = { 1.15f, 1.25f, 1.45f };

                        wetL = 0.0f;
                        wetR = 0.0f;
                        for (size_t b = 0; b < 3; ++b)
                        {
                            const float pk = std::max(std::abs(bandL[b]), std::abs(bandR[b]));
                            const float c = (pk > compEnvBand[b]) ? attCoeff : relCoeff;
                            compEnvBand[b] += c * (pk - compEnvBand[b]);

                            const float envDb = juce::Decibels::gainToDecibels(std::max(1e-5f, compEnvBand[b]));
                            float gainChangeDb = 0.0f;

                            if (envDb > threshDb)
                            {
                                gainChangeDb = (threshDb - envDb) * (1.0f - 1.0f / ratio);
                                outCompGr = std::max(outCompGr, -gainChangeDb);
                            }
                            else if (envDb > -52.0f && envDb < (threshDb - 10.0f))
                            {
                                const float below = (threshDb - 10.0f) - envDb;
                                gainChangeDb = std::min(12.0f, below * 0.45f);
                            }

                            const float g = juce::Decibels::decibelsToGain(gainChangeDb) * bandMakeup[b];
                            wetL += bandL[b] * g;
                            wetR += bandR[b] * g;
                        }
                        wetL *= makeup * 0.78f;
                        wetR *= makeup * 0.78f;
                    }
                    else
                    {
                        const float pk = std::max(std::abs(inL), std::abs(inR));
                        const float effAtt = (params.compMode == 2) ? (attCoeff * 0.35f) : attCoeff;
                        const float c = (pk > compEnvSingle) ? effAtt : relCoeff;
                        compEnvSingle += c * (pk - compEnvSingle);

                        const float envDb = juce::Decibels::gainToDecibels(std::max(1e-5f, compEnvSingle));
                        float gainChangeDb = 0.0f;
                        if (envDb > threshDb)
                        {
                            const float effRatio = (params.compMode == 3) ? std::max(10.0f, ratio) : ratio;
                            gainChangeDb = (threshDb - envDb) * (1.0f - 1.0f / effRatio);
                            outCompGr = std::max(outCompGr, -gainChangeDb);
                        }
                        const float g = juce::Decibels::decibelsToGain(gainChangeDb) * makeup;
                        wetL = inL * g;
                        wetR = inR * g;
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::RingMod:
            {
                if (!params.ringEnable || (params.ringMix <= 0.005f && !smoothRingMix.isSmoothing()))
                {
                    smoothRingFreq.skip(numSamples);
                    smoothRingMix.skip(numSamples);
                    break;
                }

                const float depth  = juce::jlimit(0.0f, 1.0f, params.ringDepth);
                const float lfoInc = (params.ringLfoRate * invSampleRate) * juce::MathConstants<float>::twoPi;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float baseFreq = smoothRingFreq.getNextValue();
                    const float mix      = smoothRingMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];

                    ringLfoPhase += lfoInc;
                    if (ringLfoPhase >= juce::MathConstants<float>::twoPi)
                        ringLfoPhase -= juce::MathConstants<float>::twoPi;

                    const float lfoSin  = fastSin(ringLfoPhase);
                    const float lfoCos  = fastCos(ringLfoPhase);
                    const float modFreq = baseFreq * std::pow(2.0f, lfoSin * params.ringLfoDepth * 1.5f);
                    const float carInc  = (modFreq * invSampleRate) * juce::MathConstants<float>::twoPi;

                    ringCarrierPhase += carInc;
                    if (ringCarrierPhase >= juce::MathConstants<float>::twoPi)
                        ringCarrierPhase -= juce::MathConstants<float>::twoPi;

                    const float cSin = fastSin(ringCarrierPhase);
                    const float cCos = fastCos(ringCarrierPhase);

                    float wetL = inL, wetR = inR;
                    if (params.ringMode == 0) // Diode Ring Mod
                    {
                        const float modL = (1.0f - depth) + depth * cSin;
                        const float modR = (1.0f - depth) + depth * (cSin * 0.7f + cCos * 0.3f);
                        wetL = std::tanh(inL * modL * 1.35f);
                        wetR = std::tanh(inR * modR * 1.35f);
                    }
                    else if (params.ringMode == 1) // True Hilbert SSB Frequency Shifter
                    {
                        float iL = 0.0f, qL = 0.0f, iR = 0.0f, qR = 0.0f;
                        ringHilbertL.process(inL, iL, qL);
                        ringHilbertR.process(inR, iR, qR);
                        const float shiftedL = iL * cCos + qL * cSin;
                        const float shiftedR = iR * cCos + qR * cSin;
                        wetL = inL * (1.0f - depth) + shiftedL * depth;
                        wetR = inR * (1.0f - depth) + shiftedR * depth;
                    }
                    else if (params.ringMode == 2) // Stereo Auto-Pan + Harmonic Ring
                    {
                        const float panModL = 0.5f + 0.5f * lfoSin * depth;
                        const float panModR = 0.5f - 0.5f * lfoSin * depth;
                        const float subtleRing = (1.0f - params.ringLfoDepth * 0.5f) + params.ringLfoDepth * 0.5f * cSin;
                        wetL = inL * panModL * 1.4f * subtleRing;
                        wetR = inR * panModR * 1.4f * subtleRing;
                    }
                    else // Rotary Speaker
                    {
                        const float hornL = 1.0f + 0.45f * depth * lfoSin;
                        const float hornR = 1.0f + 0.45f * depth * lfoCos;
                        wetL = std::tanh(inL * hornL * (1.0f + 0.25f * cSin));
                        wetR = std::tanh(inR * hornR * (1.0f + 0.25f * cCos));
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::Bitcrusher:
            {
                if (!params.crushEnable || (params.crushMix <= 0.005f && !smoothCrushMix.isSmoothing()))
                {
                    smoothCrushMix.skip(numSamples);
                    break;
                }
                const float bits      = juce::jlimit(1.0f, 16.0f, params.crushBits);
                const float levels    = std::pow(2.0f, bits - 1.0f);
                const float dsFactor  = juce::jlimit(1.0f, 64.0f, params.crushDownsample) * (static_cast<float>(sampleRate) / 44100.0f);
                const float holdStep  = 1.0f / std::max(1.0f, dsFactor);
                const float drive     = 1.0f + params.crushDrive * 3.2f;
                const float toneHz    = 500.0f + std::pow(juce::jlimit(0.0f, 1.0f, params.crushTone), 1.8f) * 16500.0f;
                const float toneCoeff = sampleRateScaledAlpha(toneHz, sampleRate);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothCrushMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];

                    crushHoldPhase += holdStep;
                    if (crushHoldPhase >= 1.0f)
                    {
                        crushHoldPhase -= std::floor(crushHoldPhase);
                        const float preL = std::tanh(inL * drive);
                        const float preR = std::tanh(inR * drive);

                        if (params.crushMode == 0)
                        {
                            crushHeldL = std::round(preL * levels) / levels;
                            crushHeldR = std::round(preR * levels) / levels;
                        }
                        else if (params.crushMode == 1)
                        {
                            const float fineLevels = std::max(8.0f, levels * 2.0f);
                            crushHeldL = std::round(preL * fineLevels) / fineLevels;
                            crushHeldR = std::round(preR * fineLevels) / fineLevels;
                        }
                        else if (params.crushMode == 2)
                        {
                            const float spLevels = std::min(2048.0f, std::max(4.0f, levels));
                            const float asymL = preL + 0.08f * preL * preL;
                            const float asymR = preR + 0.08f * preR * preR;
                            crushHeldL = std::round(asymL * spLevels) / spLevels;
                            crushHeldR = std::round(asymR * spLevels) / spLevels;
                        }
                        else
                        {
                            const float rLevels = std::max(3.0f, levels * 0.75f);
                            crushHeldL = std::round(preL * rLevels) / rLevels;
                            crushHeldR = std::round(preR * rLevels) / rLevels;
                        }
                    }

                    crushLpL += toneCoeff * (crushHeldL - crushLpL);
                    crushLpR += toneCoeff * (crushHeldR - crushLpR);

                    float wetL = crushLpL;
                    float wetR = crushLpR;
                    if (params.crushMode == 3)
                    {
                        wetL = (crushHeldL - crushLpL * 0.45f) * 1.15f;
                        wetR = (crushHeldR - crushLpR * 0.45f) * 1.15f;
                    }

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::Flanger:
            {
                if (!params.flangerEnable || (params.flangerMix <= 0.005f && !smoothFlangerMix.isSmoothing()))
                {
                    smoothFlangerMix.skip(numSamples);
                    break;
                }

                const float rateInc = (params.flangerRate * invSampleRate) * juce::MathConstants<float>::twoPi;
                const float depth   = juce::jlimit(0.0f, 1.0f, params.flangerDepth);
                const float fb      = juce::jlimit(-0.86f, 0.86f, params.flangerFeedback);
                const float manual  = juce::jlimit(0.0f, 1.0f, params.flangerManual);

                const float minDelaySamp = static_cast<float>(sampleRate) * (0.00025f + manual * 0.0025f);
                const float sweepSamp    = static_cast<float>(sampleRate) * (0.0004f + depth * 0.0055f);
                const int bufMask = flangerBufSize - 1;

                for (int i = 0; i < numSamples; ++i)
                {
                    const float mix = smoothFlangerMix.getNextValue();
                    const float inL = outL[i];
                    const float inR = outR[i];

                    flangerPhase += rateInc;
                    if (flangerPhase >= juce::MathConstants<float>::twoPi)
                        flangerPhase -= juce::MathConstants<float>::twoPi;

                    float lfoL = 0.5f + 0.5f * fastSin(flangerPhase);
                    float lfoR = 0.5f + 0.5f * fastCos(flangerPhase);

                    if (params.flangerMode == 2)
                    {
                        lfoL = flangerPhase / juce::MathConstants<float>::twoPi;
                        lfoR = std::fmod(lfoL + 0.5f, 1.0f);
                    }
                    else if (params.flangerMode == 3)
                    {
                        lfoL = std::round(lfoL * 8.0f) / 8.0f;
                        lfoR = std::round(lfoR * 8.0f) / 8.0f;
                    }

                    flangerBufL[static_cast<size_t>(flangerWritePos)] = std::tanh((inL + flangerFbL * fb) * 0.75f) * 1.33f;
                    flangerBufR[static_cast<size_t>(flangerWritePos)] = std::tanh((inR + flangerFbR * fb) * 0.75f) * 1.33f;

                    auto readFlangeTap = [&](const std::vector<float>& buf, float delaySamples) -> float
                    {
                        float rPos = static_cast<float>(flangerWritePos) - delaySamples;
                        while (rPos < 0.0f)
                            rPos += static_cast<float>(flangerBufSize);
                        const int r0 = static_cast<int>(rPos) & bufMask;
                        const int r1 = (r0 + 1) & bufMask;
                        const float frac = rPos - std::floor(rPos);
                        return sanitizeSample(buf[static_cast<size_t>(r0)] * (1.0f - frac) + buf[static_cast<size_t>(r1)] * frac);
                    };

                    const float tapL = readFlangeTap(flangerBufL, minDelaySamp + lfoL * sweepSamp);
                    const float tapR = readFlangeTap(flangerBufR, minDelaySamp + lfoR * sweepSamp);
                    flangerFbL = tapL;
                    flangerFbR = tapR;

                    if (params.flangerMode == 1)
                    {
                        const float centerDelay = minDelaySamp + 0.5f * sweepSamp;
                        const float dryTzL = readFlangeTap(flangerBufL, centerDelay);
                        const float dryTzR = readFlangeTap(flangerBufR, centerDelay);
                        outL[i] = inL * (1.0f - mix) + (dryTzL - tapL * 0.85f) * mix;
                        outR[i] = inR * (1.0f - mix) + (dryTzR - tapR * 0.85f) * mix;
                    }
                    else
                    {
                        outL[i] = inL * (1.0f - mix * 0.5f) + tapL * mix * 0.78f;
                        outR[i] = inR * (1.0f - mix * 0.5f) + tapR * mix * 0.78f;
                    }

                    flangerWritePos = (flangerWritePos + 1) & bufMask;
                }
                break;
            }

            case FxModuleId::Bode:
            {
                if (!params.bodeEnable || (params.bodeMix <= 0.005f && !smoothBodeMix.isSmoothing()))
                {
                    smoothBodeShift.skip(numSamples);
                    smoothBodeFeedback.skip(numSamples);
                    smoothBodeMix.skip(numSamples);
                    break;
                }

                for (int i = 0; i < numSamples; ++i)
                {
                    const float shift = smoothBodeShift.getNextValue();
                    const float fb    = smoothBodeFeedback.getNextValue();
                    const float mix   = smoothBodeMix.getNextValue();
                    const float phaseInc = (shift * invSampleRate) * juce::MathConstants<float>::twoPi;

                    const float inL = sanitizeSample(outL[i]);
                    const float inR = sanitizeSample(outR[i]);

                    const float fbInL = inL + std::tanh(bodeFbL * 0.85f) * fb;
                    const float fbInR = inR + std::tanh(bodeFbR * 0.85f) * fb;

                    bodeCarrierPhase += phaseInc;
                    if (bodeCarrierPhase > juce::MathConstants<float>::pi)
                        bodeCarrierPhase -= juce::MathConstants<float>::twoPi;
                    else if (bodeCarrierPhase < -juce::MathConstants<float>::pi)
                        bodeCarrierPhase += juce::MathConstants<float>::twoPi;

                    const float cCos = std::cos(bodeCarrierPhase);
                    const float cSin = std::sin(bodeCarrierPhase);

                    float iL = 0.0f, qL = 0.0f;
                    float iR = 0.0f, qR = 0.0f;
                    bodeHilbertL.process(fbInL, iL, qL);
                    bodeHilbertR.process(fbInR, iR, qR);

                    float wetL = std::tanh(bodeDcL.process(sanitizeSample(iL * cCos + qL * cSin)));
                    float wetR = std::tanh(bodeDcR.process(sanitizeSample(iR * cCos + qR * cSin)));

                    bodeFbL = wetL;
                    bodeFbR = wetR;

                    outL[i] = inL * (1.0f - mix) + wetL * mix;
                    outR[i] = inR * (1.0f - mix) + wetR * mix;
                }
                break;
            }

            case FxModuleId::Count:
            default:
                break;
        }
    }

    void processTimeModulation(float* outL,
                               float* outR,
                               int numSamples,
                               const SynthParameters& params,
                               SplineCurve& timeModCurve,
                               VisualizerSnapshot& visOut)
    {
        if (timeModBufL.empty() || timeModBufR.empty())
            return;

        const float safeBpm = static_cast<float>(juce::jlimit(40.0, 260.0, params.hostBpm));
        const float beatsPerSec = safeBpm / 60.0f;
        const float beatSamples = static_cast<float>(sampleRate) / beatsPerSec;

        float cycleHz = beatsPerSec * 0.25f;
        switch (juce::jlimit(0, 5, params.timeModRate))
        {
            case 0: cycleHz = beatsPerSec * 0.125f; break;
            case 1: cycleHz = beatsPerSec * 0.25f;  break;
            case 2: cycleHz = beatsPerSec * 0.5f;   break;
            case 3: cycleHz = beatsPerSec * 1.0f;   break;
            case 4: cycleHz = beatsPerSec * 2.0f;   break;
            case 5: cycleHz = beatsPerSec * 4.0f;   break;
            default: break;
        }

        const float phaseInc = cycleHz * invSampleRate;
        const float maxDelaySamples = std::min(static_cast<float>(timeModBufSize - 64), beatSamples * 2.0f);
        const float depth = juce::jlimit(0.0f, 1.0f, params.timeModDepth);
        const int tmMask = timeModBufSize - 1;

        if (!params.timeModEnable || (params.timeModMix <= 0.002f && !smoothTimeModMix.isSmoothing()))
        {
            smoothTimeModMix.skip(numSamples);
            for (int i = 0; i < numSamples; ++i)
            {
                timeModBufL[static_cast<size_t>(timeModWritePos)] = outL[i];
                timeModBufR[static_cast<size_t>(timeModWritePos)] = outR[i];
                timeModWritePos = (timeModWritePos + 1) & tmMask;
            }
            timeModBarPhase = std::fmod(timeModBarPhase + phaseInc * static_cast<float>(numSamples), 1.0f);
            visOut.timeModCursor = timeModBarPhase;
            visOut.timeModOffsetNorm = timeModCurve.evaluateLUT(timeModBarPhase) * depth;
            return;
        }

        const float smoothTau = 0.002f + std::pow(juce::jlimit(0.0f, 1.0f, params.timeModSmooth), 2.0f) * 0.035f;
        const float smoothCoeff = 1.0f - std::exp(-invSampleRate / smoothTau);

        const float filtAmt = juce::jlimit(-1.0f, 1.0f, params.timeModFilter);
        const float lpHz = (filtAmt < 0.0f) ? (120.0f + std::pow(1.0f + filtAmt, 2.2f) * 16500.0f) : 18500.0f;
        const float hpHz = (filtAmt > 0.0f) ? (30.0f + filtAmt * filtAmt * 5500.0f) : 10.0f;
        const float lpCut = sampleRateScaledAlpha(lpHz, sampleRate);
        const float hpCut = sampleRateScaledAlpha(hpHz, sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = smoothTimeModMix.getNextValue();
            const float inL = outL[i];
            const float inR = outR[i];

            timeModBufL[static_cast<size_t>(timeModWritePos)] = inL;
            timeModBufR[static_cast<size_t>(timeModWritePos)] = inR;

            timeModBarPhase += phaseInc;
            if (timeModBarPhase >= 1.0f)
                timeModBarPhase -= std::floor(timeModBarPhase);

            float targetOffsetNorm = 0.0f;
            float stutterGate = 1.0f;

            switch (params.timeModMode)
            {
                case 0: targetOffsetNorm = timeModCurve.evaluateLUT(timeModBarPhase) * depth; break;
                case 1:
                {
                    const float subdiv = 2.0f + std::round(params.timeModStutter * 6.0f) * 2.0f;
                    const float slicePhase = std::fmod(timeModBarPhase * subdiv, 1.0f);
                    const float repeatRamp = (timeModBarPhase - slicePhase / subdiv);
                    targetOffsetNorm = juce::jlimit(0.0f, 1.0f, repeatRamp * depth);
                    const float gateWidth = 1.0f - params.timeModStutter * 0.45f;
                    if (slicePhase > gateWidth)
                        stutterGate = std::max(0.0f, 1.0f - (slicePhase - gateWidth) * 12.0f);
                    break;
                }
                case 2:
                {
                    const float curveExp = 1.6f + std::abs(params.timeModSpeed) * 2.4f;
                    targetOffsetNorm = std::pow(timeModBarPhase, curveExp) * depth;
                    stutterGate = 1.0f - 0.35f * targetOffsetNorm;
                    break;
                }
                case 3:
                {
                    const float revPhase = std::fmod(timeModBarPhase * 2.0f, 1.0f);
                    targetOffsetNorm = revPhase * depth;
                    const float edgeWin = fastSin(revPhase * juce::MathConstants<float>::pi);
                    stutterGate = std::min(1.0f, edgeWin * 3.5f);
                    break;
                }
                case 4:
                    targetOffsetNorm = (timeModBarPhase * 0.5f) * depth;
                    break;
                case 5:
                default:
                {
                    const float scratch = 0.5f - 0.5f * fastCos(timeModBarPhase * juce::MathConstants<float>::twoPi * 2.0f);
                    targetOffsetNorm = scratch * depth * 0.65f;
                    break;
                }
            }

            const float desiredDelaySamples = targetOffsetNorm * maxDelaySamples;
            timeModPrevOffsetSamples += smoothCoeff * (desiredDelaySamples - timeModPrevOffsetSamples);

            float readPos = static_cast<float>(timeModWritePos) - timeModPrevOffsetSamples;
            while (readPos < 0.0f)
                readPos += static_cast<float>(timeModBufSize);

            const int r0 = static_cast<int>(readPos) & tmMask;
            const int r1 = (r0 + 1) & tmMask;
            const float frac = readPos - std::floor(readPos);

            const float tapL = (timeModBufL[static_cast<size_t>(r0)] * (1.0f - frac)
                              + timeModBufL[static_cast<size_t>(r1)] * frac) * stutterGate;
            const float tapR = (timeModBufR[static_cast<size_t>(r0)] * (1.0f - frac)
                              + timeModBufR[static_cast<size_t>(r1)] * frac) * stutterGate;

            timeModLpL += lpCut * (tapL - timeModLpL);
            timeModLpR += lpCut * (tapR - timeModLpR);
            timeModHpL += hpCut * (timeModLpL - timeModHpL);
            timeModHpR += hpCut * (timeModLpR - timeModHpR);

            const float wetL = (filtAmt > 0.0f) ? (timeModLpL - timeModHpL) : timeModLpL;
            const float wetR = (filtAmt > 0.0f) ? (timeModLpR - timeModHpR) : timeModLpR;

            outL[i] = inL * (1.0f - mix) + wetL * mix;
            outR[i] = inR * (1.0f - mix) + wetR * mix;

            visOut.timeModOffsetNorm = targetOffsetNorm;
            timeModWritePos = (timeModWritePos + 1) & tmMask;
        }

        visOut.timeModCursor = timeModBarPhase;
        const int previewStep = std::max(1, static_cast<int>(maxDelaySamples) / 64);
        for (size_t b = 0; b < 64; ++b)
        {
            const int idx = (timeModWritePos - static_cast<int>((63 - b) * static_cast<size_t>(previewStep)) + timeModBufSize) & tmMask;
            visOut.timeModWavePreview[b] = juce::jlimit(-1.0f, 1.0f, timeModBufL[static_cast<size_t>(idx)]);
        }
    }

    double sampleRate = 44100.0;
    float  invSampleRate = 1.0f / 44100.0f;
    juce::Random rng;
    std::array<float, kSineLutSize> sineLUT {};

    // 4x Oversampling (factor = 2 => 2^2 = 4x)
    juce::dsp::Oversampling<float> driveOversampler { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::Oversampling<float> distOversampler  { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::AudioBuffer<float> osScratch;
    DcBlocker driveDcL, driveDcR;
    DcBlocker distDcL, distDcR;
    DcBlocker revShimDcL, revShimDcR;
    DcBlocker bodeDcL, bodeDcR;

    // Parameter Smoothers
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothFilterDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDistDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDistMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTremDepth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothPhaserMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothChorusMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDelayTime;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothDelayMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothReverbMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCompMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothRingFreq;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothRingMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothCrushMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothFlangerMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBodeShift;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBodeFeedback;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothBodeMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothTimeModMix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothMasterGainDb;

    // Noise Generator State
    float  pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;
    float  brownStateL = 0.0f, brownStateR = 0.0f;
    float  bitHoldPhase = 0.0f, bitHoldSampleL = 0.0f, bitHoldSampleR = 0.0f;
    float  airBandL = 0.0f, airLowL = 0.0f, airBandR = 0.0f, airLowR = 0.0f;
    float  noiseLpStateL = 0.0f, noiseLpStateR = 0.0f;
    float  noiseHpStateL = 0.0f, noiseHpStateR = 0.0f;

    // Tremolo, Distortion & Phaser
    float  tremMasterPhase = 0.0f;
    float  phaserLfoPhase = 0.0f;
    float  distToneStateL = 0.0f, distToneStateR = 0.0f;
    std::array<float, 6> phaserApStateL {};
    std::array<float, 6> phaserApStateR {};

    // Parametric EQ Biquads
    StereoBiquad eqLowShelf, eqMidPeak, eqHighShelf;
    float lastEqLowFreq = -1.0f, lastEqLowGain = 0.0f;
    float lastEqMidFreq = -1.0f, lastEqMidGain = 0.0f, lastEqMidQ = 1.2f;
    float lastEqHighFreq = -1.0f, lastEqHighGain = 0.0f;

    // Chorus State
    int    chorusBufSize = 4096;
    float  chorusPhase = 0.0f;
    int    chorusWritePos = 0;
    std::vector<float> chorusBufL;
    std::vector<float> chorusBufR;

    // Delay State
    int    delayBufSize = 131072;
    int    delayWritePos = 0;
    float  delayLpStateL = 0.0f, delayLpStateR = 0.0f;
    std::vector<float> delayBufL;
    std::vector<float> delayBufR;

    // Reverb State (Dynamic SR-Scaled Combs & Allpasses)
    int    revPreBufSize = 4096;
    int    revApLen0 = 347, revApLen1 = 113;
    int    revPrePos = 0, revPos0 = 0, revPos1 = 0, revPos2 = 0, revPos3 = 0, revApPos0 = 0, revApPos1 = 0;
    float  revLpL0 = 0.0f, revLpR0 = 0.0f, revLpL1 = 0.0f, revLpR1 = 0.0f;
    float  revLpL2 = 0.0f, revLpR2 = 0.0f, revLpL3 = 0.0f, revLpR3 = 0.0f;
    std::vector<float> revPreBufL, revPreBufR;
    std::vector<float> revCombBufL0, revCombBufR0;
    std::vector<float> revCombBufL1, revCombBufR1;
    std::vector<float> revCombBufL2, revCombBufR2;
    std::vector<float> revCombBufL3, revCombBufR3;
    std::vector<float> revApBufL0, revApBufR0;
    std::vector<float> revApBufL1, revApBufR1;

    // Compressor State
    float  compEnvSingle = 0.0001f;
    std::array<float, 3> compEnvBand { 0.0001f, 0.0001f, 0.0001f };
    float  compLpLowL = 0.0f, compLpLowR = 0.0f;
    float  compLpMidL = 0.0f, compLpMidR = 0.0f;

    // Ring Mod State (w/ Hilbert pair for Mode 1)
    float  ringCarrierPhase = 0.0f;
    float  ringLfoPhase = 0.0f;
    BodeHilbertPair ringHilbertL;
    BodeHilbertPair ringHilbertR;

    // Bitcrusher State
    float  crushHoldPhase = 0.0f;
    float  crushHeldL = 0.0f, crushHeldR = 0.0f;
    float  crushLpL = 0.0f, crushLpR = 0.0f;

    // Flanger State
    int    flangerBufSize = 2048;
    float  flangerPhase = 0.0f;
    int    flangerWritePos = 0;
    float  flangerFbL = 0.0f, flangerFbR = 0.0f;
    std::vector<float> flangerBufL;
    std::vector<float> flangerBufR;

    // Bode State
    float bodeCarrierPhase = 0.0f;
    float bodeFbL = 0.0f, bodeFbR = 0.0f;
    BodeHilbertPair bodeHilbertL;
    BodeHilbertPair bodeHilbertR;

    // Time Mod State
    int    timeModBufSize = 262144;
    std::vector<float> timeModBufL;
    std::vector<float> timeModBufR;
    int    timeModWritePos = 0;
    float  timeModBarPhase = 0.0f;
    float  timeModPrevOffsetSamples = 0.0f;
    float  timeModLpL = 0.0f, timeModLpR = 0.0f;
    float  timeModHpL = 0.0f, timeModHpR = 0.0f;

    float  limiterEnv = 1.0f;
};

} // namespace PrismorDSP
