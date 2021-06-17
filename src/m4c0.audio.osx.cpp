#include "m4c0.audio.hpp"
#include "m4c0/log.hpp"

#include <AudioToolbox/AudioComponent.h>
#include <AudioToolbox/AudioOutputUnit.h>
#include <AudioToolbox/AudioUnitProperties.h>
#include <memory>

static AudioComponentInstance create_tone_unit() {
  AudioComponentDescription acd;
  acd.componentType = kAudioUnitType_Output;
  acd.componentSubType = kAudioUnitSubType_DefaultOutput;
  acd.componentManufacturer = kAudioUnitManufacturer_Apple;
  acd.componentFlags = 0;
  acd.componentFlagsMask = 0;

  AudioComponent ac = AudioComponentFindNext(nullptr, &acd);
  if (ac == nullptr) return nullptr;

  AudioComponentInstance tone_unit {};
  if (AudioComponentInstanceNew(ac, &tone_unit) != noErr) return nullptr;

  return tone_unit;
}

class osx_streamer : public audio::streamer {
  AudioComponentInstance m_tone_unit;

  static OSStatus render(
      void * ref,
      AudioUnitRenderActionFlags * /*flags*/,
      const AudioTimeStamp * /*timestamp*/,
      UInt32 /*bus_number*/,
      UInt32 number_frames,
      AudioBufferList * data) {
    auto * str = static_cast<osx_streamer *>(ref);
    auto * f_data = static_cast<float *>(data->mBuffers[0].mData);
    if (str->producer() != nullptr) str->producer()->fill_buffer({ f_data, number_frames });
    return noErr;
  }

  bool set_render_callback() {
    AURenderCallbackStruct rcs;
    rcs.inputProc = render;
    rcs.inputProcRefCon = this;
    return noErr
        == AudioUnitSetProperty(
               m_tone_unit,
               kAudioUnitProperty_SetRenderCallback,
               kAudioUnitScope_Input,
               0,
               &rcs,
               sizeof(rcs));
  }
  bool set_format() {
    constexpr const auto bits_per_byte = 8;

    AudioStreamBasicDescription sbd;
    sbd.mSampleRate = streamer::rate;
    sbd.mFormatID = kAudioFormatLinearPCM;
    sbd.mFormatFlags = static_cast<AudioFormatFlags>(kAudioFormatFlagsNativeFloatPacked)
                     | static_cast<AudioFormatFlags>(kAudioFormatFlagIsNonInterleaved);
    sbd.mBytesPerPacket = sizeof(float);
    sbd.mFramesPerPacket = 1;
    sbd.mBytesPerFrame = sizeof(float) / 1;
    sbd.mChannelsPerFrame = 1;
    sbd.mBitsPerChannel = sizeof(float) * bits_per_byte;
    return noErr
        == AudioUnitSetProperty(
               m_tone_unit,
               kAudioUnitProperty_StreamFormat,
               kAudioUnitScope_Input,
               0,
               &sbd,
               sizeof(sbd));
  }
  bool init() {
    return AudioUnitInitialize(m_tone_unit) == noErr;
  }

public:
  osx_streamer() : streamer(), m_tone_unit(create_tone_unit()) {
    if (!set_render_callback() || !set_format() || !init()) return;

    AudioOutputUnitStart(m_tone_unit);

    m4c0::log::info("Audio unit initialized");
  }
  ~osx_streamer() override {
    if (m_tone_unit == nullptr) return;
    AudioOutputUnitStop(m_tone_unit);
    AudioUnitUninitialize(m_tone_unit);
    AudioComponentInstanceDispose(m_tone_unit);
  }

  osx_streamer(const osx_streamer & o) = delete;
  osx_streamer(osx_streamer && o) = delete;
  osx_streamer & operator=(const osx_streamer & o) = delete;
  osx_streamer & operator=(osx_streamer && o) = delete;
};

std::unique_ptr<audio::streamer> audio::streamer::create() {
  return std::make_unique<osx_streamer>();
}
