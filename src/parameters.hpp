#pragma once

struct parameters {
  float m_base_freq { 0.3f };
  float m_freq_limit {};
  float m_freq_ramp {};
  float m_freq_dramp {};
  float m_duty {};
  float m_duty_ramp {};

  float m_vib_strength {};
  float m_vib_speed {};
  float m_vib_delay {};

  float m_env_attack {};
  float m_env_sustain { 0.3f };
  float m_env_decay { 0.4f };
  float m_env_punch {};

  float m_lpf_resonance {};
  float m_lpf_freq { 1.0f };
  float m_lpf_ramp {};
  float m_hpf_freq {};
  float m_hpf_ramp {};

  float m_pha_offset {};
  float m_pha_ramp {};

  float m_repeat_speed {};

  float m_arp_speed {};
  float m_arp_mod {};
};
extern parameters p;
