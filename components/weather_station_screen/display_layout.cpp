// SPDX-License-Identifier: GPL-3.0-or-later

#include "display_layout.h"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace weather_station_display {
namespace {

const char *const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *const MONTHS[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void add_text(
    Scene &scene,
    int x,
    int y,
    FontRole font,
    ColorRole color,
    TextAlign align,
    const std::string &text) {
  scene.push_back(
      {CommandKind::TEXT, static_cast<int16_t>(x), static_cast<int16_t>(y), 0, 0,
       0, color, font, align, text});
}

void add_line(
    Scene &scene, int x1, int y1, int x2, int y2, ColorRole color) {
  scene.push_back(
      {CommandKind::LINE, static_cast<int16_t>(x1), static_cast<int16_t>(y1),
       static_cast<int16_t>(x2), static_cast<int16_t>(y2), 0, color,
       FontRole::SMALL, TextAlign::LEFT, ""});
}

void add_rect(
    Scene &scene, int x, int y, int width, int height, ColorRole color, bool filled) {
  scene.push_back(
      {filled ? CommandKind::FILLED_RECTANGLE : CommandKind::RECTANGLE,
       static_cast<int16_t>(x), static_cast<int16_t>(y),
       static_cast<int16_t>(width), static_cast<int16_t>(height), 0, color,
       FontRole::SMALL, TextAlign::LEFT, ""});
}

void add_circle(
    Scene &scene, int x, int y, int radius, ColorRole color, bool filled) {
  scene.push_back(
      {filled ? CommandKind::FILLED_CIRCLE : CommandKind::CIRCLE,
       static_cast<int16_t>(x), static_cast<int16_t>(y), 0, 0,
       static_cast<int16_t>(radius), color, FontRole::SMALL, TextAlign::LEFT, ""});
}

std::string normalized(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  std::replace(value.begin(), value.end(), '_', '-');
  return value;
}

bool contains(const std::string &value, const char *needle) {
  return value.find(needle) != std::string::npos;
}

std::string ellipsize(const std::string &value, size_t limit) {
  if (value.size() <= limit) {
    return value;
  }
  return value.substr(0, limit - 3U) + "...";
}

void draw_sun(Scene &scene, int x, int y) {
  add_circle(scene, x, y, 7, ColorRole::SUN, false);
  for (int offset = -12; offset <= 12; offset += 24) {
    add_line(scene, x + offset, y, x + offset / 2, y, ColorRole::SUN);
    add_line(scene, x, y + offset, x, y + offset / 2, ColorRole::SUN);
  }
  add_line(scene, x - 9, y - 9, x - 5, y - 5, ColorRole::SUN);
  add_line(scene, x + 9, y - 9, x + 5, y - 5, ColorRole::SUN);
  add_line(scene, x - 9, y + 9, x - 5, y + 5, ColorRole::SUN);
  add_line(scene, x + 9, y + 9, x + 5, y + 5, ColorRole::SUN);
}

void draw_cloud(Scene &scene, int x, int y) {
  add_circle(scene, x - 7, y, 6, ColorRole::CLOUD, true);
  add_circle(scene, x + 1, y - 5, 9, ColorRole::CLOUD, true);
  add_circle(scene, x + 10, y, 6, ColorRole::CLOUD, true);
  add_rect(scene, x - 13, y, 29, 8, ColorRole::CLOUD, true);
}

void draw_condition_icon(
    Scene &scene, int x, int y, const std::string &condition, bool is_night) {
  const std::string value = normalized(condition);
  if (is_night || contains(value, "night")) {
    add_circle(scene, x, y, 12, ColorRole::ACCENT, true);
    add_circle(scene, x + 6, y - 4, 11, ColorRole::CARD, true);
  } else if (contains(value, "sun") || contains(value, "clear")) {
    draw_sun(scene, x, y);
  } else if (contains(value, "partly")) {
    draw_sun(scene, x - 6, y - 5);
    draw_cloud(scene, x + 3, y + 3);
  } else {
    draw_cloud(scene, x, y - 2);
  }

  if (contains(value, "rain") || contains(value, "pour")) {
    add_line(scene, x - 8, y + 10, x - 11, y + 17, ColorRole::RAIN);
    add_line(scene, x, y + 10, x - 3, y + 17, ColorRole::RAIN);
    add_line(scene, x + 8, y + 10, x + 5, y + 17, ColorRole::RAIN);
  } else if (contains(value, "snow")) {
    add_line(scene, x - 8, y + 11, x - 8, y + 18, ColorRole::TEXT);
    add_line(scene, x - 11, y + 14, x - 5, y + 14, ColorRole::TEXT);
    add_line(scene, x + 7, y + 11, x + 7, y + 18, ColorRole::TEXT);
    add_line(scene, x + 4, y + 14, x + 10, y + 14, ColorRole::TEXT);
  } else if (contains(value, "lightning")) {
    add_line(scene, x + 2, y + 7, x - 3, y + 15, ColorRole::WARNING);
    add_line(scene, x - 3, y + 15, x + 2, y + 15, ColorRole::WARNING);
    add_line(scene, x + 2, y + 15, x - 3, y + 22, ColorRole::WARNING);
  }
}

void add_card(Scene &scene, int y, int height) {
  add_rect(scene, 6, y, 228, height, ColorRole::CARD, true);
  add_rect(scene, 6, y, 3, height, ColorRole::ACCENT, true);
}

void draw_wifi_icon(Scene &scene, bool available, int16_t rssi_dbm) {
  const uint8_t level = wifi_signal_level(available, rssi_dbm);
  const auto level_color = [&](uint8_t required) {
    return available && level >= required ? ColorRole::ACCENT : ColorRole::MUTED;
  };

  const ColorRole outer = level_color(3);
  add_line(scene, 208, 15, 214, 10, outer);
  add_line(scene, 214, 10, 220, 8, outer);
  add_line(scene, 220, 8, 226, 10, outer);
  add_line(scene, 226, 10, 232, 15, outer);

  const ColorRole middle = level_color(2);
  add_line(scene, 212, 19, 216, 15, middle);
  add_line(scene, 216, 15, 220, 14, middle);
  add_line(scene, 220, 14, 224, 15, middle);
  add_line(scene, 224, 15, 228, 19, middle);

  const ColorRole inner = level_color(1);
  add_line(scene, 216, 23, 220, 20, inner);
  add_line(scene, 220, 20, 224, 23, inner);
  add_circle(
      scene, 220, 27, 2, available ? ColorRole::ACCENT : ColorRole::MUTED, true);

  if (!available) {
    add_line(scene, 216, 24, 224, 32, ColorRole::WARNING);
    add_line(scene, 224, 24, 216, 32, ColorRole::WARNING);
  }
}

}  // namespace

std::string format_time(uint8_t hour, uint8_t minute, bool use_24_hour) {
  char result[16];
  if (use_24_hour) {
    std::snprintf(result, sizeof(result), "%02u:%02u", hour, minute);
  } else {
    const uint8_t display_hour = hour % 12U == 0U ? 12U : hour % 12U;
    std::snprintf(
        result, sizeof(result), "%u:%02u %s", display_hour, minute,
        hour < 12U ? "AM" : "PM");
  }
  return result;
}

std::string format_date(
    uint16_t year, uint8_t month, uint8_t day, uint8_t day_of_week) {
  const char *weekday =
      day_of_week >= 1U && day_of_week <= 7U ? WEEKDAYS[day_of_week - 1U] : "---";
  const char *month_name =
      month >= 1U && month <= 12U ? MONTHS[month - 1U] : "---";
  char result[32];
  std::snprintf(result, sizeof(result), "%s, %s %u, %u", weekday, month_name, day, year);
  return result;
}

std::string format_age(uint32_t age_seconds, bool heard) {
  if (!heard) {
    return "waiting";
  }
  char result[32];
  if (age_seconds < 10U) {
    return "now";
  }
  if (age_seconds < 60U) {
    std::snprintf(result, sizeof(result), "%us ago", age_seconds);
  } else if (age_seconds < 3600U) {
    std::snprintf(result, sizeof(result), "%um ago", age_seconds / 60U);
  } else if (age_seconds < 86400U) {
    std::snprintf(
        result, sizeof(result), "%uh %um ago", age_seconds / 3600U,
        (age_seconds % 3600U) / 60U);
  } else {
    std::snprintf(
        result, sizeof(result), "%ud %uh ago", age_seconds / 86400U,
        (age_seconds % 86400U) / 3600U);
  }
  return result;
}

std::string format_station_values(const StationView &station) {
  if (!station.heard) {
    return "--.-°C  --%";
  }
  char result[32];
  const int16_t absolute = station.temperature_tenths_c < 0
                               ? -station.temperature_tenths_c
                               : station.temperature_tenths_c;
  std::snprintf(
      result, sizeof(result), "%s%d.%d°C  %u%%",
      station.temperature_tenths_c < 0 ? "-" : "", absolute / 10,
      absolute % 10, station.humidity_percent);
  return result;
}

std::string humanize_condition(const std::string &condition) {
  if (condition.empty()) {
    return "Weather unavailable";
  }
  const std::string value = normalized(condition);
  if (value == "partlycloudy") {
    return "Partly Cloudy";
  }
  if (value == "lightning-rainy") {
    return "Lightning & Rain";
  }
  if (value == "snowy-rainy") {
    return "Snow & Rain";
  }
  if (value == "windy-variant") {
    return "Windy";
  }
  std::string result = value;
  std::replace(result.begin(), result.end(), '-', ' ');
  std::replace(result.begin(), result.end(), '_', ' ');
  bool new_word = true;
  for (char &character : result) {
    if (character == ' ') {
      new_word = true;
    } else if (new_word) {
      character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
      new_word = false;
    }
  }
  return result;
}

size_t selected_secondary_index(size_t count, uint32_t now_ms) {
  return count == 0U ? 0U : (now_ms / 2000U) % count;
}

uint8_t wifi_signal_level(bool available, int16_t rssi_dbm) {
  if (!available) {
    return 0;
  }
  if (rssi_dbm >= -55) {
    return 3;
  }
  if (rssi_dbm >= -67) {
    return 2;
  }
  if (rssi_dbm >= -78) {
    return 1;
  }
  return 0;
}

Scene build_scene(const ScreenSnapshot &snapshot, const LayoutOptions &options) {
  Scene scene;
  scene.reserve(64);
  add_rect(scene, 0, 0, 240, 240, ColorRole::BACKGROUND, true);
  if (options.show_network) {
    draw_wifi_icon(scene, snapshot.wifi_valid, snapshot.wifi_dbm);
  }
  int y = 5;

  if (options.show_time) {
    add_text(
        scene, 120, y, FontRole::LARGE, ColorRole::TEXT, TextAlign::CENTER,
        snapshot.time_valid
            ? format_time(snapshot.hour, snapshot.minute, options.use_24_hour)
            : "--:--");
    y += 35;
  }
  if (options.show_date) {
    add_text(
        scene, 120, y, FontRole::SMALL, ColorRole::MUTED, TextAlign::CENTER,
        snapshot.time_valid
            ? format_date(
                  snapshot.year, snapshot.month, snapshot.day, snapshot.day_of_week)
            : "Waiting for Home Assistant time");
    y += 19;
  }
  if (options.show_condition) {
    add_card(scene, y, 38);
    draw_condition_icon(scene, 28, y + 18, snapshot.condition, snapshot.is_night);
    add_text(
        scene, 51, y + 10, FontRole::MEDIUM, ColorRole::TEXT, TextAlign::LEFT,
        ellipsize(humanize_condition(snapshot.condition), 21));
    y += 43;
  }
  if (options.show_primary) {
    add_card(scene, y, 45);
    add_text(
        scene, 14, y + 5, FontRole::SMALL, ColorRole::ACCENT, TextAlign::LEFT,
        snapshot.primary.name.empty()
            ? "Primary station"
            : ellipsize(snapshot.primary.name, 18));
    add_text(
        scene, 226, y + 5, FontRole::SMALL, ColorRole::MUTED, TextAlign::RIGHT,
        format_age(snapshot.primary.age_seconds, snapshot.primary.heard));
    add_text(
        scene, 120, y + 21, FontRole::LARGE, ColorRole::TEXT, TextAlign::CENTER,
        format_station_values(snapshot.primary));
    y += 50;
  }
  if (options.show_secondary && !snapshot.secondaries.empty()) {
    const auto &secondary =
        snapshot.secondaries[selected_secondary_index(
            snapshot.secondaries.size(), snapshot.now_ms)];
    add_card(scene, y, 25);
    add_text(
        scene, 14, y + 6, FontRole::SMALL, ColorRole::MUTED, TextAlign::LEFT,
        ellipsize(secondary.name, 10));
    add_text(
        scene, 226, y + 6, FontRole::SMALL, ColorRole::TEXT, TextAlign::RIGHT,
        format_station_values(secondary) + " · " +
            format_age(secondary.age_seconds, secondary.heard));
    y += 30;
  }
  if (options.show_sun) {
    add_text(
        scene, 14, y + 4, FontRole::SMALL, ColorRole::SUN, TextAlign::LEFT,
        "Rise " + (snapshot.sunrise.empty() ? "--:--" : snapshot.sunrise));
    add_text(
        scene, 226, y + 4, FontRole::SMALL, ColorRole::ACCENT, TextAlign::RIGHT,
        "Set " + (snapshot.sunset.empty() ? "--:--" : snapshot.sunset));
    y += 22;
  }
  if (options.show_network) {
    char network[80];
    std::snprintf(
        network, sizeof(network), "WiFi %s  ·  %s",
        snapshot.wifi_valid ? (std::to_string(snapshot.wifi_dbm) + " dBm").c_str()
                            : "-- dBm",
        snapshot.ip_address.empty() ? "no IP" : snapshot.ip_address.c_str());
    add_text(
        scene, 120, std::min(y + 3, 226), FontRole::SMALL, ColorRole::MUTED,
        TextAlign::CENTER, network);
  }
  return scene;
}

const char *color_hex(ColorRole color) {
  switch (color) {
    case ColorRole::BACKGROUND:
      return "#07111f";
    case ColorRole::CARD:
      return "#10243a";
    case ColorRole::TEXT:
      return "#f4f7fb";
    case ColorRole::MUTED:
      return "#8ca5bc";
    case ColorRole::ACCENT:
      return "#36d1c4";
    case ColorRole::SUN:
      return "#ffc857";
    case ColorRole::CLOUD:
      return "#c4d3df";
    case ColorRole::RAIN:
      return "#58a6ff";
    case ColorRole::WARNING:
      return "#ff6b6b";
  }
  return "#ffffff";
}

}  // namespace weather_station_display
