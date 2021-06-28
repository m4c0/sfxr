/*

   Porting: Search for WIN32 to find windows-specific code
            snippets which need to be replaced or removed.

*/

#include "m4c0.hpp"
#include "m4c0/log.hpp"
#include "m4c0/native_handles.hpp"
#include "parameters.hpp"
#include "settings.hpp"
#include "sound.hpp"

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

struct Spriteset {
  DWORD * data;
  int width;
  int height;
  int pitch;
};

parameters p;
Spriteset font;
Spriteset ld48;

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

#include "tools.h"

void Slider(int x, int y, float & value, bool bipolar, const char * text, bool enabled = true) {
  bool hover = false;
  if (MouseInBox(x, y, 100, 10)) {
    if (mouse_leftclick) vselected = &value;
    if (mouse_rightclick) value = 0.0f;
    hover = true;
  }
  auto mv = static_cast<float>(mouse_x - mouse_px);
  if (vselected != &value) mv = 0.0f;
  if (bipolar) {
    value = sound::norm(value + mv * 0.005f);
  } else {
    value = sound::norm(value + mv * 0.0025f, 0.0F, 1.0F);
  }
  DrawBar(x - 1, y, 102, 10, 0x000000);
  int ival = (int)(value * 99);
  if (bipolar) ival = (int)(value * 49.5f + 49.5f);
  DrawBar(x, y + 1, ival, 8, hover ? 0xFFFFFF : 0xF0C090);
  DrawBar(x + ival, y + 1, 100 - ival, 8, hover ? 0x000000 : 0x807060);
  DrawBar(x + ival, y + 1, 1, 8, 0xFFFFFF);
  if (bipolar) {
    DrawBar(x + 50, y - 1, 1, 3, 0x000000);
    DrawBar(x + 50, y + 8, 1, 3, 0x000000);
  }
  DWORD tcol = enabled ? 0x000000 : 0x808080;
  DrawText(x - 4 - strlen(text) * 8, y + 1, tcol, text);
}

bool Button(int x, int y, bool highlight, const char * text, int id) {
  DWORD color1 = 0x000000;
  DWORD color2 = 0xA09088;
  DWORD color3 = 0x000000;
  bool hover = MouseInBox(x, y, 100, 17);
  if (hover && mouse_leftclick) vcurbutton = id;
  bool current = (vcurbutton == id);
  if (highlight) {
    color1 = 0x000000;
    color2 = 0x988070;
    color3 = 0xFFF0E0;
  }
  if (current && hover) {
    color1 = 0xA09088;
    color2 = 0xFFF0E0;
    color3 = 0xA09088;
  } else if (hover) {
    color1 = 0x000000;
    color2 = 0x000000;
    color3 = 0xA09088;
  }
  DrawBar(x - 1, y - 1, 102, 19, color1);
  DrawBar(x, y, 100, 17, color2);
  DrawText(x + 5, y + 5, color3, text);
  return current && hover && !mouse_left;
}

static bool should_redraw() {
  static int drawcount = 0;
  static bool firstframe = true;
  static int refresh_counter = 0;

  bool redraw = true;
  if (!firstframe && mouse_x - mouse_px == 0 && mouse_y - mouse_py == 0 && !mouse_left && !mouse_right) redraw = false;
  if (!mouse_left) {
    if (vselected != nullptr || vcurbutton > -1) {
      redraw = true;
      refresh_counter = 2;
    }
    vselected = nullptr;
  }
  if (refresh_counter > 0) {
    refresh_counter--;
    redraw = true;
  }

  if (playing_sample) redraw = true;

  constexpr auto frames_between_refreshes = 20;
  if (drawcount++ > frames_between_refreshes) {
    redraw = true;
    drawcount = 0;
  }

  firstframe = false;
  return redraw;
}

