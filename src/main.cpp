/*

   Porting: Search for WIN32 to find windows-specific code
            snippets which need to be replaced or removed.

*/

#include "draw_context.hpp"
#include "gui.hpp"
#include "m4c0.hpp"
#include "m4c0/log.hpp"
#include "m4c0/native_handles.hpp"
#include "parameters.hpp"
#include "settings.hpp"
#include "sound.hpp"
#include "sprite_set.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#define rnd(n) (rand() % (n + 1))

#define PI 3.14159265f

float frnd(float range) {
  return (float)rnd(10000) / 10000 * range;
}

parameters p;

int wave_type;

float sound_vol = 0.5f;

bool playing_sample = false;
double fperiod;
double fmaxperiod;
double fslide;
double fdslide;
int period;
float square_duty;
float square_slide;
float fphase;
float fdphase;
float fltp;
float fltdp;
float fltw;
float fltw_d;
float fltdmp;
float fltphp;
float flthp;
float flthp_d;
float vib_phase;
float vib_speed;
float vib_amp;
int rep_time;
int rep_limit;
int arp_time;
int arp_limit;
double arp_mod;

static sound::envelope<int> g_env;       // NOLINT
static sound::phase_shift g_phs;         // NOLINT
static int g_sample_phase;               // NOLINT
static int g_supersample_phase;          // NOLINT
static sound::waveform::fn_t g_waveform; // NOLINT

float * vselected = NULL;
int vcurbutton = -1;

int wav_bits = 16;
int wav_freq = 44100;

void ResetParams() {
  wave_type = 0;
  p = {};
}

static constexpr auto penv_to_int(float penv) {
  constexpr const auto scale = 100000.0F;
  return static_cast<int>(penv * penv * scale);
}

void ResetSample(bool restart) {
  fperiod = 100.0 / (p.m_base_freq * p.m_base_freq + 0.001);
  period = (int)fperiod;
  fmaxperiod = 100.0 / (p.m_freq_limit * p.m_freq_limit + 0.001);
  fslide = 1.0 - pow((double)p.m_freq_ramp, 3.0) * 0.01;
  fdslide = -pow((double)p.m_freq_dramp, 3.0) * 0.000001;
  square_duty = 0.5f - p.m_duty * 0.5f;
  square_slide = -p.m_duty_ramp * 0.00005f;
  if (p.m_arp_mod >= 0.0f)
    arp_mod = 1.0 - pow((double)p.m_arp_mod, 2.0) * 0.9;
  else
    arp_mod = 1.0 + pow((double)p.m_arp_mod, 2.0) * 10.0;
  arp_time = 0;
  arp_limit = (int)(pow(1.0f - p.m_arp_speed, 2.0f) * 20000 + 32);
  if (p.m_arp_speed == 1.0f) arp_limit = 0;
  if (!restart) {
    g_sample_phase = 0;
    g_supersample_phase = 0;
    // reset filter
    fltp = 0.0f;
    fltdp = 0.0f;
    fltw = pow(p.m_lpf_freq, 3.0f) * 0.1f;
    fltw_d = 1.0f + p.m_lpf_ramp * 0.0001f;
    fltdmp = 5.0f / (1.0f + pow(p.m_lpf_resonance, 2.0f) * 20.0f) * (0.01f + fltw);
    if (fltdmp > 0.8f) fltdmp = 0.8f;
    fltphp = 0.0f;
    flthp = pow(p.m_hpf_freq, 2.0f) * 0.1f;
    flthp_d = 1.0 + p.m_hpf_ramp * 0.0003f;
    // reset vibrato
    vib_phase = 0.0f;
    vib_speed = pow(p.m_vib_speed, 2.0f) * 0.01f;
    vib_amp = p.m_vib_strength * 0.5f;
    // reset envelope
    g_env = { penv_to_int(p.m_env_attack), penv_to_int(p.m_env_sustain), penv_to_int(p.m_env_decay) };

    fphase = pow(p.m_pha_offset, 2.0f) * 1020.0f;
    if (p.m_pha_offset < 0.0f) fphase = -fphase;
    fdphase = pow(p.m_pha_ramp, 2.0f) * 1.0f;
    if (p.m_pha_ramp < 0.0f) fdphase = -fdphase;
    g_phs = {};

    rep_time = 0;
    rep_limit = (int)(pow(1.0f - p.m_repeat_speed, 2.0f) * 20000 + 32);
    if (p.m_repeat_speed == 0.0f) rep_limit = 0;
  }

  constexpr const auto waveform_fns =
      std::array { sound::waveform::square, sound::waveform::sawtooth, sound::waveform::sine, sound::waveform::noise };
  g_waveform = waveform_fns.at(wave_type);
}

