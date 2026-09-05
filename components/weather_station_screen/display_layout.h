// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace weather_station_display {

enum class CommandKind : uint8_t {
  TEXT,
  LINE,
  RECTANGLE,
  FILLED_RECTANGLE,
  CIRCLE,
  FILLED_CIRCLE,
};

enum class ColorRole : uint8_t {
  BACKGROUND,
  CARD,
  TEXT,
  MUTED,
  ACCENT,
  SUN,
  CLOUD,
  RAIN,
  WARNING,
};

enum class FontRole : uint8_t {
  SMALL,
  MEDIUM,
  LARGE,
};

enum class TextAlign : uint8_t {
  LEFT,
  CENTER,
  RIGHT,
};

struct DrawCommand {
  CommandKind kind{CommandKind::TEXT};
  int16_t x1{0};
  int16_t y1{0};
  int16_t x2{0};
  int16_t y2{0};
  int16_t radius{0};
  ColorRole color{ColorRole::TEXT};
  FontRole font{FontRole::SMALL};
  TextAlign align{TextAlign::LEFT};
  std::string text;
};

struct StationView {
  std::string name;
  bool heard{false};
  int16_t temperature_tenths_c{0};
  uint8_t humidity_percent{0};
  uint32_t age_seconds{0};
};

struct ScreenSnapshot {
  bool time_valid{false};
  uint8_t hour{0};
  uint8_t minute{0};
  uint16_t year{0};
  uint8_t month{0};
  uint8_t day{0};
  uint8_t day_of_week{1};
  std::string condition;
  bool is_night{false};
  StationView primary;
  std::vector<StationView> secondaries;
  std::string sunrise;
  std::string sunset;
  bool wifi_valid{false};
  int16_t wifi_dbm{0};
  std::string ip_address;
  uint32_t now_ms{0};
};

struct LayoutOptions {
  bool use_24_hour{false};
  bool show_time{true};
  bool show_date{true};
  bool show_condition{true};
  bool show_primary{true};
  bool show_secondary{true};
  bool show_sun{true};
  bool show_network{true};
};

using Scene = std::vector<DrawCommand>;

std::string format_time(uint8_t hour, uint8_t minute, bool use_24_hour);
std::string format_date(
    uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week);
std::string format_age(uint32_t age_seconds, bool heard);
std::string format_station_values(const StationView &station);
std::string humanize_condition(const std::string &condition);
size_t selected_secondary_index(size_t count, uint32_t now_ms);
uint8_t wifi_signal_level(bool available, int16_t rssi_dbm);
Scene build_scene(const ScreenSnapshot &snapshot, const LayoutOptions &options);
const char *color_hex(ColorRole color);

}  // namespace weather_station_display
