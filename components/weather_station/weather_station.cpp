// SPDX-License-Identifier: GPL-3.0-or-later

#include "weather_station.h"

#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome {
namespace weather_station {

static const char *const TAG = "weather_station";

void WeatherStationComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Weather Station Decoder:");
  LOG_SENSOR("  ", "Temperature", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity", this->humidity_sensor_);
  LOG_SENSOR("  ", "Channel", this->channel_sensor_);
  LOG_SENSOR("  ", "Rolling Code", this->rolling_code_sensor_);
  LOG_BINARY_SENSOR("  ", "Battery Low", this->battery_low_sensor_);
}

bool WeatherStationComponent::on_receive(remote_base::RemoteReceiveData data) {
  const auto &raw = data.get_raw_data();
  return this->decode_and_publish_(raw.data(), raw.size());
}

bool WeatherStationComponent::decode_and_publish_(
    const int32_t *pulses, size_t pulse_count) {
  ::weather_station_decoder::Oregon2Reading reading;
  if (!this->decoder_.decode(pulses, pulse_count, reading)) {
    return false;
  }

  if (this->temperature_sensor_ != nullptr) {
    this->temperature_sensor_->publish_state(reading.temperature_tenths_c / 10.0f);
  }
  if (this->humidity_sensor_ != nullptr) {
    this->humidity_sensor_->publish_state(reading.humidity_percent);
  }
  if (this->channel_sensor_ != nullptr) {
    this->channel_sensor_->publish_state(reading.channel);
  }
  if (this->rolling_code_sensor_ != nullptr) {
    this->rolling_code_sensor_->publish_state(reading.rolling_code);
  }
  if (this->battery_low_sensor_ != nullptr) {
    this->battery_low_sensor_->publish_state(reading.battery_low);
  }

  ESP_LOGI(
      TAG,
      "Oregon2 THGR122N id=0x%04X channel=%u rolling=0x%02X "
      "temperature=%.1fC humidity=%u%% battery=%s raw=0x%08" PRIX32,
      reading.sensor_model,
      reading.channel,
      reading.rolling_code,
      reading.temperature_tenths_c / 10.0f,
      reading.humidity_percent,
      reading.battery_low ? "LOW" : "OK",
      reading.raw_fixed_data);
  return true;
}

}  // namespace weather_station
}  // namespace esphome
