#pragma once

#include "parameters.hpp"

#include <fstream>
#include <optional>

extern int wave_type;
extern float sound_vol;

struct settings {
  parameters m_p = p;
  int m_wave_type = wave_type;
  float m_sound_vol = sound_vol;
};

// TODO: add support to SFXR files
class file_format_ec0000 {
  static constexpr const int version = 0xec0000;

  int v = version;
  settings s {};

  [[nodiscard]] explicit constexpr operator bool() const noexcept {
    return v == version;
  }

public:
  [[nodiscard]] static std::optional<settings> load(const char * filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file) return {};

    file_format_ec0000 data {};
    file.read(reinterpret_cast<char *>(&data), sizeof(data)); // NOLINT
    if (!data) return {};

    return { data.s };
  }
  static void save(const char * filename, const settings & s) {
    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file) return;

    file_format_ec0000 data;
    data.s = s;
    file.write(reinterpret_cast<char *>(&data), sizeof(data)); // NOLINT
  }
};