void PlaySample() {
  ResetSample(false);
  playing_sample = true;
}

void SynthSample(int length, float * buffer) {
  for (int i = 0; i < length; i++, g_sample_phase++) {
    if (!playing_sample) break;

    rep_time++;
    if (rep_limit != 0 && rep_time >= rep_limit) {
      rep_time = 0;
      ResetSample(true);
    }

    // frequency envelopes/arpeggios
    arp_time++;
    if (arp_limit != 0 && arp_time >= arp_limit) {
      arp_limit = 0;
      fperiod *= arp_mod;
    }
    fslide += fdslide;
    fperiod *= fslide;
    if (fperiod > fmaxperiod) {
      fperiod = fmaxperiod;
      if (p.m_freq_limit > 0.0f) playing_sample = false;
    }
    float rfperiod = fperiod;
    if (vib_amp > 0.0f) {
      vib_phase += vib_speed;
      rfperiod = fperiod * (1.0 + sin(vib_phase) * vib_amp);
    }
    period = (int)rfperiod;
    if (period < 8) period = 8;
    // volume envelope
    float env_vol = g_env.volume(g_sample_phase, p.m_env_punch);
    if (std::isnan(env_vol)) {
      playing_sample = false;
      env_vol = 0;
    }

    // phaser step
    fphase += fdphase;

    if (flthp_d != 0.0f) {
      flthp *= flthp_d;
      if (flthp < 0.00001f) flthp = 0.00001f;
      if (flthp > 0.1f) flthp = 0.1f;
    }

    const auto sq_duty = sound::square_duty(square_duty, square_slide, g_sample_phase);

    float ssample = 0.0f;
    constexpr const auto supersample_count = 8;
    for (int si = 0; si < supersample_count; si++, g_supersample_phase++) // 8x supersampling
    {
      float sample = 0.0f;
      // base waveform
      float fp = (float)g_supersample_phase / period;
      sample = g_waveform(fp, sq_duty);
      // lp filter
      constexpr const auto max_fltw = 0.01F;
      float pp = fltp;
      fltw = sound::norm(fltw * fltw_d, 0.0F, max_fltw);
      if (p.m_lpf_freq != 1.0f) {
        fltdp += (sample - fltp) * fltw;
        fltdp -= fltdp * fltdmp;
      } else {
        fltp = sample;
        fltdp = 0.0f;
      }
      fltp += fltdp;
      // hp filter
      fltphp += fltp - pp;
      fltphp -= fltphp * flthp;
      sample = fltphp;
      // phaser
      sample = g_phs.shift(g_supersample_phase, fphase, sample);
      // final accumulation and envelope application
      ssample += sample * env_vol;
    }
    ssample = sound::apply_volumes(ssample / supersample_count, sound_vol);
    ssample = sound::norm(ssample);
    *buffer++ = ssample;
  }
}

DPInput * input;
bool mute_stream;

class sfxr_producer : public m4c0::audio::producer {
public:
  void fill_buffer(std::span<float> data) override {
    if (playing_sample && !mute_stream) {
      SynthSample(static_cast<int>(data.size()), data.data());
    } else {
      std::fill(data.begin(), data.end(), 0);
    }
  }
};

