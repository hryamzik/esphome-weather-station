// SPDX-License-Identifier: GPL-3.0-or-later

#include "display_layout.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace weather_station_display {
namespace {

const char *const WEEKDAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *const MONTHS[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

struct CommandStream {
  DrawCommandCallback callback;
  void *context;

  bool push(const DrawCommand &command) {
    return this->callback(this->context, command, nullptr);
  }
  bool push_text(const DrawCommand &command, const char *text) {
    return this->callback(this->context, command, text);
  }
};

void add_text(
    CommandStream &scene,
    int x,
    int y,
    FontRole font,
    ColorRole color,
    TextAlign align,
    const char *text) {
  DrawCommand command;
  command.kind = CommandKind::TEXT;
  command.x1 = static_cast<int16_t>(x);
  command.y1 = static_cast<int16_t>(y);
  command.color = color;
  command.font = font;
  command.align = align;
  scene.push_text(command, text);
}

void add_line(
    CommandStream &scene, int x1, int y1, int x2, int y2, ColorRole color) {
  DrawCommand command;
  command.kind = CommandKind::LINE;
  command.x1 = static_cast<int16_t>(x1);
  command.y1 = static_cast<int16_t>(y1);
  command.x2 = static_cast<int16_t>(x2);
  command.y2 = static_cast<int16_t>(y2);
  command.color = color;
  scene.push(command);
}

void add_rect(
    CommandStream &scene,
    int x,
    int y,
    int width,
    int height,
    ColorRole color,
    bool filled) {
  DrawCommand command;
  command.kind =
      filled ? CommandKind::FILLED_RECTANGLE : CommandKind::RECTANGLE;
  command.x1 = static_cast<int16_t>(x);
  command.y1 = static_cast<int16_t>(y);
  command.x2 = static_cast<int16_t>(width);
  command.y2 = static_cast<int16_t>(height);
  command.color = color;
  scene.push(command);
}

void add_circle(
    CommandStream &scene,
    int x,
    int y,
    int radius,
    ColorRole color,
    bool filled) {
  DrawCommand command;
  command.kind = filled ? CommandKind::FILLED_CIRCLE : CommandKind::CIRCLE;
  command.x1 = static_cast<int16_t>(x);
  command.y1 = static_cast<int16_t>(y);
  command.radius = static_cast<int16_t>(radius);
  command.color = color;
  scene.push(command);
}

void normalize(const char *input, char *output, size_t output_size) {
  if (output_size == 0U) {
    return;
  }
  size_t index = 0;
  while (input != nullptr && input[index] != '\0' && index + 1U < output_size) {
    const unsigned char character = static_cast<unsigned char>(input[index]);
    output[index] =
        character == '_' ? '-' : static_cast<char>(std::tolower(character));
    index++;
  }
  output[index] = '\0';
}

bool contains(const char *value, const char *needle) {
  return std::strstr(value, needle) != nullptr;
}

void ellipsize(
    const char *value, size_t limit, char *output, size_t output_size) {
  if (output_size == 0U) {
    return;
  }
  const size_t length = std::strlen(value);
  if (length <= limit) {
    std::snprintf(output, output_size, "%s", value);
    return;
  }
  const size_t prefix = limit > 3U ? limit - 3U : 0U;
  std::snprintf(output, output_size, "%.*s...", static_cast<int>(prefix), value);
}

void draw_sun(CommandStream &scene, int x, int y) {
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

void draw_cloud(CommandStream &scene, int x, int y) {
  add_circle(scene, x - 7, y, 6, ColorRole::CLOUD, true);
  add_circle(scene, x + 1, y - 5, 9, ColorRole::CLOUD, true);
  add_circle(scene, x + 10, y, 6, ColorRole::CLOUD, true);
  add_rect(scene, x - 13, y, 29, 8, ColorRole::CLOUD, true);
}

void draw_condition_icon(
    CommandStream &scene, int x, int y, const char *condition, bool is_night) {
  char value[32];
  normalize(condition, value, sizeof(value));
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

void add_card(CommandStream &scene, int y, int height) {
  add_rect(scene, 6, y, 228, height, ColorRole::CARD, true);
  add_rect(scene, 6, y, 3, height, ColorRole::ACCENT, true);
}

void draw_wifi_icon(
    CommandStream &scene, bool available, int16_t rssi_dbm) {
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
    add_line(scene, 207, 7, 233, 33, ColorRole::WARNING);
    add_line(scene, 206, 8, 232, 34, ColorRole::WARNING);
    add_line(scene, 208, 6, 234, 32, ColorRole::WARNING);
    add_line(scene, 233, 7, 207, 33, ColorRole::WARNING);
    add_line(scene, 234, 8, 208, 34, ColorRole::WARNING);
    add_line(scene, 232, 6, 206, 32, ColorRole::WARNING);
  }
}

}  // namespace

void format_time(
    uint8_t hour,
    uint8_t minute,
    bool use_24_hour,
    char *output,
    size_t output_size) {
  if (use_24_hour) {
    std::snprintf(output, output_size, "%02u:%02u", hour, minute);
  } else {
    const uint8_t display_hour = hour % 12U == 0U ? 12U : hour % 12U;
    std::snprintf(
        output, output_size, "%u:%02u %s", display_hour, minute,
        hour < 12U ? "AM" : "PM");
  }
}

void format_date(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t day_of_week,
    char *output,
    size_t output_size) {
  const char *weekday =
      day_of_week >= 1U && day_of_week <= 7U ? WEEKDAYS[day_of_week - 1U] : "---";
  const char *month_name =
      month >= 1U && month <= 12U ? MONTHS[month - 1U] : "---";
  std::snprintf(
      output, output_size, "%s, %s %u, %u", weekday, month_name, day, year);
}

void format_age(
    uint32_t age_seconds, bool heard, char *output, size_t output_size) {
  if (!heard) {
    std::snprintf(output, output_size, "waiting");
  } else if (age_seconds < 10U) {
    std::snprintf(output, output_size, "now");
  } else if (age_seconds < 60U) {
    std::snprintf(output, output_size, "%us ago", age_seconds);
  } else if (age_seconds < 3600U) {
    std::snprintf(output, output_size, "%um ago", age_seconds / 60U);
  } else if (age_seconds < 86400U) {
    std::snprintf(
        output, output_size, "%uh %um ago", age_seconds / 3600U,
        (age_seconds % 3600U) / 60U);
  } else {
    std::snprintf(
        output, output_size, "%ud %uh ago", age_seconds / 86400U,
        (age_seconds % 86400U) / 3600U);
  }
}

void format_station_values(
    const StationView &station, char *output, size_t output_size) {
  if (!station.heard) {
    std::snprintf(output, output_size, "--.-°C  --%%");
    return;
  }
  const int32_t temperature = station.temperature_tenths_c;
  const int32_t absolute = temperature < 0 ? -temperature : temperature;
  std::snprintf(
      output, output_size, "%s%d.%d°C  %u%%", temperature < 0 ? "-" : "",
      static_cast<int>(absolute / 10), static_cast<int>(absolute % 10),
      station.humidity_percent);
}

void humanize_condition(
    const char *condition, char *output, size_t output_size) {
  if (condition == nullptr || condition[0] == '\0') {
    std::snprintf(output, output_size, "Weather unavailable");
    return;
  }
  char value[32];
  normalize(condition, value, sizeof(value));
  if (std::strcmp(value, "partlycloudy") == 0) {
    std::snprintf(output, output_size, "Partly Cloudy");
    return;
  }
  if (std::strcmp(value, "lightning-rainy") == 0) {
    std::snprintf(output, output_size, "Lightning & Rain");
    return;
  }
  if (std::strcmp(value, "snowy-rainy") == 0) {
    std::snprintf(output, output_size, "Snow & Rain");
    return;
  }
  if (std::strcmp(value, "windy-variant") == 0) {
    std::snprintf(output, output_size, "Windy");
    return;
  }

  size_t index = 0;
  bool new_word = true;
  while (value[index] != '\0' && index + 1U < output_size) {
    char character = value[index];
    if (character == '-') {
      character = ' ';
      new_word = true;
    } else if (new_word) {
      character = static_cast<char>(
          std::toupper(static_cast<unsigned char>(character)));
      new_word = false;
    }
    output[index] = character;
    index++;
  }
  output[index] = '\0';
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

void build_scene_impl(
    const ScreenSnapshot &snapshot,
    const LayoutOptions &options,
    CommandStream &scene) {
  add_rect(scene, 0, 0, 240, 240, ColorRole::BACKGROUND, true);
  if (options.show_network) {
    draw_wifi_icon(scene, snapshot.wifi_valid, snapshot.wifi_dbm);
  }
  int y = 5;
  char text[80];

  if (options.show_time) {
    if (snapshot.time_valid) {
      format_time(
          snapshot.hour, snapshot.minute, options.use_24_hour, text, sizeof(text));
    } else {
      std::snprintf(text, sizeof(text), "--:--");
    }
    add_text(
        scene, 120, y, FontRole::LARGE, ColorRole::TEXT, TextAlign::CENTER, text);
    y += 35;
  }
  if (options.show_date) {
    if (snapshot.time_valid) {
      format_date(
          snapshot.year, snapshot.month, snapshot.day, snapshot.day_of_week, text,
          sizeof(text));
    } else {
      std::snprintf(text, sizeof(text), "Waiting for Home Assistant time");
    }
    add_text(
        scene, 120, y, FontRole::SMALL, ColorRole::MUTED, TextAlign::CENTER, text);
    y += 19;
  }
  if (options.show_condition) {
    add_card(scene, y, 38);
    draw_condition_icon(
        scene, 28, y + 18, snapshot.condition.c_str(), snapshot.is_night);
    char condition[32];
    humanize_condition(snapshot.condition.c_str(), text, sizeof(text));
    ellipsize(text, 21, condition, sizeof(condition));
    add_text(
        scene, 51, y + 10, FontRole::MEDIUM, ColorRole::TEXT, TextAlign::LEFT,
        condition);
    y += 43;
  }
  if (options.show_primary) {
    add_card(scene, y, 45);
    char name[24];
    ellipsize(
        snapshot.primary.name.empty() ? "Primary station"
                                      : snapshot.primary.name.c_str(),
        18, name, sizeof(name));
    add_text(
        scene, 14, y + 5, FontRole::SMALL, ColorRole::ACCENT, TextAlign::LEFT,
        name);
    format_age(
        snapshot.primary.age_seconds, snapshot.primary.heard, text, sizeof(text));
    add_text(
        scene, 226, y + 5, FontRole::SMALL, ColorRole::MUTED, TextAlign::RIGHT,
        text);
    format_station_values(snapshot.primary, text, sizeof(text));
    add_text(
        scene, 120, y + 21, FontRole::LARGE, ColorRole::TEXT, TextAlign::CENTER,
        text);
    y += 50;
  }
  if (options.show_secondary && snapshot.secondary_count != 0U) {
    const auto &secondary =
        snapshot.secondaries[selected_secondary_index(
            snapshot.secondary_count, snapshot.now_ms)];
    add_card(scene, y, 25);
    char name[24];
    ellipsize(secondary.name.c_str(), 10, name, sizeof(name));
    add_text(
        scene, 14, y + 6, FontRole::SMALL, ColorRole::MUTED, TextAlign::LEFT,
        name);
    char values[32];
    char age[32];
    format_station_values(secondary, values, sizeof(values));
    format_age(secondary.age_seconds, secondary.heard, age, sizeof(age));
    std::snprintf(text, sizeof(text), "%s · %s", values, age);
    add_text(
        scene, 226, y + 6, FontRole::SMALL, ColorRole::TEXT, TextAlign::RIGHT,
        text);
    y += 30;
  }
  if (options.show_sun) {
    const char *left_label = snapshot.is_night ? "Set" : "Rise";
    const char *left_value =
        snapshot.is_night ? snapshot.sunset.c_str() : snapshot.sunrise.c_str();
    const char *right_label = snapshot.is_night ? "Rise" : "Set";
    const char *right_value =
        snapshot.is_night ? snapshot.sunrise.c_str() : snapshot.sunset.c_str();
    std::snprintf(
        text, sizeof(text), "%s %s", left_label,
        left_value[0] == '\0' ? "--:--" : left_value);
    add_text(
        scene, 14, y + 4, FontRole::SMALL,
        snapshot.is_night ? ColorRole::ACCENT : ColorRole::SUN, TextAlign::LEFT,
        text);
    std::snprintf(
        text, sizeof(text), "%s %s", right_label,
        right_value[0] == '\0' ? "--:--" : right_value);
    add_text(
        scene, 226, y + 4, FontRole::SMALL,
        snapshot.is_night ? ColorRole::SUN : ColorRole::ACCENT,
        TextAlign::RIGHT, text);
    if (snapshot.sun_progress_valid) {
      constexpr int track_x = 79;
      constexpr int track_width = 82;
      constexpr int inner_width = track_width - 2;
      const int elapsed =
          inner_width * snapshot.sun_progress_percent / 100U;
      const ColorRole progress_color =
          snapshot.is_night ? ColorRole::ACCENT : ColorRole::SUN;
      add_rect(
          scene, track_x, y + 9, track_width, 5, ColorRole::MUTED, false);
      if (elapsed > 0) {
        add_rect(
            scene, track_x + 1, y + 10, elapsed, 3, progress_color, true);
      }
      add_circle(
          scene, track_x + 1 + elapsed, y + 11, 2, ColorRole::TEXT, true);
    }
    y += 22;
  }
  if (options.show_network) {
    if (snapshot.wifi_valid) {
      std::snprintf(
          text, sizeof(text), "WiFi %d dBm  ·  %s", snapshot.wifi_dbm,
          snapshot.ip_address.empty() ? "no IP" : snapshot.ip_address.c_str());
    } else {
      std::snprintf(
          text, sizeof(text), "WiFi -- dBm  ·  %s",
          snapshot.ip_address.empty() ? "no IP" : snapshot.ip_address.c_str());
    }
    const int network_y = y + 3 < 226 ? y + 3 : 226;
    add_text(
        scene, 120, network_y, FontRole::SMALL, ColorRole::MUTED,
        TextAlign::CENTER, text);
  }
}

void emit_scene(
    const ScreenSnapshot &snapshot,
    const LayoutOptions &options,
    DrawCommandCallback callback,
    void *context) {
  CommandStream stream{callback, context};
  build_scene_impl(snapshot, options, stream);
}

void build_scene(
    const ScreenSnapshot &snapshot, const LayoutOptions &options, Scene &scene) {
  scene.clear();
  emit_scene(
      snapshot, options,
      [](void *context, const DrawCommand &command, const char *text) {
        auto &target = *static_cast<Scene *>(context);
        return text == nullptr ? target.push(command)
                               : target.push_text(command, text);
      },
      &scene);
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
