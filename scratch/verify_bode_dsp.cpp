#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <iomanip>

struct BodeAllpassSection
{
    float c = 0.0f;
    float x1 = 0.0f, x2 = 0.0f;
    float y1 = 0.0f, y2 = 0.0f;

    void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }

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

struct BodeHilbertPair
{
    BodeAllpassSection path1[4];
    BodeAllpassSection path2[4];
    float delay1 = 0.0f;

    BodeHilbertPair() noexcept { reset(); }

    void reset() noexcept
    {
        const float c1[4] = {
            0.4794411160829620f,
            0.8762435898950486f,
            0.9766029691687166f,
            0.9974994041220338f
        };
        const float c2[4] = {
            0.1617774170636317f,
            0.7330669013033557f,
            0.9453630196680628f,
            0.9906005141670404f
        };

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
        for (int i = 0; i < 4; ++i)
            y1 = path1[i].process(y1);
        outI = delay1;
        delay1 = y1;

        float y2 = x;
        for (int i = 0; i < 4; ++i)
            y2 = path2[i].process(y2);
        outQ = y2;
    }
};

struct BodeEngine
{
    BodeHilbertPair hilbertL;
    BodeHilbertPair hilbertR;
    float carrierPhase = 0.0f;
    float fbL = 0.0f;
    float fbR = 0.0f;
    double sampleRate = 44100.0;

    void reset(double sr)
    {
        sampleRate = sr;
        carrierPhase = 0.0f;
        fbL = fbR = 0.0f;
        hilbertL.reset();
        hilbertR.reset();
    }

    void process(float* outL, float* outR, int numSamples, float shiftHz, float mix, float feedback)
    {
        const float invSampleRate = 1.0f / static_cast<float>(sampleRate);
        const float shift = std::max(-5000.0f, std::min(5000.0f, shiftHz));
        const float m = std::max(0.0f, std::min(1.0f, mix));
        const float fb = std::max(-0.88f, std::min(0.88f, feedback));
        const float twoPi = 6.28318530717958647692f;
        const float phaseInc = (shift * invSampleRate) * twoPi;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = outL[i];
            float inR = (outR != nullptr) ? outR[i] : inL;
            if (std::isnan(inL) || std::isinf(inL)) inL = 0.0f;
            if (std::isnan(inR) || std::isinf(inR)) inR = 0.0f;

            const float fbInL = inL + std::tanh(fbL * 0.85f) * fb;
            const float fbInR = inR + std::tanh(fbR * 0.85f) * fb;

            carrierPhase += phaseInc;
            if (carrierPhase > 3.14159265358979323846f)
                carrierPhase -= twoPi;
            else if (carrierPhase < -3.14159265358979323846f)
                carrierPhase += twoPi;

            const float cCos = std::cos(carrierPhase);
            const float cSin = std::sin(carrierPhase);

            float iL = 0.0f, qL = 0.0f;
            float iR = 0.0f, qR = 0.0f;
            hilbertL.process(fbInL, iL, qL);
            hilbertR.process(fbInR, iR, qR);

            float wetL = iL * cCos + qL * cSin;
            float wetR = iR * cCos + qR * cSin;

            if (std::isnan(wetL) || std::isinf(wetL)) wetL = 0.0f;
            if (std::isnan(wetR) || std::isinf(wetR)) wetR = 0.0f;

            wetL = std::tanh(wetL);
            wetR = std::tanh(wetR);

            fbL = wetL;
            fbR = wetR;

            outL[i] = inL * (1.0f - m) + wetL * m;
            if (outR != nullptr)
                outR[i] = inR * (1.0f - m) + wetR * m;
        }
    }
};

