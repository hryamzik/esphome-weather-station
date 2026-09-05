// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "oregon2_decoder.h"
#include "station_router.h"

namespace esphome {
namespace weather_station {

class WeatherStationComponent : public Component, public remote_base::RemoteReceiverListener {
 public:
  void enable_protocol(uint8_t protocol);
  void add_station(
      const std::string &id,
      const std::string &name,
      bool primary,
      uint8_t protocol,
      uint16_t model,
      uint8_t channel,
      bool has_rolling_code,
      uint8_t rolling_code,
      sensor::Sensor *temperature,
      sensor::Sensor *humidity,
      sensor::Sensor *channel_sensor,
      sensor::Sensor *rolling_code_sensor,
      binary_sensor::BinarySensor *battery_low,
      sensor::Sensor *age);
  void add_ignore(
      uint8_t protocol,
      uint16_t model,
      uint8_t channel,
      bool has_rolling_code,
      uint8_t rolling_code);
  void set_last_unknown_selector_sensor(text_sensor::TextSensor *sensor) {
    this->last_unknown_selector_sensor_ = sensor;
  }
  void set_recent_unknown_count_sensor(sensor::Sensor *sensor) {
    this->recent_unknown_count_sensor_ = sensor;
  }
  void set_unknown_window_ms(uint32_t value) { this->unknown_window_ms_ = value; }

  void dump_config() override;
  void loop() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;
  const ::weather_station_domain::StationRouter &router() const { return this->router_; }

  template<typename T> bool inject(const T &pulses) {
    std::vector<int32_t> copy;
    copy.reserve(pulses.size());
    for (size_t index = 0; index < pulses.size(); index++) {
      copy.push_back(pulses[index]);
    }
    return this->decode_and_publish_(copy.data(), copy.size());
  }

 protected:
  struct StationBinding {
    std::string id;
    std::string name;
    sensor::Sensor *temperature{nullptr};
    sensor::Sensor *humidity{nullptr};
    sensor::Sensor *channel{nullptr};
    sensor::Sensor *rolling_code{nullptr};
    binary_sensor::BinarySensor *battery_low{nullptr};
    sensor::Sensor *age{nullptr};
  };

  bool decode_and_publish_(const int32_t *pulses, size_t pulse_count);
  void publish_station_(size_t index);
  void publish_diagnostics_();

  ::weather_station_decoder::Oregon2Decoder decoder_;
  ::weather_station_domain::StationRouter router_;
  std::vector<StationBinding> bindings_;
  bool oregon2_enabled_{false};
  text_sensor::TextSensor *last_unknown_selector_sensor_{nullptr};
  sensor::Sensor *recent_unknown_count_sensor_{nullptr};
  uint32_t unknown_window_ms_{300000U};
  uint32_t last_periodic_update_ms_{0};
  size_t last_unknown_count_{static_cast<size_t>(-1)};
};

}  // namespace weather_station
}  // namespace esphome