void wav_file_write_sample(FILE * output, float sample) {
  constexpr auto arbitrary_gain = 4.0F;
  auto ssample = sound::norm(sample * arbitrary_gain); // arbitrary gain to get reasonable output volume...

  if (wav_bits == 16) {
    constexpr const auto threshold = 32000; // TODO: Why not 32767?
    auto isample = static_cast<std::int16_t>(ssample * threshold);
    fwrite(&isample, 1, 2, output);
  } else {
    auto isample = static_cast<unsigned char>(ssample * 127 + 128);
    fwrite(&isample, 1, 1, output);
  }
}
void wav_file_next_sample(FILE * output) {
  std::array<float, 2> data {};
  SynthSample(2, data.data());

  if (wav_freq == 44100) {
    wav_file_write_sample(output, data.at(0));
    wav_file_write_sample(output, data.at(1));
  } else {
    auto sample = (data.at(0) + data.at(1)) / 2;
    wav_file_write_sample(output, sample);
  }
}
bool ExportWAV(char * filename) {
  FILE * foutput = fopen(filename, "wb");
  if (!foutput) return false;
  // write wav header
  char string[32];
  unsigned int dword = 0;
  unsigned short word = 0;
  fwrite("RIFF", 4, 1, foutput); // "RIFF"
  dword = 0;
  fwrite(&dword, 1, 4, foutput); // remaining file size
  auto riff_start = ftell(foutput);

  fwrite("WAVE", 4, 1, foutput); // "WAVE"

  fwrite("fmt ", 4, 1, foutput); // "fmt "
  dword = 16;
  fwrite(&dword, 1, 4, foutput); // chunk size
  word = 1;
  fwrite(&word, 1, 2, foutput); // compression code
  word = 1;
  fwrite(&word, 1, 2, foutput); // channels
  dword = wav_freq;
  fwrite(&dword, 1, 4, foutput); // sample rate
  dword = wav_freq * wav_bits / 8;
  fwrite(&dword, 1, 4, foutput); // bytes/sec
  word = wav_bits / 8;
  fwrite(&word, 1, 2, foutput); // block align
  word = wav_bits;
  fwrite(&word, 1, 2, foutput); // bits per sample

  fwrite("data", 4, 1, foutput); // "data"
  dword = 0;
  fwrite(&dword, 1, 4, foutput); // chunk size
  auto data_start = ftell(foutput);

  // write sample data
  mute_stream = true;
  PlaySample();
  while (playing_sample) {
    wav_file_next_sample(foutput);
  }
  mute_stream = false;

  auto data_end = ftell(foutput);
  auto riff_end = ftell(foutput);

  // seek back to header and write size info
  fseek(foutput, riff_start - 4, SEEK_SET);
  dword = riff_end - riff_start;
  fwrite(&dword, 1, 4, foutput); // remaining file size

  fseek(foutput, data_start - 4, SEEK_SET);
  dword = data_end - data_start;
  fwrite(&dword, 1, 4, foutput); // chunk size (data)

  fclose(foutput);

  return true;
}

