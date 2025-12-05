#include "Biquad.h"

namespace analog::sat {

namespace {
constexpr double kPi = 3.1415926535897932384626433832795;
}

void Biquad::setLowpass(double sampleRate, double cutoffHz, double q) {
    setCoefficients(sampleRate, cutoffHz, q, false);
}

void Biquad::setHighpass(double sampleRate, double cutoffHz, double q) {
    setCoefficients(sampleRate, cutoffHz, q, true);
}

void Biquad::setCoefficients(double sampleRate, double cutoffHz, double q, bool highpass) {
    const double omega = 2.0 * kPi * cutoffHz / sampleRate;
    const double sinOmega = std::sin(omega);
    const double cosOmega = std::cos(omega);
    const double alpha = sinOmega / (2.0 * q);

    double b0 = 0.0;
    double b1 = 0.0;
    double b2 = 0.0;

    if (highpass) {
        b0 = (1.0 + cosOmega) * 0.5;
        b1 = -(1.0 + cosOmega);
        b2 = (1.0 + cosOmega) * 0.5;
    } else {
        b0 = (1.0 - cosOmega) * 0.5;
        b1 = 1.0 - cosOmega;
        b2 = (1.0 - cosOmega) * 0.5;
    }

    const double a0 = 1.0 + alpha;
    a1_ = -2.0 * cosOmega / a0;
    a2_ = (1.0 - alpha) / a0;
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
}

}  // namespace analog::sat
