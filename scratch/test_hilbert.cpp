#include <iostream>
#include <iomanip>
#include <cmath>

struct AllpassSection
{
    float c = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;

    void reset() { x1 = x2 = y1 = y2 = 0.0f; }

    inline float process(float x) noexcept
    {
        const float y = c * (x + y2) - x2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

struct HilbertTransformOlli
{
    AllpassSection path1[4];
    AllpassSection path2[4];
    float delay1 = 0.0f;

    void init()
    {
        const float c1[4] = { 0.4794411160829620f, 0.8762435898950486f, 0.9766029691687166f, 0.9974994041220338f };
        const float c2[4] = { 0.1617774170636317f, 0.7330669013033557f, 0.9453630196680628f, 0.9906005141670404f };

        for (int i = 0; i < 4; ++i)
        {
            path1[i].c = c1[i];
            path1[i].reset();
            path2[i].c = c2[i];
            path2[i].reset();
        }
        delay1 = 0.0f;
    }

    inline void process(float x, float& outI, float& outQ) noexcept
    {
        float y1 = x;
        for (int i = 0; i < 4; ++i) y1 = path1[i].process(y1);
        outI = delay1;
        delay1 = y1;

        float y2 = x;
        for (int i = 0; i < 4; ++i) y2 = path2[i].process(y2);
        outQ = y2;
    }
};

int main()
{
    const float sampleRate = 44100.0f;
    HilbertTransformOlli ht;
    ht.init();

    // Test Downshift: 1000 Hz input, shift = -200 Hz -> Output should be 800 Hz, with 1200 Hz suppressed!
    const float inFreq = 1000.0f;
    const float shiftHz = -200.0f;
    const float wIn = 2.0f * 3.14159265f * inFreq / sampleRate;
    const float wShift = 2.0f * 3.14159265f * shiftHz / sampleRate;
    float carrierPhase = 0.0f;

    float power800 = 0.0f;
    float power1200 = 0.0f;
    const int N = 20000;

    for (int n = 0; n < N; ++n)
    {
        float in = std::sin(wIn * n);
        float I, Q;
        ht.process(in, I, Q);

        carrierPhase += wShift;
        if (carrierPhase > 3.14159265f) carrierPhase -= 2.0f * 3.14159265f;
        if (carrierPhase < -3.14159265f) carrierPhase += 2.0f * 3.14159265f;

        const float cCos = std::cos(carrierPhase);
        const float cSin = std::sin(carrierPhase);

        // General formula: I * cos - Q * sin
        float yDown = I * cCos - Q * cSin;

        if (n > 10000)
        {
            const float ref800 = std::sin(2.0f * 3.14159265f * 800.0f * n / sampleRate);
            const float ref1200 = std::sin(2.0f * 3.14159265f * 1200.0f * n / sampleRate);
            power800 += yDown * ref800;
            power1200 += yDown * ref1200;
        }
    }

    float suppressionDb = 20.0f * std::log10(std::max(1e-6f, std::abs(power800) / std::max(1e-6f, std::abs(power1200))));
    std::cout << "Downshift sideband suppression at 1 kHz input (-200 Hz shift: 800 Hz desired vs 1200 Hz rejected): " 
              << suppressionDb << " dB\n";

    return 0;
}