void do_pickup_coin() {
  ResetParams();
  p.m_base_freq = 0.4f + frnd(0.5f);
  p.m_env_attack = 0.0f;
  p.m_env_sustain = frnd(0.1f);
  p.m_env_decay = 0.1f + frnd(0.4f);
  p.m_env_punch = 0.3f + frnd(0.3f);
  if (rnd(1)) {
    p.m_arp_speed = 0.5f + frnd(0.2f);
    p.m_arp_mod = 0.2f + frnd(0.4f);
  }
  PlaySample();
}
static void do_laser_shoot() {
  ResetParams();
  wave_type = rnd(2);
  if (wave_type == 2 && rnd(1)) wave_type = rnd(1);
  p.m_base_freq = 0.5f + frnd(0.5f);
  p.m_freq_limit = p.m_base_freq - 0.2f - frnd(0.6f);
  if (p.m_freq_limit < 0.2f) p.m_freq_limit = 0.2f;
  p.m_freq_ramp = -0.15f - frnd(0.2f);
  if (rnd(2) == 0) {
    p.m_base_freq = 0.3f + frnd(0.6f);
    p.m_freq_limit = frnd(0.1f);
    p.m_freq_ramp = -0.35f - frnd(0.3f);
  }
  if (rnd(1)) {
    p.m_duty = frnd(0.5f);
    p.m_duty_ramp = frnd(0.2f);
  } else {
    p.m_duty = 0.4f + frnd(0.5f);
    p.m_duty_ramp = -frnd(0.7f);
  }
  p.m_env_attack = 0.0f;
  p.m_env_sustain = 0.1f + frnd(0.2f);
  p.m_env_decay = frnd(0.4f);
  if (rnd(1)) p.m_env_punch = frnd(0.3f);
  if (rnd(2) == 0) {
    p.m_pha_offset = frnd(0.2f);
    p.m_pha_ramp = -frnd(0.2f);
  }
  if (rnd(1)) p.m_hpf_freq = frnd(0.3f);
  PlaySample();
}
static void do_explosion() {
  ResetParams();
  wave_type = 3;
  if (rnd(1)) {
    p.m_base_freq = 0.1f + frnd(0.4f);
    p.m_freq_ramp = -0.1f + frnd(0.4f);
  } else {
    p.m_base_freq = 0.2f + frnd(0.7f);
    p.m_freq_ramp = -0.2f - frnd(0.2f);
  }
  p.m_base_freq *= p.m_base_freq;
  if (rnd(4) == 0) p.m_freq_ramp = 0.0f;
  if (rnd(2) == 0) p.m_repeat_speed = 0.3f + frnd(0.5f);
  p.m_env_attack = 0.0f;
  p.m_env_sustain = 0.1f + frnd(0.3f);
  p.m_env_decay = frnd(0.5f);
  if (rnd(1) == 0) {
    p.m_pha_offset = -0.3f + frnd(0.9f);
    p.m_pha_ramp = -frnd(0.3f);
  }
  p.m_env_punch = 0.2f + frnd(0.6f);
  if (rnd(1)) {
    p.m_vib_strength = frnd(0.7f);
    p.m_vib_speed = frnd(0.6f);
  }
  if (rnd(2) == 0) {
    p.m_arp_speed = 0.6f + frnd(0.3f);
    p.m_arp_mod = 0.8f - frnd(1.6f);
  }
  PlaySample();
}
static void do_powerup() {
  ResetParams();
  if (rnd(1))
    wave_type = 1;
  else
    p.m_duty = frnd(0.6f);
  if (rnd(1)) {
    p.m_base_freq = 0.2f + frnd(0.3f);
    p.m_freq_ramp = 0.1f + frnd(0.4f);
    p.m_repeat_speed = 0.4f + frnd(0.4f);
  } else {
    p.m_base_freq = 0.2f + frnd(0.3f);
    p.m_freq_ramp = 0.05f + frnd(0.2f);
    if (rnd(1)) {
      p.m_vib_strength = frnd(0.7f);
      p.m_vib_speed = frnd(0.6f);
    }
  }
  p.m_env_attack = 0.0f;
  p.m_env_sustain = frnd(0.4f);
  p.m_env_decay = 0.1f + frnd(0.4f);
  PlaySample();
}
static void do_hit_hurt() {
  ResetParams();
  wave_type = rnd(2);
  if (wave_type == 2) wave_type = 3;
  if (wave_type == 0) p.m_duty = frnd(0.6f);
  p.m_base_freq = 0.2f + frnd(0.6f);
  p.m_freq_ramp = -0.3f - frnd(0.4f);
  p.m_env_attack = 0.0f;
  p.m_env_sustain = frnd(0.1f);
  p.m_env_decay = 0.1f + frnd(0.2f);
  if (rnd(1)) p.m_hpf_freq = frnd(0.3f);
  PlaySample();
}
static void do_jump() {
  ResetParams();
  wave_type = 0;
  p.m_duty = frnd(0.6f);
  p.m_base_freq = 0.3f + frnd(0.3f);
  p.m_freq_ramp = 0.1f + frnd(0.2f);
  p.m_env_attack = 0.0f;
  p.m_env_sustain = 0.1f + frnd(0.3f);
  p.m_env_decay = 0.1f + frnd(0.2f);
  if (rnd(1)) p.m_hpf_freq = frnd(0.3f);
  if (rnd(1)) p.m_lpf_freq = 1.0f - frnd(0.6f);
  PlaySample();
}
static void do_blip_select() {
  ResetParams();
  wave_type = rnd(1);
  if (wave_type == 0) p.m_duty = frnd(0.6f);
  p.m_base_freq = 0.2f + frnd(0.4f);
  p.m_env_attack = 0.0f;
  p.m_env_sustain = 0.1f + frnd(0.1f);
  p.m_env_decay = frnd(0.2f);
  p.m_hpf_freq = 0.1f;
  PlaySample();
}

