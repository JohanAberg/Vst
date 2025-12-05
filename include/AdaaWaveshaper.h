#pragma once

#include <cmath>

namespace analog::sat {

class AdaTanhStage {
   public:
    void setGain(double g) { gain_ = g; }
    void reset() {
        prev_ = 0.0;
    }

    double process(double input) {
        const double scaled = gain_ * input;
        const double diff = scaled - prev_;
        double y = 0.0;
        if (std::abs(diff) > 1e-10) {
            y = (antiderivative(scaled) - antiderivative(prev_)) / diff;
        } else {
            y = nonlinearity(scaled);
        }
        prev_ = scaled;
        return y;
    }

   private:
    static double nonlinearity(double x) { return std::tanh(x); }
    static double antiderivative(double x) { return std::log(std::cosh(x)); }

    double gain_ = 1.0;
    double prev_ = 0.0;
};

class AdaSoftClipStage {
   public:
    void setDrive(double drive) { drive_ = drive; }
    void reset() {
        prev_ = 0.0;
    }

    double process(double input) {
        const double x = drive_ * input;
        const double diff = x - prev_;
        double y = 0.0;
        if (std::abs(diff) > 1e-9) {
            y = (antiderivative(x) - antiderivative(prev_)) / diff;
        } else {
            y = nonlinearity(x);
        }
        prev_ = x;
        return y;
    }

   private:
    static double nonlinearity(double x) {
        return x - (x * x * x) * 0.3333333333333;
    }
    static double antiderivative(double x) {
        return 0.5 * x * x - 0.25 * x * x * x * x;
    }

    double drive_ = 1.0;
    double prev_ = 0.0;
};

class HysteresisStage {
   public:
    void setReactance(double value) { reactance_ = value; }
    void reset() {
        state_ = 0.0;
        flux_ = 0.0;
    }

    double process(double input) {
        const double alpha = 0.6 + reactance_ * 0.35;
        const double beta = 0.25 + reactance_ * 0.5;
        state_ = state_ + alpha * (input - state_);
        flux_ = beta * state_ + (1.0 - beta) * flux_;
        return std::tanh(flux_);
    }

   private:
    double reactance_ = 0.5;
    double state_ = 0.0;
    double flux_ = 0.0;
};

class EnvelopeFollower {
   public:
    void setCoefficients(double attackMs, double releaseMs, double sampleRate);
    double process(double input);
    void reset() { value_ = 0.0; }

   private:
    double attackCoef_ = 0.0;
    double releaseCoef_ = 0.0;
    double value_ = 0.0;
};

}  // namespace analog::sat