static void do_pickup_coin() {
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

static constexpr const auto bg_color = 0xC0B090;
static constexpr const auto txt_color = 0x504030;
static constexpr const auto bar_color = 0x000000;
void DrawGeneratorButtons() {
  struct button {
    const char * name;
    void (*callback)();
  };
  constexpr const auto categories = std::array {
    button { "PICKUP/COIN", do_pickup_coin }, button { "LASER/SHOOT", do_laser_shoot },
    button { "EXPLOSION", do_explosion },     button { "POWERUP", do_powerup },
    button { "HIT/HURT", do_hit_hurt },       button { "JUMP", do_jump },
    button { "BLIP/SELECT", do_blip_select },
  };

  int i = 0;
  for (const auto & cat : categories) {
    if (Button(5, 35 + i * 30, false, cat.name, 300 + i)) {
      cat.callback();
      PlaySample();
    }
    i++;
  }
}
void DrawButtonsAndWhereabouts() {
  DrawText(10, 10, txt_color, "GENERATOR");
  DrawGeneratorButtons();

  DrawBar(110, 0, 2, 480, bar_color);
  DrawText(120, 10, txt_color, "MANUAL SETTINGS");
  DrawSprite(ld48, 8, 440, 0, 0xB0A080);

  if (Button(130, 30, wave_type == 0, "SQUAREWAVE", 10)) wave_type = 0;
  if (Button(250, 30, wave_type == 1, "SAWTOOTH", 11)) wave_type = 1;
  if (Button(370, 30, wave_type == 2, "SINEWAVE", 12)) wave_type = 2;
  if (Button(490, 30, wave_type == 3, "NOISE", 13)) wave_type = 3;

  DrawBar(5 - 1 - 1, 412 - 1 - 1, 102 + 2, 19 + 2, bar_color);
  if (Button(5, 412, false, "RANDOMIZE", 40)) do_randomize();

  if (Button(5, 382, false, "MUTATE", 30)) do_mutate();

  DrawText(515, 170, 0x000000, "VOLUME");
  DrawBar(490 - 1 - 1 + 60, 180 - 1 + 5, 70, 2, bar_color);
  DrawBar(490 - 1 - 1 + 60 + 68, 180 - 1 + 5, 2, 205, bar_color);
  DrawBar(490 - 1 - 1 + 60, 180 - 1, 42 + 2, 10 + 2, 0xFF0000);
  Slider(490, 180, sound_vol, false, " ");
  if (Button(490, 200, false, "PLAY SOUND", 20)) PlaySample();

  if (Button(490, 290, false, "LOAD SOUND", 14)) do_load_sound();
  if (Button(490, 320, false, "SAVE SOUND", 15)) do_save_sound();

  DrawBar(490 - 1 - 1 + 60, 380 - 1 + 9, 70, 2, bar_color);
  DrawBar(490 - 1 - 2, 380 - 1 - 2, 102 + 4, 19 + 4, bar_color);
  if (Button(490, 380, false, "EXPORT .WAV", 16)) do_export();
  char str[10];
  sprintf(str, "%i HZ", wav_freq);
  if (Button(490, 410, false, str, 18)) do_freq_change();
  sprintf(str, "%i-BIT", wav_bits);
  if (Button(490, 440, false, str, 19)) do_bit_change();
}
void DrawSlidersAndWhereabouts() {
  int xpos = 350;
  int ypos = 4;

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_env_attack, false, "ATTACK TIME");
  Slider(xpos, (ypos++) * 18, p.m_env_sustain, false, "SUSTAIN TIME");
  Slider(xpos, (ypos++) * 18, p.m_env_punch, false, "SUSTAIN PUNCH");
  Slider(xpos, (ypos++) * 18, p.m_env_decay, false, "DECAY TIME");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_base_freq, false, "START FREQUENCY");
  Slider(xpos, (ypos++) * 18, p.m_freq_limit, false, "MIN FREQUENCY");
  Slider(xpos, (ypos++) * 18, p.m_freq_ramp, true, "SLIDE");
  Slider(xpos, (ypos++) * 18, p.m_freq_dramp, true, "DELTA SLIDE");

  Slider(xpos, (ypos++) * 18, p.m_vib_strength, false, "VIBRATO DEPTH");
  Slider(xpos, (ypos++) * 18, p.m_vib_speed, false, "VIBRATO SPEED");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_arp_mod, true, "CHANGE AMOUNT");
  Slider(xpos, (ypos++) * 18, p.m_arp_speed, false, "CHANGE SPEED");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_duty, false, "SQUARE DUTY", wave_type == 0);
  Slider(xpos, (ypos++) * 18, p.m_duty_ramp, true, "DUTY SWEEP", wave_type == 0);

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_repeat_speed, false, "REPEAT SPEED");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_pha_offset, true, "PHASER OFFSET");
  Slider(xpos, (ypos++) * 18, p.m_pha_ramp, true, "PHASER SWEEP");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  Slider(xpos, (ypos++) * 18, p.m_lpf_freq, false, "LP FILTER CUTOFF");
  Slider(xpos, (ypos++) * 18, p.m_lpf_ramp, true, "LP FILTER CUTOFF SWEEP");
  Slider(xpos, (ypos++) * 18, p.m_lpf_resonance, false, "LP FILTER RESONANCE");
  Slider(xpos, (ypos++) * 18, p.m_hpf_freq, false, "HP FILTER CUTOFF");
  Slider(xpos, (ypos++) * 18, p.m_hpf_ramp, true, "HP FILTER CUTOFF SWEEP");

  DrawBar(xpos - 190, ypos * 18 - 5, 300, 2, bar_color);

  DrawBar(xpos - 190, 4 * 18 - 5, 1, (ypos - 4) * 18, bar_color);
  DrawBar(xpos - 190 + 299, 4 * 18 - 5, 1, (ypos - 4) * 18, bar_color);
}
void DrawScreen() {
  if (!should_redraw()) return;

  ddkLock();

  ClearScreen(bg_color);
  DrawButtonsAndWhereabouts();
  DrawSlidersAndWhereabouts();

  ddkUnlock();

  if (!mouse_left) vcurbutton = -1;
}

bool keydown = false;

bool ddkCalcFrame() {
  input->Update(); // WIN32 (for keyboard input)

  if (input->KeyPressed(DIK_SPACE) || input->KeyPressed(DIK_RETURN)) // WIN32 (keyboard input only for
                                                                     // convenience, ok to remove)
  {
    if (!keydown) {
      PlaySample();
      keydown = true;
    }
  } else
    keydown = false;

  DrawScreen();

  Sleep(5); // WIN32
  return true;
}

void ddkInit(const m4c0::native_handles * np) {
  srand(time(nullptr));

  ddkSetMode(640, 480, 32, 60, DDK_WINDOW, "sfxr");

  if (LoadTGA(np, font, "font") < 0) {
    m4c0::log::error("Error could not open font.tga");
    std::terminate();
  }
  if (LoadTGA(np, ld48, "ld48") < 0) {
    m4c0::log::error("Error could not open ld48.tga");
    std::terminate();
  }

  ld48.width = ld48.pitch;

  input = new DPInput(hWndMain, hInstanceMain); // WIN32

  ResetParams();

  static auto streamer = m4c0::audio::streamer::create();
  streamer->producer() = std::make_unique<sfxr_producer>();
}

void ddkFree() {
  delete input;
  free(ld48.data);
  free(font.data);
}
