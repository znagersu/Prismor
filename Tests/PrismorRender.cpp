#include <JuceHeader.h>
#include "../Source/PluginProcessor.h"
#include <iostream>
#include <vector>
#include <algorithm>

namespace
{

void setParamPlain(PrismorAudioProcessor& proc, const juce::String& paramId, float plainValue)
{
    if (auto* param = proc.getAPVTS().getParameter(paramId))
        param->setValueNotifyingHost(param->convertTo0to1(plainValue));
}

bool writeWav(const juce::File& outFile, const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    outFile.getParentDirectory().createDirectory();
    if (outFile.existsAsFile())
        outFile.deleteFile();

    std::unique_ptr<juce::FileOutputStream> stream(outFile.createOutputStream());
    if (stream == nullptr || !stream->openedOk())
    {
        std::cerr << "Failed to open output stream for: " << outFile.getFullPathName() << "\n";
        return false;
    }

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(stream.get(), sampleRate, static_cast<unsigned int>(buffer.getNumChannels()), 24, {}, 0));
    if (writer == nullptr)
    {
        std::cerr << "Failed to create WAV writer for: " << outFile.getFullPathName() << "\n";
        return false;
    }
    stream.release();
    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

void renderBuffer(PrismorAudioProcessor& proc,
                  juce::AudioBuffer<float>& outBuf,
                  const juce::MidiBuffer& midiEvents,
                  int totalSamples,
                  int blockSize = 512)
{
    outBuf.setSize(2, totalSamples, false, true, true);
    outBuf.clear();

    juce::AudioBuffer<float> block(2, blockSize);
    juce::MidiBuffer blockMidi;

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const int n = std::min(blockSize, totalSamples - offset);
        block.setSize(2, n, false, false, true);
        block.clear();
        blockMidi.clear();

        for (const auto meta : midiEvents)
        {
            if (meta.samplePosition >= offset && meta.samplePosition < offset + n)
                blockMidi.addEvent(meta.getMessage(), meta.samplePosition - offset);
        }

        proc.processBlock(block, blockMidi);
        for (int ch = 0; ch < 2; ++ch)
            outBuf.copyFrom(ch, offset, block, ch, 0, n);
    }
}

