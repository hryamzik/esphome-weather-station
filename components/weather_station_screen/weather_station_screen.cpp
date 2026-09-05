// SPDX-License-Identifier: GPL-3.0-or-later

#include "weather_station_screen.h"

#include <cmath>
#include <cstring>

#include "esphome/core/hal.h"

namespace esphome {
namespace weather_station_screen {

using ::weather_station_display::CommandKind;
using ::weather_station_display::FontRole;

void WeatherStationScreen::snapshot_(
    ::weather_station_display::ScreenSnapshot &snapshot) const {
  snapshot = {};
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
    snapshot.condition.assign(this->condition_->state.c_str());
  }
  if (this->sun_state_ != nullptr && this->sun_state_->has_state()) {
    snapshot.is_night = this->sun_state_->state == "below_horizon";
  } else {
    snapshot.is_night = std::strcmp(snapshot.condition.c_str(), "clear-night") == 0;
  }
  if (this->sunrise_ != nullptr && this->sunrise_->has_state()) {
    snapshot.sunrise.assign(this->sunrise_->state.c_str());
  }
  if (this->sunset_ != nullptr && this->sunset_->has_state()) {
    snapshot.sunset.assign(this->sunset_->state.c_str());
  }
  if (this->sun_progress_ != nullptr && this->sun_progress_->has_state() &&
      std::isfinite(this->sun_progress_->state)) {
    const float progress = this->sun_progress_->state;
    snapshot.sun_progress_valid = true;
    snapshot.sun_progress_percent =
        progress <= 0.0f
            ? 0U
            : (progress >= 100.0f
                   ? 100U
                   : static_cast<uint8_t>(std::lround(progress)));
  }
  const bool wifi_connected =
      wifi::global_wifi_component != nullptr &&
      wifi::global_wifi_component->is_connected();
  if (wifi_connected) {
    if (this->wifi_signal_ != nullptr && this->wifi_signal_->has_state()) {
      snapshot.wifi_valid = true;
      snapshot.wifi_dbm =
          static_cast<int16_t>(std::lround(this->wifi_signal_->state));
    }
    if (this->ip_address_ != nullptr && this->ip_address_->has_state()) {
      snapshot.ip_address.assign(this->ip_address_->state.c_str());
    }
    if (this->wifi_signal_ == nullptr) {
      const int8_t rssi = wifi::global_wifi_component->wifi_rssi();
      if (rssi != wifi::WIFI_RSSI_DISCONNECTED) {
        snapshot.wifi_valid = true;
        snapshot.wifi_dbm = rssi;
      }
    }
    if (this->ip_address_ == nullptr) {
      const auto addresses =
          wifi::global_wifi_component->wifi_sta_ip_addresses();
      for (const auto &address : addresses) {
        if (address.is_set()) {
          char buffer[network::IP_ADDRESS_BUFFER_SIZE];
          snapshot.ip_address.assign(address.str_to(buffer));
          break;
        }
      }
    }
  }

  if (this->weather_station_ == nullptr) {
    return;
  }
  const auto &router = this->weather_station_->router();
  const size_t primary = router.primary_station_index();
  if (primary != ::weather_station_domain::StationRouter::NO_STATION) {
    const auto &definition = router.station(primary);
    const auto &state = router.state(primary);
    snapshot.primary.name.assign(definition.name.c_str());
    snapshot.primary.heard = state.heard;
    snapshot.primary.temperature_tenths_c =
        state.reading.temperature_tenths_c;
    snapshot.primary.humidity_percent = state.reading.humidity_percent;
    snapshot.primary.age_seconds = state.age_seconds(snapshot.now_ms);
  }
  for (size_t index = 0; index < router.station_count(); index++) {
    if (index == primary) {
      continue;
    }
    const auto &definition = router.station(index);
    const auto &state = router.state(index);
    ::weather_station_display::StationView secondary;
    secondary.name.assign(definition.name.c_str());
    secondary.heard = state.heard;
    secondary.temperature_tenths_c = state.reading.temperature_tenths_c;
    secondary.humidity_percent = state.reading.humidity_percent;
    secondary.age_seconds = state.age_seconds(snapshot.now_ms);
    snapshot.add_secondary(secondary);
  }
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
  struct RenderContext {
    WeatherStationScreen *screen;
    display::Display *display;
  } context{this, &display};

  const auto draw_command =
      [](void *raw_context,
         const ::weather_station_display::DrawCommand &command,
         const char *text) {
    auto &render = *static_cast<RenderContext *>(raw_context);
    const Color color = WeatherStationScreen::color_(command.color);
    switch (command.kind) {
      case CommandKind::TEXT: {
        font::Font *font = render.screen->small_font_;
        if (command.font == FontRole::MEDIUM) {
          font = render.screen->medium_font_;
        } else if (command.font == FontRole::LARGE) {
          font = render.screen->large_font_;
        }
        display::TextAlign align = display::TextAlign::TOP_LEFT;
        if (command.align == ::weather_station_display::TextAlign::CENTER) {
          align = display::TextAlign::TOP_CENTER;
        } else if (command.align == ::weather_station_display::TextAlign::RIGHT) {
          align = display::TextAlign::TOP_RIGHT;
        }
        if (font != nullptr) {
          render.display->print(
              command.x1, command.y1, font, color, align, text);
        }
        break;
      }
      case CommandKind::LINE:
        render.display->line(
            command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::RECTANGLE:
        render.display->rectangle(
            command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::FILLED_RECTANGLE:
        render.display->filled_rectangle(
            command.x1, command.y1, command.x2, command.y2, color);
        break;
      case CommandKind::CIRCLE:
        render.display->circle(command.x1, command.y1, command.radius, color);
        break;
      case CommandKind::FILLED_CIRCLE:
        render.display->filled_circle(
            command.x1, command.y1, command.radius, color);
        break;
    }
    return true;
  };

  const bool wifi_connected =
      wifi::global_wifi_component != nullptr &&
      wifi::global_wifi_component->is_connected();
  if (!::weather_station_display::update_wifi_startup_gate(
          wifi_connected, this->wifi_ever_connected_)) {
    ::weather_station_display::emit_startup_scene(draw_command, &context);
    return;
  }

  ::weather_station_display::ScreenSnapshot snapshot;
  this->snapshot_(snapshot);
  ::weather_station_display::emit_scene(
      snapshot, this->options_, draw_command, &context);
}

}  // namespace weather_station_screen
}  // namespace esphome
