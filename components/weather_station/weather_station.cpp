// SPDX-License-Identifier: GPL-3.0-or-later

#include "weather_station.h"

#include <cinttypes>

#include "esphome/core/log.h"

namespace esphome {
namespace weather_station {

static const char *const TAG = "weather_station";

void WeatherStationComponent::enable_protocol(uint8_t protocol) {
  if (protocol == static_cast<uint8_t>(::weather_station_domain::Protocol::OREGON2)) {
    this->oregon2_enabled_ = true;
  }
}

void WeatherStationComponent::add_station(
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
    sensor::Sensor *age) {
  ::weather_station_domain::StationDefinition definition{
      id,
      name,
      {static_cast<::weather_station_domain::Protocol>(protocol),
       model,
       channel,
       has_rolling_code,
       rolling_code},
      primary};
  if (!this->router_.add_station(definition)) {
    ESP_LOGE(TAG, "Rejected duplicate or ambiguous station '%s'", id.c_str());
    return;
  }
  this->bindings_.push_back(
      {id,
       name,
       temperature,
       humidity,
       channel_sensor,
       rolling_code_sensor,
       battery_low,
       age});
}

void WeatherStationComponent::add_ignore(
    uint8_t protocol,
    uint16_t model,
    uint8_t channel,
    bool has_rolling_code,
    uint8_t rolling_code) {
  this->router_.add_ignore(
      {static_cast<::weather_station_domain::Protocol>(protocol),
       model,
       channel,
       has_rolling_code,
       rolling_code});
}

void WeatherStationComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Weather Station:");
  ESP_LOGCONFIG(TAG, "  Oregon2: %s", YESNO(this->oregon2_enabled_));
  ESP_LOGCONFIG(TAG, "  Configured stations: %u", this->bindings_.size());
  for (const auto &binding : this->bindings_) {
    ESP_LOGCONFIG(TAG, "  Station %s (%s):", binding.id.c_str(), binding.name.c_str());
    LOG_SENSOR("    ", "Temperature", binding.temperature);
    LOG_SENSOR("    ", "Humidity", binding.humidity);
    LOG_SENSOR("    ", "Channel", binding.channel);
    LOG_SENSOR("    ", "Rolling Code", binding.rolling_code);
    LOG_BINARY_SENSOR("    ", "Battery Low", binding.battery_low);
    LOG_SENSOR("    ", "Age", binding.age);
  }
  LOG_TEXT_SENSOR("  ", "Last Unknown Selector", this->last_unknown_selector_sensor_);
  LOG_SENSOR("  ", "Recent Unknown Count", this->recent_unknown_count_sensor_);
}

void WeatherStationComponent::loop() {
  const uint32_t now = millis();
  if (now - this->last_periodic_update_ms_ < 1000U) {
    return;
  }
  this->last_periodic_update_ms_ = now;
  for (size_t index = 0; index < this->bindings_.size(); index++) {
    if (this->bindings_[index].age != nullptr && this->router_.state(index).heard) {
      this->bindings_[index].age->publish_state(
          this->router_.state(index).age_seconds(now));
    }
  }
  this->publish_diagnostics_();
}

bool WeatherStationComponent::on_receive(remote_base::RemoteReceiveData data) {
  const auto &raw = data.get_raw_data();
  return this->decode_and_publish_(raw.data(), raw.size());
}

bool WeatherStationComponent::decode_and_publish_(
    const int32_t *pulses, size_t pulse_count) {
  if (!this->oregon2_enabled_) {
    return false;
  }

  ::weather_station_decoder::Oregon2Reading reading;
  if (!this->decoder_.decode(pulses, pulse_count, reading)) {
    return false;
  }

  const auto decoded = ::weather_station_domain::from_oregon2(
      reading.sensor_model,
      reading.channel,
      reading.rolling_code,
      reading.temperature_tenths_c,
      reading.humidity_percent,
      reading.battery_low);
  const auto result = this->router_.route(decoded, millis());

  if (result.kind == ::weather_station_domain::RouteKind::CONFIGURED) {
    this->publish_station_(result.station_index);
  } else if (result.kind == ::weather_station_domain::RouteKind::UNKNOWN) {
    if (this->last_unknown_selector_sensor_ != nullptr) {
      this->last_unknown_selector_sensor_->publish_state(this->router_.last_unknown_yaml());
    }
    this->publish_diagnostics_();
  }

  ESP_LOGI(
      TAG,
      "Oregon2 THGR122N id=0x%04X channel=%u rolling=0x%02X "
      "temperature=%.1fC humidity=%u%% battery=%s route=%u",
      reading.sensor_model,
      reading.channel,
      reading.rolling_code,
      reading.temperature_tenths_c / 10.0f,
      reading.humidity_percent,
      reading.battery_low ? "LOW" : "OK",
      static_cast<unsigned>(result.kind));
  return true;
}

void WeatherStationComponent::publish_station_(size_t index) {
  const auto &state = this->router_.state(index);
  const auto &reading = state.reading;
  const auto &binding = this->bindings_[index];
  using namespace ::weather_station_domain;

  if (binding.temperature != nullptr && (reading.capabilities & CAP_TEMPERATURE) != 0U) {
    binding.temperature->publish_state(reading.temperature_tenths_c / 10.0f);
  }
  if (binding.humidity != nullptr && (reading.capabilities & CAP_HUMIDITY) != 0U) {
    binding.humidity->publish_state(reading.humidity_percent);
  }
  if (binding.channel != nullptr && (reading.capabilities & CAP_CHANNEL) != 0U) {
    binding.channel->publish_state(reading.identity.channel);
  }
  if (binding.rolling_code != nullptr && (reading.capabilities & CAP_ROLLING_CODE) != 0U) {
    binding.rolling_code->publish_state(reading.identity.rolling_code);
  }
  if (binding.battery_low != nullptr && (reading.capabilities & CAP_BATTERY_LOW) != 0U) {
    binding.battery_low->publish_state(reading.battery_low);
  }
  if (binding.age != nullptr) {
    binding.age->publish_state(0);
  }
}

void WeatherStationComponent::publish_diagnostics_() {
  const size_t count =
      this->router_.recent_unknown_count(millis(), this->unknown_window_ms_);
  if (this->recent_unknown_count_sensor_ == nullptr) {
    return;
  }
  if (count != this->last_unknown_count_) {
    this->recent_unknown_count_sensor_->publish_state(count);
    this->last_unknown_count_ = count;
  }
}

}  // namespace weather_station
}  // namespace esphome