void renderAutomated(PrismorAudioProcessor& proc,
                     juce::AudioBuffer<float>& outBuf,
                     int totalSamples,
                     std::function<void(int sampleOffset, juce::MidiBuffer&)> onBlock,
                     int blockSize = 512)
{
    outBuf.setSize(2, totalSamples, false, true, true);
    outBuf.clear();

    juce::AudioBuffer<float> block(2, blockSize);
    juce::MidiBuffer blockMidi;

    for (int offset = 0; offset < totalSamples; offset += blockSize)
    {
        const int n = std::min(blockSize, totalSamples - offset);
        block.setSize(2, n, false, false, true);
        block.clear();
        blockMidi.clear();

        onBlock(offset, blockMidi);
        proc.processBlock(block, blockMidi);

        for (int ch = 0; ch < 2; ++ch)
            outBuf.copyFrom(ch, offset, block, ch, 0, n);
    }
}

} // namespace

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;
    const double sr = 48000.0;

    juce::File outDir = (argc > 1)
        ? juce::File::getCurrentWorkingDirectory().getChildFile(argv[1])
        : juce::File::getCurrentWorkingDirectory().getChildFile("render_out");
    outDir.createDirectory();
    std::cout << "Rendering Prismor WAV inspection suite to: " << outDir.getFullPathName() << "\n";

    // 1. clean_saw.wav
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(0);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 48, 0.0f), 36000);
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 40000);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 84000);
        juce::AudioBuffer<float> buf;
        renderBuffer(proc, buf, midi, 96000);
        writeWav(outDir.getChildFile("clean_saw.wav"), buf, sr);
    }

    // 2. poly_chord_p39.wav
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(39);
        juce::MidiBuffer midi;
        const int notes[] = { 48, 52, 55, 59, 62, 64, 67, 72 };
        for (int i = 0; i < 8; ++i)
        {
            midi.addEvent(juce::MidiMessage::noteOn(1, notes[i], 0.85f), i * 1200);
            midi.addEvent(juce::MidiMessage::noteOff(1, notes[i], 0.0f), 96000);
        }
        juce::AudioBuffer<float> buf;
        renderBuffer(proc, buf, midi, 120000);
        writeWav(outDir.getChildFile("poly_chord_p39.wav"), buf, sr);
    }

    // 3. distortion_4x_sweep.wav (High Saw note sweeping drive 0 -> 1 through 4x oversampler)
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(0);
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Distortion) });
        setParamPlain(proc, "filter1Enable", 0.0f);
        setParamPlain(proc, "distEnable",    1.0f);
        setParamPlain(proc, "distType",      5.0f); // Asym Scream
        setParamPlain(proc, "distTone",      0.85f);
        setParamPlain(proc, "distMix",       1.0f);
        setParamPlain(proc, "limiterEnable", 1.0f);

        bool noteStarted = false;
        const int total = 96000;
        juce::AudioBuffer<float> buf;
        renderAutomated(proc, buf, total, [&](int offset, juce::MidiBuffer& m) {
            if (!noteStarted)
            {
                m.addEvent(juce::MidiMessage::noteOn(1, 72, 0.95f), 0); // C5
                noteStarted = true;
            }
            if (offset == 76800)
                m.addEvent(juce::MidiMessage::noteOff(1, 72, 0.0f), 0);
            const float t = juce::jlimit(0.0f, 1.0f, static_cast<float>(offset) / 72000.0f);
            setParamPlain(proc, "distDrive", t);
        });
        writeWav(outDir.getChildFile("distortion_4x_sweep.wav"), buf, sr);
    }

    // 4. bode_shifter.wav (Sweep Shift Hz -1200 -> +1200 Hz on 880 Hz Sine + Saw blend)
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(0);
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Bode) });
        proc.getSynthEngine().getResynthesisEngine().resetPartTableToWaveform(0, 6); // Sine
        setParamPlain(proc, "filter1Enable", 0.0f);
        setParamPlain(proc, "bodeEnable",    1.0f);
        setParamPlain(proc, "bodeFeedback",  0.35f);
        setParamPlain(proc, "bodeMix",       0.85f);
        setParamPlain(proc, "limiterEnable", 1.0f);

        bool started = false;
        const int total = 96000;
        juce::AudioBuffer<float> buf;
        renderAutomated(proc, buf, total, [&](int offset, juce::MidiBuffer& m) {
            if (!started)
            {
                m.addEvent(juce::MidiMessage::noteOn(1, 69, 0.9f), 0); // A4 440 Hz
                started = true;
            }
            if (offset == 76800)
                m.addEvent(juce::MidiMessage::noteOff(1, 69, 0.0f), 0);
            const float t = juce::jlimit(0.0f, 1.0f, static_cast<float>(offset) / 72000.0f);
            setParamPlain(proc, "bodeShift", -1200.0f + t * 2400.0f);
        });
        writeWav(outDir.getChildFile("bode_shifter.wav"), buf, sr);
    }

    // 5. shimmer_reverb.wav
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(0);
        proc.getSynthEngine().setActiveFxChain({ static_cast<int>(PrismorDSP::FxModuleId::Reverb) });
        setParamPlain(proc, "reverbEnable",  1.0f);
        setParamPlain(proc, "reverbSize",    0.78f);
        setParamPlain(proc, "reverbDecay",   0.85f);
        setParamPlain(proc, "reverbShimmer", 0.85f);
        setParamPlain(proc, "reverbMix",     0.65f);
        setParamPlain(proc, "limiterEnable", 1.0f);

        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.9f), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 67, 0.9f), 0);
        midi.addEvent(juce::MidiMessage::noteOff(1, 60, 0.0f), 40000);
        midi.addEvent(juce::MidiMessage::noteOff(1, 67, 0.0f), 40000);
        juce::AudioBuffer<float> buf;
        renderBuffer(proc, buf, midi, 120000);
        writeWav(outDir.getChildFile("shimmer_reverb.wav"), buf, sr);
    }

    // 6. prism_inharmonic.wav (Sweep Prism Hz Shift + harmonicScale)
    {
        PrismorAudioProcessor proc;
        proc.prepareToPlay(sr, 512);
        proc.loadFactoryPreset(0);
        proc.getSynthEngine().setActiveFxChain({});
        setParamPlain(proc, "filter1Enable", 0.0f);
        setParamPlain(proc, "prismEnable",   1.0f);
        setParamPlain(proc, "prismMode",     1.0f); // Hz Shift
        setParamPlain(proc, "limiterEnable", 1.0f);

        bool started = false;
        const int total = 96000;
        juce::AudioBuffer<float> buf;
        renderAutomated(proc, buf, total, [&](int offset, juce::MidiBuffer& m) {
            if (!started)
            {
                m.addEvent(juce::MidiMessage::noteOn(1, 57, 0.9f), 0); // A3
                started = true;
            }
            if (offset == 76800)
                m.addEvent(juce::MidiMessage::noteOff(1, 57, 0.0f), 0);
            const float t = juce::jlimit(0.0f, 1.0f, static_cast<float>(offset) / 72000.0f);
            setParamPlain(proc, "prismAmount", t);
            setParamPlain(proc, "harmonicScale", 1.0f + t * 0.8f);
        });
        writeWav(outDir.getChildFile("prism_inharmonic.wav"), buf, sr);
    }

    std::cout << "All 6 inspection WAV files rendered cleanly.\n";
    return 0;
}
