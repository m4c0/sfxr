#pragma once

#include <memory>
#include <span>

namespace audio {
  class producer {
  public:
    producer() = default;
    producer(const producer &) = default;
    producer(producer &&) = default;
    producer & operator=(const producer &) = default;
    producer & operator=(producer &&) = default;
    virtual ~producer() = default;

    virtual void fill_buffer(std::span<float> data) = 0;
  };

  class streamer {
    std::unique_ptr<producer> m_producer;

  public:
    static constexpr const auto rate = 44100;

    streamer() = default;
    streamer(const streamer &) = delete;
    streamer(streamer &&) = default;
    streamer & operator=(const streamer &) = delete;
    streamer & operator=(streamer &&) = default;
    virtual ~streamer() = default;

    [[nodiscard]] constexpr const auto & producer() const noexcept {
      return m_producer;
    }
    [[nodiscard]] constexpr auto & producer() noexcept {
      return m_producer;
    }

    static std::unique_ptr<streamer> create();
  };
}