static void do_randomize() {
  p.m_base_freq = pow(frnd(2.0f) - 1.0f, 2.0f);
  if (rnd(1)) p.m_base_freq = pow(frnd(2.0f) - 1.0f, 3.0f) + 0.5f;
  p.m_freq_limit = 0.0f;
  p.m_freq_ramp = pow(frnd(2.0f) - 1.0f, 5.0f);
  if (p.m_base_freq > 0.7f && p.m_freq_ramp > 0.2f) p.m_freq_ramp = -p.m_freq_ramp;
  if (p.m_base_freq < 0.2f && p.m_freq_ramp < -0.05f) p.m_freq_ramp = -p.m_freq_ramp;
  p.m_freq_dramp = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_duty = frnd(2.0f) - 1.0f;
  p.m_duty_ramp = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_vib_strength = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_vib_speed = frnd(2.0f) - 1.0f;
  p.m_vib_delay = frnd(2.0f) - 1.0f;
  p.m_env_attack = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_env_sustain = pow(frnd(2.0f) - 1.0f, 2.0f);
  p.m_env_decay = frnd(2.0f) - 1.0f;
  p.m_env_punch = pow(frnd(0.8f), 2.0f);
  if (p.m_env_attack + p.m_env_sustain + p.m_env_decay < 0.2f) {
    p.m_env_sustain += 0.2f + frnd(0.3f);
    p.m_env_decay += 0.2f + frnd(0.3f);
  }
  p.m_lpf_resonance = frnd(2.0f) - 1.0f;
  p.m_lpf_freq = 1.0f - pow(frnd(1.0f), 3.0f);
  p.m_lpf_ramp = pow(frnd(2.0f) - 1.0f, 3.0f);
  if (p.m_lpf_freq < 0.1f && p.m_lpf_ramp < -0.05f) p.m_lpf_ramp = -p.m_lpf_ramp;
  p.m_hpf_freq = pow(frnd(1.0f), 5.0f);
  p.m_hpf_ramp = pow(frnd(2.0f) - 1.0f, 5.0f);
  p.m_pha_offset = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_pha_ramp = pow(frnd(2.0f) - 1.0f, 3.0f);
  p.m_repeat_speed = frnd(2.0f) - 1.0f;
  p.m_arp_speed = frnd(2.0f) - 1.0f;
  p.m_arp_mod = frnd(2.0f) - 1.0f;
  PlaySample();
}
static void do_mutate() {
  if (rnd(1)) p.m_base_freq += frnd(0.1f) - 0.05f;
  //		if(rnd(1)) p.m_freq_limit+=frnd(0.1f)-0.05f;
  if (rnd(1)) p.m_freq_ramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_freq_dramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_duty += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_duty_ramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_vib_strength += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_vib_speed += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_vib_delay += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_env_attack += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_env_sustain += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_env_decay += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_env_punch += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_lpf_resonance += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_lpf_freq += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_lpf_ramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_hpf_freq += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_hpf_ramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_pha_offset += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_pha_ramp += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_repeat_speed += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_arp_speed += frnd(0.1f) - 0.05f;
  if (rnd(1)) p.m_arp_mod += frnd(0.1f) - 0.05f;
  PlaySample();
}
static void do_load_sound() {
  char filename[256];
  if (FileSelectorLoad(hWndMain, filename, 1)) // WIN32
  {
    ResetParams();

    auto data = file_format_ec0000::load(static_cast<const char *>(filename));
    if (data) {
      p = data->m_p;
      wave_type = data->m_wave_type;
      sound_vol = data->m_sound_vol;
    }

    PlaySample();
  }
}
static void do_save_sound() {
  char filename[256];
  if (FileSelectorSave(hWndMain, filename, 1)) // WIN32
    file_format_ec0000::save(static_cast<const char *>(filename), settings {});
}
static void do_export() {
  char filename[256];
  if (FileSelectorSave(hWndMain, filename, 0)) // WIN32
    ExportWAV(filename);
}
static void do_freq_change() {
  if (wav_freq == 44100)
    wav_freq = 22050;
  else
    wav_freq = 44100;
}
static void do_bit_change() {
  wav_bits = (wav_bits == 16) ? 8 : 16;
}
template<auto N>
static void do_set_wave() {
  wave_type = N;
}

