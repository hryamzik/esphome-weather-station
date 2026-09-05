// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "oregon2_decoder.h"

namespace esphome {
namespace weather_station {

class WeatherStationComponent : public Component, public remote_base::RemoteReceiverListener {
 public:
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
  void set_humidity_sensor(sensor::Sensor *sensor) { this->humidity_sensor_ = sensor; }
  void set_channel_sensor(sensor::Sensor *sensor) { this->channel_sensor_ = sensor; }
  void set_rolling_code_sensor(sensor::Sensor *sensor) { this->rolling_code_sensor_ = sensor; }
  void set_battery_low_sensor(binary_sensor::BinarySensor *sensor) {
    this->battery_low_sensor_ = sensor;
  }

  void dump_config() override;
  bool on_receive(remote_base::RemoteReceiveData data) override;

  template<typename T> bool inject(const T &pulses) {
    std::vector<int32_t> copy;
    copy.reserve(pulses.size());
    for (size_t index = 0; index < pulses.size(); index++) {
      copy.push_back(pulses[index]);
    }
    return this->decode_and_publish_(copy.data(), copy.size());
  }

 protected:
  bool decode_and_publish_(const int32_t *pulses, size_t pulse_count);

  ::weather_station_decoder::Oregon2Decoder decoder_;
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *channel_sensor_{nullptr};
  sensor::Sensor *rolling_code_sensor_{nullptr};
  binary_sensor::BinarySensor *battery_low_sensor_{nullptr};
};

}  // namespace weather_station
}  // namespace esphome
