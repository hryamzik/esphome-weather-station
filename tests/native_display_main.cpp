// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cassert>
#include <string>

#include "display_layout.h"

namespace {

bool scene_contains(
    const weather_station_display::Scene &scene, const std::string &needle) {
  return std::any_of(scene.begin(), scene.end(), [&](const auto &command) {
    return command.text.find(needle) != std::string::npos;
  });
}

size_t wifi_icon_commands(
    const weather_station_display::Scene &scene,
    weather_station_display::ColorRole color) {
  return static_cast<size_t>(std::count_if(scene.begin(), scene.end(), [&](const auto &command) {
    return command.x1 >= 208 && command.y1 <= 32 && command.color == color &&
           command.kind != weather_station_display::CommandKind::TEXT;
  }));
}

}  // namespace

int main() {
  using namespace weather_station_display;

  assert(format_time(0, 5, false) == "12:05 AM");
  assert(format_time(12, 0, false) == "12:00 PM");
  assert(format_time(23, 7, true) == "23:07");
  assert(format_date(2026, 9, 5, 7) == "Sat, Sep 5, 2026");

  assert(format_age(0, false) == "waiting");
  assert(format_age(5, true) == "now");
  assert(format_age(45, true) == "45s ago");
  assert(format_age(720, true) == "12m ago");
  assert(format_age(11580, true) == "3h 13m ago");
  assert(format_age(1324800, true) == "15d 8h ago");
  assert(format_age(1324800, true).find("STALE") == std::string::npos);

  assert(humanize_condition("partly-cloudy") == "Partly Cloudy");
  assert(selected_secondary_index(3, 0) == 0);
  assert(selected_secondary_index(3, 1999) == 0);
  assert(selected_secondary_index(3, 2000) == 1);
  assert(selected_secondary_index(3, 6000) == 0);
  assert(wifi_signal_level(false, -40) == 0);
  assert(wifi_signal_level(true, -80) == 0);
  assert(wifi_signal_level(true, -78) == 1);
  assert(wifi_signal_level(true, -67) == 2);
  assert(wifi_signal_level(true, -55) == 3);

  ScreenSnapshot snapshot;
  snapshot.time_valid = true;
  snapshot.hour = 14;
  snapshot.minute = 37;
  snapshot.year = 2026;
  snapshot.month = 9;
  snapshot.day = 5;
  snapshot.day_of_week = 7;
  snapshot.condition = "partlycloudy";
  snapshot.primary = {"Garden", true, 213, 47, 18};
  snapshot.secondaries = {
      {"Greenhouse", true, 246, 61, 75},
      {"Balcony", true, 198, 52, 245}};
  snapshot.sunrise = "06:42";
  snapshot.sunset = "19:51";
  snapshot.wifi_valid = true;
  snapshot.wifi_dbm = -58;
  snapshot.ip_address = "192.168.87.44";

  LayoutOptions options;
  auto first = build_scene(snapshot, options);
  assert(scene_contains(first, "Greenhouse"));
  assert(!scene_contains(first, "Balcony"));
  assert(scene_contains(first, "Garden"));
  assert(scene_contains(first, "WiFi -58 dBm"));
  assert(wifi_icon_commands(first, ColorRole::ACCENT) == 7);
  assert(wifi_icon_commands(first, ColorRole::MUTED) == 4);

  snapshot.now_ms = 2000;
  auto second = build_scene(snapshot, options);
  assert(scene_contains(second, "Balcony"));
  assert(!scene_contains(second, "Greenhouse"));

  options.show_network = false;
  options.show_sun = false;
  const auto compact = build_scene(snapshot, options);
  assert(!scene_contains(compact, "WiFi"));
  assert(!scene_contains(compact, "Rise"));
  assert(wifi_icon_commands(compact, ColorRole::ACCENT) == 0);
  assert(wifi_icon_commands(compact, ColorRole::MUTED) == 0);

  options.show_network = true;
  snapshot.wifi_valid = false;
  const auto disconnected = build_scene(snapshot, options);
  assert(scene_contains(disconnected, "WiFi -- dBm"));
  assert(wifi_icon_commands(disconnected, ColorRole::WARNING) == 2);

  for (const auto &command : first) {
    assert(command.x1 >= 0 && command.x1 <= 240);
    assert(command.y1 >= 0 && command.y1 <= 240);
  }
}