class labeled_slider : public gui::hbox {
public:
  labeled_slider(unsigned id, const char * label, float * value, float min, float max) : hbox(true) {
    make_child<gui::label>(label);
    make_child<gui::slider>(id, value, min, max);
  }
};
class screen {
  static void create_left_menu(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::vbox>();
    res->make_child<gui::label>("GENERATOR");
    res->make_child<gui::button>(++id, "PICKUP/COIN", do_pickup_coin);
    res->make_child<gui::button>(++id, "LASER/SHOOT", do_laser_shoot);
    res->make_child<gui::button>(++id, "EXPLOSION", do_explosion);
    res->make_child<gui::button>(++id, "POWERUP", do_powerup);
    res->make_child<gui::button>(++id, "HIT/HURT", do_hit_hurt);
    res->make_child<gui::button>(++id, "JUMP", do_jump);
    res->make_child<gui::button>(++id, "BLIP/SELECT", do_blip_select);
    res->make_child<gui::button>(++id, "MUTATE", do_mutate);
    res->make_child<gui::button>(++id, "RANDOMIZE", do_randomize);
  }

  static void create_wave_btns(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::hbox>();
    res->make_child<gui::button>(++id, "SQUAREWAVE", do_set_wave<0>, wave_type == 0);
    res->make_child<gui::button>(++id, "SAWTOOTH", do_set_wave<1>, wave_type == 1);
    res->make_child<gui::button>(++id, "SINEWAVE", do_set_wave<2>, wave_type == 2);
    res->make_child<gui::button>(++id, "NOISE", do_set_wave<3>, wave_type == 3);
  }

  static void create_sliders(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::vbox>();
    res->make_child<labeled_slider>(++id, "ATTACK TIME", &p.m_env_attack, 0, 1);
    res->make_child<labeled_slider>(++id, "SUSTAIN TIME", &p.m_env_sustain, 0, 1);
    res->make_child<labeled_slider>(++id, "SUSTAIN PUNCH", &p.m_env_punch, 0, 1);
    res->make_child<labeled_slider>(++id, "DECAY TIME", &p.m_env_decay, 0, 1);

    res->make_child<labeled_slider>(++id, "START FREQUENCY", &p.m_base_freq, 0, 1);
    res->make_child<labeled_slider>(++id, "MIN FREQUENCY", &p.m_freq_limit, 0, 1);
    res->make_child<labeled_slider>(++id, "SLIDE", &p.m_freq_ramp, -1, 1);
    res->make_child<labeled_slider>(++id, "DELTA SLIDE", &p.m_freq_dramp, -1, 1);

    res->make_child<labeled_slider>(++id, "VIBRATO DEPTH", &p.m_vib_strength, 0, 1);
    res->make_child<labeled_slider>(++id, "VIBRATO SPEED", &p.m_vib_speed, 0, 1);

    res->make_child<labeled_slider>(++id, "CHANGE AMOUNT", &p.m_arp_mod, -1, 1);
    res->make_child<labeled_slider>(++id, "CHANGE SPEED", &p.m_arp_speed, 0, 1);

    res->make_child<labeled_slider>(++id, "SQUARE DUTY", &p.m_duty, 0, 1);
    res->make_child<labeled_slider>(++id, "DUTY SWEEP", &p.m_duty_ramp, -1, 1);

    res->make_child<labeled_slider>(++id, "REPEAT SPEED", &p.m_repeat_speed, 0, 1);

    res->make_child<labeled_slider>(++id, "PHASER OFFSET", &p.m_pha_offset, -1, 1);
    res->make_child<labeled_slider>(++id, "PHASER SWEEP", &p.m_pha_ramp, -1, 1);

    res->make_child<labeled_slider>(++id, "LP FILTER CUTOFF", &p.m_lpf_freq, 0, 1);
    res->make_child<labeled_slider>(++id, "LP FILTER CUTOFF SWEEP", &p.m_lpf_ramp, -1, 1);
    res->make_child<labeled_slider>(++id, "LP FILTER RESONANCE", &p.m_lpf_resonance, 0, 1);
    res->make_child<labeled_slider>(++id, "HP FILTER CUTOFF", &p.m_hpf_freq, 0, 1);
    res->make_child<labeled_slider>(++id, "HP FILTER SWEEP", &p.m_hpf_ramp, -1, 1);
  }

