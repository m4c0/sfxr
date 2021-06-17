#include "m4c0.audio.hpp"
#include "m4c0/log.hpp"

#include <wrl/client.h>
#include <xaudio2.h>

struct destroyer {
  void operator()(IXAudio2Voice * ptr) {
    ptr->DestroyVoice();
  }
};
class win_streamer : public audio::streamer {
  Microsoft::WRL::ComPtr<IXAudio2> m_xa2;
  std::unique_ptr<IXAudio2MasteringVoice, destroyer> m_main_voice;
  std::unique_ptr<IXAudio2SourceVoice, destroyer> m_src_voice;

public:
  win_streamer() {
    HRESULT hr {};
    if (FAILED(hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED))) {
      m4c0::log::warning("Failed to initialise COM");
      return;
    }

    if (FAILED(hr = XAudio2Create(m_xa2.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR))) {
      m4c0::log::warning("Failed to initialise XAudio2");
      return;
    }

    IXAudio2MasteringVoice * mv {};
    if (FAILED(hr = m_xa2->CreateMasteringVoice(&mv))) {
      m4c0::log::warning("Failed to initialise XAudio2 main voice");
      return;
    }
    m_main_voice.reset(mv);

    constexpr const auto channels = 1;
    constexpr const auto bits_per_sample = 32; // IEEE_FORMAT
    constexpr const auto alignment = (channels * bits_per_sample) / 8;
    WAVEFORMATEX wfx {};
    wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nChannels = channels;
    wfx.nSamplesPerSec = rate;
    wfx.nAvgBytesPerSec = rate * sizeof(float);
    wfx.nBlockAlign = alignment;
    wfx.wBitsPerSample = bits_per_sample;
    wfx.cbSize = 0;

    constexpr const auto ratio = XAUDIO2_DEFAULT_FREQ_RATIO;
    IXAudio2SourceVoice * sv {};
    if (FAILED(hr = m_xa2->CreateSourceVoice(&sv, &wfx, 0, ratio))) {
      m4c0::log::warning("Failed to initialise XAudio2 source voice");
      return;
    }

    m4c0::log::info("XAudio2 initialised");
  }
};

std::unique_ptr<audio::streamer> audio::streamer::create() {
  return std::make_unique<win_streamer>();
}
