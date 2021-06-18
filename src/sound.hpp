#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

// https://mklimenko.github.io/english/2018/06/04/constexpr-random/
namespace cemath::details {
  static constexpr auto two_digits_atoi(char a, char b) {
    constexpr const auto radix = 10;
    auto ia = static_cast<unsigned>(a - '0');
    auto ib = static_cast<unsigned>(b - '0');
    return ia * radix + ib;
  }
}
namespace cemath {
  static constexpr const auto lce_a = 4096;
  static constexpr const auto lce_c = 150889;
  static constexpr const auto lce_m = 714025;

  static constexpr auto seed() {
    constexpr const std::array<char, 9> time { __TIME__ }; // HH:MM:SS
    constexpr const auto a = details::two_digits_atoi(time.at(0), time.at(1)) * 60 * 60;
    constexpr const auto b = details::two_digits_atoi(time.at(3), time.at(4)) * 60;
    constexpr const auto c = details::two_digits_atoi(time.at(6), time.at(7));
    return a + b + c;
  }
  static constexpr auto uniform_distribution(unsigned & prev) {
    return prev = ((lce_a * prev) + lce_c) % lce_m;
  }
  static constexpr auto uniform_distribution_n(unsigned & prev) {
    return static_cast<double>(uniform_distribution(prev)) / lce_m;
  }
  template<class Tp>
  static constexpr auto uniform_distribution_n(unsigned & prev, Tp min, Tp max) {
    return static_cast<Tp>(uniform_distribution_n(prev) * (max - min) + min);
  }
  template<class Tp, auto N>
  static constexpr auto uniform_distribution(Tp min, Tp max) {
    std::array<Tp, N> dst {};
    auto prev = seed();
    std::generate(dst.begin(), dst.end(), [min, max, &prev] {
      return uniform_distribution_n(prev, min, max);
    });
    return dst;
  }
}

namespace sound {
  static constexpr auto apply_volumes(float f, float vol) {
    constexpr const auto master_vol = 0.05F;
    constexpr const auto no_idea_why = 2.0F;
    return f * master_vol * (no_idea_why * vol);
  }
  template<class Tp>
  static constexpr auto norm(Tp f, Tp min, Tp max) noexcept {
    if (f > max) return max;
    if (f < min) return min;
    return f;
  }
  static constexpr auto norm(float f) noexcept {
    return norm(f, -1.0F, 1.0F);
  }
  static auto frac(float f) noexcept {
    float i {};
    return std::modf(f, &i);
  }

  static constexpr float square_duty(float start, float delta, int phase) {
    constexpr const auto min_duty = 0.0F;
    constexpr const auto max_duty = 0.5F;
    const auto fphase = static_cast<float>(phase);
    const auto raw = start + fphase * delta;
    return norm(raw, min_duty, max_duty);
  }

  template<class Tp>
  class envelope {
    Tp m_attack;
    Tp m_sustain;
    Tp m_decay;

  public:
    constexpr envelope() = default;
    constexpr envelope(Tp att, Tp sus, Tp dec) : m_attack(att), m_sustain(sus), m_decay(dec) {
    }

    [[nodiscard]] constexpr float volume(Tp time, float punch) const noexcept {
      if (time < m_attack) {
        auto ftime = static_cast<float>(time);
        return ftime / m_attack;
      }
      time -= m_attack;
      if (time < m_sustain) {
        constexpr const auto punch_scale = 2.0F;
        auto ftime = static_cast<float>(time);
        return 1.0F + (1.0F - ftime / m_sustain) * punch_scale * punch;
      }
      time -= m_sustain;
      if (time < m_decay) {
        auto ftime = static_cast<float>(time);
        return 1.0F - ftime / m_decay;
      }
      return std::numeric_limits<float>::quiet_NaN();
    }
  };

  class phase_shift {
    static constexpr const auto buf_size = 1024;
    std::array<float, buf_size> m_buffer {};

  public:
    [[nodiscard]] constexpr float shift(int cur_time, float phase, float cur_sample) noexcept {
      int ipp = cur_time % buf_size;
      m_buffer.at(ipp) = cur_sample;

      auto i_phase = static_cast<int>(phase);
      auto abs_phase = i_phase < 0 ? -i_phase : i_phase;

      int iphase = norm(abs_phase, 0, buf_size - 1);
      auto phased_sample = m_buffer.at((ipp - iphase + buf_size) % buf_size);
      return cur_sample + phased_sample;
    }
  };
}

namespace sound::waveform {
  using fn_t = float (*)(float, float);

  static auto square(float phase, float duty) {
    constexpr const auto amplitude = 0.5F;
    return (frac(phase) < duty) ? amplitude : -amplitude;
  }
  static auto sawtooth(float phase, float /*duty*/) {
    return 1.0F - frac(phase) * 2;
  }
  static auto sine(float phase, float /*duty*/) {
    constexpr const auto pi = 3.14159265358979323F;
    constexpr const auto two_times_pi = 2.0F * pi;
    return std::sinf(phase * two_times_pi);
  }
  static auto noise(float /*phase*/, float /*duty*/) {
    static auto seed = cemath::seed();
    return cemath::uniform_distribution_n(seed, -1.0F, 1.0F);
  }
}