  static void create_br_stuff(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::vbox>();
    res->make_child<gui::label>("VOLUME");
    res->make_child<gui::slider>(++id, &sound_vol, 0, 1);
    res->make_child<gui::button>(++id, "PLAY SOUND", PlaySample);
    res->make_child<gui::button>(++id, "LOAD SOUND", do_load_sound);
    res->make_child<gui::button>(++id, "SAVE SOUND", do_save_sound);
    res->make_child<gui::button>(++id, "EXPORT .WAV", do_export);

    static constexpr const auto str_max_len = 10;
    static std::array<char, str_max_len> wav_freq_str;
    sprintf(wav_freq_str.data(), "%i HZ", wav_freq); // NOLINT
    res->make_child<gui::button>(++id, wav_freq_str.data(), do_freq_change);

    static std::array<char, str_max_len> wav_bits_str;
    sprintf(wav_bits_str.data(), "%i-BIT", wav_bits); // NOLINT
    res->make_child<gui::button>(++id, wav_bits_str.data(), do_bit_change);
  }

  static void create_bottom_stuff(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::hbox>(true);
    create_sliders(id, res);
    create_br_stuff(id, res);
  }

  static void create_right_stuff(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::vbox>();
    res->make_child<gui::label>("MANUAL SETTINGS");
    create_wave_btns(id, res);
    create_bottom_stuff(id, res);
  }

  static void create_stuff(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::hbox>();
    create_left_menu(id, res);
    create_right_stuff(id, res);
  }

  static void create_inside_margin(unsigned & id, gui::box * parent) {
    auto * res = parent->make_child<gui::stack_box>();
    create_stuff(id, res);
  }

  static gui::stack_box create_stack(unsigned & id) {
    gui::stack_box res { 0 };
    res.make_child<gui::panel>(bg_color);
    create_inside_margin(id, &res);
    return res;
  }

  unsigned id = 0;
  const gui::stack_box stack = create_stack(id);

public:
  void draw(gui::draw_context * ctx) const {
    constexpr const gui::rect full_scr { 0, 0, { 640, 480 } };
    stack.draw(ctx, full_scr);

    if (!ctx->mouse()->is_left_down()) ctx->mouse()->holder() = 0;
  }
};

bool ddkCalcFrame(gui::draw_context * ctx) {
  screen scr;
  scr.draw(ctx);
  return true;
}

void ddkInit(const m4c0::native_handles * nh) {
  srand(time(nullptr));

  ddkSetMode(640, 480, 32, 60, DDK_WINDOW, "sfxr");

  input = new DPInput(hWndMain, hInstanceMain); // WIN32

  ResetParams();

  static auto streamer = m4c0::audio::streamer::create();
  streamer->producer() = std::make_unique<sfxr_producer>();
}

void ddkFree() {
  delete input;
}