int main()
{
    std::cout << "=== BODE FREQUENCY SHIFTER DSP VERIFICATION ===" << std::endl;
    const double sr = 44100.0;
    BodeEngine bode;
    bode.reset(sr);

    // Test 1: Pure Tone 1000 Hz shifted by +250 Hz -> Expected 1250 Hz (SSB)
    const int N = 44100; // 1 second
    std::vector<float> bufL(N, 0.0f);
    std::vector<float> bufR(N, 0.0f);

    const float f_in = 1000.0f;
    const float twoPi = 6.28318530717958647692f;
    for (int i = 0; i < N; ++i)
    {
        float val = std::sin(twoPi * f_in * (static_cast<float>(i) / static_cast<float>(sr)));
        bufL[i] = val;
        bufR[i] = val;
    }

    const float shift = 250.0f;
    bode.process(bufL.data(), bufR.data(), N, shift, 1.0f, 0.0f);

    // Discrete Fourier analysis around 1250 Hz (desired) vs 750 Hz (unwanted sideband)
    const float f_desired = 1250.0f;
    const float f_unwanted = 750.0f;
    double re_des = 0.0, im_des = 0.0;
    double re_unw = 0.0, im_unw = 0.0;
    const int startN = 22050; // analyze second half after transient settling
    const int countN = N - startN;

    for (int i = startN; i < N; ++i)
    {
        double t = static_cast<double>(i) / sr;
        double w_d = 2.0 * M_PI * f_desired * t;
        double w_u = 2.0 * M_PI * f_unwanted * t;
        double s = bufL[i];
        re_des += s * std::cos(w_d);
        im_des -= s * std::sin(w_d);
        re_unw += s * std::cos(w_u);
        im_unw -= s * std::sin(w_u);
    }
    double mag_des = std::sqrt(re_des * re_des + im_des * im_des) / countN * 2.0;
    double mag_unw = std::sqrt(re_unw * re_unw + im_unw * im_unw) / countN * 2.0;
    double suppressionDb = 20.0 * std::log10(mag_unw / (mag_des + 1e-12));

    std::cout << "[Test 1: Upshift +250 Hz on 1000 Hz Sine]" << std::endl;
    std::cout << "  Desired 1250 Hz Amplitude:  " << mag_des << std::endl;
    std::cout << "  Unwanted 750 Hz Amplitude:  " << mag_unw << std::endl;
    std::cout << "  Sideband Suppression:       " << suppressionDb << " dB (Target: < -40 dB)" << std::endl;
    assert(suppressionDb < -40.0);
    std::cout << "  -> PASS: Single-sideband upshift verified cleanly!" << std::endl;

    // Test 2: Downshift -300 Hz on 1000 Hz Sine -> Expected 700 Hz (SSB)
    bode.reset(sr);
    for (int i = 0; i < N; ++i)
    {
        float val = std::sin(twoPi * f_in * (static_cast<float>(i) / static_cast<float>(sr)));
        bufL[i] = val;
        bufR[i] = val;
    }
    bode.process(bufL.data(), bufR.data(), N, -300.0f, 1.0f, 0.0f);

    const float f_down_desired = 700.0f;
    const float f_down_unwanted = 1300.0f;
    re_des = 0.0; im_des = 0.0;
    re_unw = 0.0; im_unw = 0.0;
    for (int i = startN; i < N; ++i)
    {
        double t = static_cast<double>(i) / sr;
        double w_d = 2.0 * M_PI * f_down_desired * t;
        double w_u = 2.0 * M_PI * f_down_unwanted * t;
        double s = bufL[i];
        re_des += s * std::cos(w_d);
        im_des -= s * std::sin(w_d);
        re_unw += s * std::cos(w_u);
        im_unw -= s * std::sin(w_u);
    }
    mag_des = std::sqrt(re_des * re_des + im_des * im_des) / countN * 2.0;
    mag_unw = std::sqrt(re_unw * re_unw + im_unw * im_unw) / countN * 2.0;
    suppressionDb = 20.0 * std::log10(mag_unw / (mag_des + 1e-12));

    std::cout << "\n[Test 2: Downshift -300 Hz on 1000 Hz Sine]" << std::endl;
    std::cout << "  Desired 700 Hz Amplitude:   " << mag_des << std::endl;
    std::cout << "  Unwanted 1300 Hz Amplitude: " << mag_unw << std::endl;
    std::cout << "  Sideband Suppression:       " << suppressionDb << " dB (Target: < -40 dB)" << std::endl;
    assert(suppressionDb < -40.0);
    std::cout << "  -> PASS: Single-sideband downshift verified cleanly!" << std::endl;

    // Test 3: Sawtooth rich harmonics inharmonicity test
    // Sawtooth 200 Hz: Harmonics at 200, 400, 600, 800, 1000 Hz
    // Shifted +75 Hz: Frequencies become 275, 475, 675, 875, 1075 Hz
    // Ratios: 475/275 = 1.727 != 2.0 (Inharmonic / Metallic ring!)
    bode.reset(sr);
    for (int i = 0; i < N; ++i)
    {
        float phase = std::fmod(static_cast<float>(i) * 200.0f / static_cast<float>(sr), 1.0f);
        float saw = 2.0f * phase - 1.0f;
        bufL[i] = saw * 0.5f;
        bufR[i] = saw * 0.5f;
    }
    bode.process(bufL.data(), bufR.data(), N, 75.0f, 1.0f, 0.45f); // with feedback

    bool hasNaN = false;
    float maxVal = 0.0f;
    for (int i = 0; i < N; ++i)
    {
        if (std::isnan(bufL[i]) || std::isinf(bufL[i]) || std::isnan(bufR[i]) || std::isinf(bufR[i]))
            hasNaN = true;
        maxVal = std::max(maxVal, std::abs(bufL[i]));
    }
    std::cout << "\n[Test 3: Metallic Inharmonic Sawtooth with Feedback (fb=0.45)]" << std::endl;
    std::cout << "  Peak Amplitude: " << maxVal << " (Soft bounded by tanh)" << std::endl;
    std::cout << "  Has NaN/Inf:    " << (hasNaN ? "YES (FAIL)" : "NO (CLEAN)") << std::endl;
    assert(!hasNaN);
    assert(maxVal <= 1.05f);
    std::cout << "  -> PASS: Complex inharmonic timbre stable without clipping or NaNs!" << std::endl;

    std::cout << "\nALL 3 BODE DSP UNIT TESTS COMPLETED SUCCESSFULLY!" << std::endl;
    return 0;
}
