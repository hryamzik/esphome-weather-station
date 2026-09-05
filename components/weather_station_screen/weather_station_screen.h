// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "display_layout.h"
#include "esphome/components/display/display.h"
#include "esphome/components/font/font.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/components/weather_station/weather_station.h"
#include "esphome/components/wifi/wifi_component.h"

namespace esphome {
namespace weather_station_screen {

class WeatherStationScreen {
 public:
  void set_weather_station(weather_station::WeatherStationComponent *station) {
    this->weather_station_ = station;
  }
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_fonts(font::Font *small, font::Font *medium, font::Font *large) {
    this->small_font_ = small;
    this->medium_font_ = medium;
    this->large_font_ = large;
  }
  void set_condition(text_sensor::TextSensor *sensor) { this->condition_ = sensor; }
  void set_sun_state(text_sensor::TextSensor *sensor) { this->sun_state_ = sensor; }
  void set_sunrise(text_sensor::TextSensor *sensor) { this->sunrise_ = sensor; }
  void set_sunset(text_sensor::TextSensor *sensor) { this->sunset_ = sensor; }
  void set_sun_progress(sensor::Sensor *sensor) { this->sun_progress_ = sensor; }
  void set_wifi_signal(sensor::Sensor *sensor) { this->wifi_signal_ = sensor; }
  void set_ip_address(text_sensor::TextSensor *sensor) { this->ip_address_ = sensor; }

  void set_use_24_hour(bool value) { this->options_.use_24_hour = value; }
  void set_show_time(bool value) { this->options_.show_time = value; }
  void set_show_date(bool value) { this->options_.show_date = value; }
  void set_show_condition(bool value) { this->options_.show_condition = value; }
  void set_show_primary(bool value) { this->options_.show_primary = value; }
  void set_show_secondary(bool value) { this->options_.show_secondary = value; }
  void set_show_sun(bool value) { this->options_.show_sun = value; }
  void set_show_network(bool value) { this->options_.show_network = value; }

  void render(display::Display &display);

 protected:
  void snapshot_(::weather_station_display::ScreenSnapshot &snapshot) const;
  static Color color_(::weather_station_display::ColorRole role);

  weather_station::WeatherStationComponent *weather_station_{nullptr};
  time::RealTimeClock *time_{nullptr};
  font::Font *small_font_{nullptr};
  font::Font *medium_font_{nullptr};
  font::Font *large_font_{nullptr};
  text_sensor::TextSensor *condition_{nullptr};
  text_sensor::TextSensor *sun_state_{nullptr};
  text_sensor::TextSensor *sunrise_{nullptr};
  text_sensor::TextSensor *sunset_{nullptr};
  sensor::Sensor *sun_progress_{nullptr};
  sensor::Sensor *wifi_signal_{nullptr};
  text_sensor::TextSensor *ip_address_{nullptr};
  ::weather_station_display::LayoutOptions options_;
};

}  // namespace weather_station_screen
}  // namespace esphome
