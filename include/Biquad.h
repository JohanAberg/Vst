#pragma once

#include <cmath>

namespace analog::sat {

class Biquad {
   public:
    void setLowpass(double sampleRate, double cutoffHz, double q = 0.707);
    void setHighpass(double sampleRate, double cutoffHz, double q = 0.707);
    double process(double input) {
        const double out = b0_ * input + b1_ * z1_ + b2_ * z2_ - a1_ * y1_ - a2_ * y2_;
        z2_ = z1_;
        z1_ = input;
        y2_ = y1_;
        y1_ = out;
        return out;
    }
    void reset() {
        z1_ = z2_ = y1_ = y2_ = 0.0;
    }

   private:
    void setCoefficients(double sampleRate, double cutoffHz, double q, bool highpass);

    double b0_ = 1.0;
    double b1_ = 0.0;
    double b2_ = 0.0;
    double a1_ = 0.0;
    double a2_ = 0.0;
    double z1_ = 0.0;
    double z2_ = 0.0;
    double y1_ = 0.0;
    double y2_ = 0.0;
};

}  // namespace analog::sat
