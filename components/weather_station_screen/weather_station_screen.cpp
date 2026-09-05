// SPDX-License-Identifier: GPL-3.0-or-later

#include "weather_station_screen.h"

#include <cmath>

#include "esphome/core/hal.h"

namespace esphome {
namespace weather_station_screen {

using ::weather_station_display::CommandKind;
using ::weather_station_display::FontRole;

::weather_station_display::ScreenSnapshot WeatherStationScreen::snapshot_() const {
  ::weather_station_display::ScreenSnapshot snapshot;
  snapshot.now_ms = millis();

  if (this->time_ != nullptr) {
    const auto now = this->time_->now();
    snapshot.time_valid = now.is_valid();
    if (snapshot.time_valid) {
      snapshot.hour = now.hour;
      snapshot.minute = now.minute;
      snapshot.year = now.year;
      snapshot.month = now.month;
      snapshot.day = now.day_of_month;
      snapshot.day_of_week = now.day_of_week;
    }
  }
  if (this->condition_ != nullptr && this->condition_->has_state()) {
    snapshot.condition = this->condition_->state;
  }
  if (this->sun_state_ != nullptr && this->sun_state_->has_state()) {
    snapshot.is_night = this->sun_state_->state == "below_horizon";
  } else {
    snapshot.is_night = snapshot.condition == "clear-night";
  }
  if (this->sunrise_ != nullptr && this->sunrise_->has_state()) {
    snapshot.sunrise = this->sunrise_->state;
  }
  if (this->sunset_ != nullptr && this->sunset_->has_state()) {
    snapshot.sunset = this->sunset_->state;
  }
  if (this->wifi_signal_ != nullptr && this->wifi_signal_->has_state()) {
    snapshot.wifi_valid = true;
    snapshot.wifi_dbm = static_cast<int16_t>(std::lround(this->wifi_signal_->state));
  }
  if (this->ip_address_ != nullptr && this->ip_address_->has_state()) {
    snapshot.ip_address = this->ip_address_->state;
  }

  if (this->weather_station_ == nullptr) {
    return snapshot;
  }
  const auto &router = this->weather_station_->router();
  const size_t primary = router.primary_station_index();
  if (primary != ::weather_station_domain::StationRouter::NO_STATION) {
    const auto &definition = router.station(primary);
    const auto &state = router.state(primary);
    snapshot.primary = {
        definition.name,
        state.heard,
        state.reading.temperature_tenths_c,
        state.reading.humidity_percent,
        state.age_seconds(snapshot.now_ms)};
  }
  for (size_t index = 0; index < router.station_count(); index++) {
    if (index == primary) {
      continue;
    }
    const auto &definition = router.station(index);
    const auto &state = router.state(index);
    snapshot.secondaries.push_back(
        {definition.name,
         state.heard,
         state.reading.temperature_tenths_c,
         state.reading.humidity_percent,
         state.age_seconds(snapshot.now_ms)});
  }
  return snapshot;
}

Color WeatherStationScreen::color_(::weather_station_display::ColorRole role) {
  using ::weather_station_display::ColorRole;
  switch (role) {
    case ColorRole::BACKGROUND:
      return Color(7, 17, 31);
    case ColorRole::CARD:
      return Color(16, 36, 58);
    case ColorRole::TEXT:
      return Color(244, 247, 251);
    case ColorRole::MUTED:
      return Color(140, 165, 188);
    case ColorRole::ACCENT:
      return Color(54, 209, 196);
    case ColorRole::SUN:
      return Color(255, 200, 87);
    case ColorRole::CLOUD:
      return Color(196, 211, 223);
    case ColorRole::RAIN:
      return Color(88, 166, 255);
    case ColorRole::WARNING:
      return Color(255, 107, 107);
  }
  return Color(255, 255, 255);
}

void WeatherStationScreen::render(display::Display &display) {
  const auto scene =
      ::weather_station_display::build_scene(this->snapshot_(), this->options_);
  for (const auto &command : scene) {
    const Color color = color_(command.color);
    switch (command.kind) {
      case CommandKind::TEXT: {
        font::Font *font = this->small_font_;
        if (command.font == FontRole::MEDIUM) {
          font = this->medium_font_;
        } else if (command.font == FontRole::LARGE) {
          font = this->large_font_;
        }
        display::TextAlign align = display::TextAlign::TOP_LEFT;
        if (command.align == ::weather_station_display::TextAlign::CENTER) {
          align = display::TextAlign::TOP_CENTER;
        } else if (command.align == ::weather_station_display::TextAlign::RIGHT) {
          align = display::TextAlign::TOP_RIGHT;
        }
        if (font != nullptr) {
          display.print(command.x1, command.y1, font, color, align, command.text.c_str());
        }
        break;
      }
      case CommandKind::LINE:
        display.line(command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::RECTANGLE:
        display.rectangle(command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::FILLED_RECTANGLE:
        display.filled_rectangle(command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::CIRCLE:
        display.circle(command.x1, command.y1, command.radius, color);
        break;
      case CommandKind::FILLED_CIRCLE:
        display.filled_circle(command.x1, command.y1, command.radius, color);
        break;
    }
  }
}

}  // namespace weather_station_screen
}  // namespace esphome
