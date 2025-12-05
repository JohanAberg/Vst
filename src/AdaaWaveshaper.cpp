#include "AdaaWaveshaper.h"

#include <algorithm>

namespace analog::sat {

void EnvelopeFollower::setCoefficients(double attackMs, double releaseMs, double sampleRate) {
    const double attackSeconds = attackMs * 0.001;
    const double releaseSeconds = releaseMs * 0.001;
    attackCoef_ = std::exp(-1.0 / std::max(attackSeconds * sampleRate, 1e-9));
    releaseCoef_ = std::exp(-1.0 / std::max(releaseSeconds * sampleRate, 1e-9));
}

double EnvelopeFollower::process(double input) {
    const double rectified = std::abs(input);
    if (rectified > value_) {
        value_ = attackCoef_ * (value_ - rectified) + rectified;
    } else {
        value_ = releaseCoef_ * (value_ - rectified) + rectified;
    }
    return value_;
}

}  // namespace analog::sat
