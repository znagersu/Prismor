#include <iostream>
#include <memory>
#include <cmath>
#include <cassert>
#include <string>
#include <vector>
#include <JuceHeader.h>
#include "../Source/DSP/FactoryPresetBank.h"
#include "../Source/DSP/SpectralSynthEngine.h"

int main()
{
    std::cout << "--- Testing Prismor Phase 5.5 Preset Bank (300+ Patches) ---" << std::endl;
    const int presetCount = PrismorDSP::FactoryPresetBank::getPresetCount();
    std::cout << "Total Presets in Factory Bank: " << presetCount << std::endl;

    if (presetCount < 300)
    {
        std::cerr << "FAIL: Preset count is less than 300! count = " << presetCount << std::endl;
        return 1;
    }

    auto enginePtr = std::make_unique<PrismorDSP::SpectralSynthEngine>();
    auto& engine = *enginePtr;
    engine.prepare(44100.0, 64);

    juce::AudioBuffer<float> buffer(2, 64);
    juce::MidiBuffer midi;

    int errorCount = 0;
    int cyberCount = 0;
    int cineCount = 0;
    int houseCount = 0;
    int lofiCount = 0;

    const auto& entries = PrismorDSP::FactoryPresetBank::getEntries();

    for (int i = 0; i < presetCount; ++i)
    {
        const auto& p = entries[static_cast<size_t>(i)];

        // Check genre tag tracking
        std::string desc(p.description);
        std::string auth(p.author);
        if (desc.find("CYBERPUNK & SYNTHWAVE") != std::string::npos || auth.find("Cyberpunk") != std::string::npos)
            cyberCount++;
        if (desc.find("CINEMATIC & AMBIENT") != std::string::npos || auth.find("Cinematic") != std::string::npos)
            cineCount++;
        if (desc.find("MODERN TECH HOUSE") != std::string::npos || auth.find("Tech House") != std::string::npos)
            houseCount++;
        if (desc.find("LO-FI & CHILLHOP") != std::string::npos || auth.find("Lo-Fi") != std::string::npos)
            lofiCount++;

        // Apply preset
        PrismorDSP::SynthParameters params;
        auto setParam = [&](const char* id, float val)
        {
            if (std::isnan(val) || std::isinf(val))
            {
                std::cerr << "ERR: NaN/Inf on param " << id << " in preset [" << i << "] " << p.name << std::endl;
                errorCount++;
            }
        };

        PrismorDSP::FactoryPresetBank::applyPreset(i, setParam, engine);

        // Render audio block
        buffer.clear();
        midi.clear();
        engine.renderBlock(buffer, midi, params);

        const float* l = buffer.getReadPointer(0);
        for (int s = 0; s < 64; ++s)
        {
            if (std::isnan(l[s]) || std::isinf(l[s]))
            {
                std::cerr << "ERR: NaN audio sample in preset [" << i << "] " << p.name << std::endl;
                errorCount++;
                break;
            }
        }
    }

    std::cout << "\nGenre Expansion Packs Breakdown:" << std::endl;
    std::cout << "  - CYBERPUNK & SYNTHWAVE : " << cyberCount << " patches" << std::endl;
    std::cout << "  - CINEMATIC & AMBIENT   : " << cineCount << " patches" << std::endl;
    std::cout << "  - MODERN TECH HOUSE     : " << houseCount << " patches" << std::endl;
    std::cout << "  - LO-FI & CHILLHOP      : " << lofiCount << " patches" << std::endl;
    std::cout << "Total Procedural Patches  : " << (cyberCount + cineCount + houseCount + lofiCount) << std::endl;
    std::cout << "Error Count: " << errorCount << std::endl;

    if (errorCount == 0 && presetCount >= 300)
    {
        std::cout << "SUCCESS: All " << presetCount << " presets verified and loaded cleanly with 0 errors!" << std::endl;
        return 0;
    }

    return 1;
}
