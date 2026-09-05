// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>

#include "display_layout.h"

namespace {

size_t allocation_count = 0;

bool scene_contains(
    const weather_station_display::Scene &scene, const char *needle) {
  return std::any_of(scene.begin(), scene.end(), [&](const auto &command) {
    return command.kind == weather_station_display::CommandKind::TEXT &&
           std::strstr(scene.text(command), needle) != nullptr;
  });
}

const weather_station_display::DrawCommand *find_text(
    const weather_station_display::Scene &scene, const char *text) {
  const auto match = std::find_if(scene.begin(), scene.end(), [&](const auto &command) {
    return command.kind == weather_station_display::CommandKind::TEXT &&
           std::strcmp(scene.text(command), text) == 0;
  });
  return match == scene.end() ? nullptr : match;
}

size_t sun_progress_commands(const weather_station_display::Scene &scene) {
  return static_cast<size_t>(
      std::count_if(scene.begin(), scene.end(), [](const auto &command) {
        return command.kind != weather_station_display::CommandKind::TEXT &&
               command.x1 >= 79 && command.x1 <= 161 &&
               command.y1 >= 185 && command.y1 <= 205;
      }));
}

size_t wifi_icon_commands(
    const weather_station_display::Scene &scene,
    weather_station_display::ColorRole color) {
  return static_cast<size_t>(
      std::count_if(scene.begin(), scene.end(), [&](const auto &command) {
        return command.x1 >= 206 && command.y1 <= 34 &&
               command.color == color &&
               command.kind != weather_station_display::CommandKind::TEXT;
      }));
}

}  // namespace

void *operator new(std::size_t size) {
  allocation_count++;
  if (void *pointer = std::malloc(size)) {
    return pointer;
  }
  throw std::bad_alloc();
}

void *operator new[](std::size_t size) {
  allocation_count++;
  if (void *pointer = std::malloc(size)) {
    return pointer;
  }
  throw std::bad_alloc();
}

void operator delete(void *pointer) noexcept { std::free(pointer); }
void operator delete[](void *pointer) noexcept { std::free(pointer); }
void operator delete(void *pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void *pointer, std::size_t) noexcept {
  std::free(pointer);
}

int main() {
  using namespace weather_station_display;

  static_assert(sizeof(Scene) <= 1536U, "scene cache exceeded static RAM budget");
  static_assert(
      sizeof(ScreenSnapshot) <= 512U, "snapshot cache exceeded static RAM budget");

  char formatted[80];
  format_time(0, 5, false, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "12:05 AM") == 0);
  format_time(12, 0, false, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "12:00 PM") == 0);
  format_time(23, 7, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "23:07") == 0);
  format_date(2026, 9, 5, 7, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Sat, Sep 5, 2026") == 0);

  format_age(0, false, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "waiting") == 0);
  format_age(5, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "now") == 0);
  format_age(45, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "45s ago") == 0);
  format_age(720, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "12m ago") == 0);
  format_age(11580, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "3h 13m ago") == 0);
  format_age(1324800, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "15d 8h ago") == 0);
  assert(std::strstr(formatted, "STALE") == nullptr);

  humanize_condition("partly-cloudy", formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Partly Cloudy") == 0);
  assert(selected_secondary_index(3, 0) == 0);
  assert(selected_secondary_index(3, 1999) == 0);
  assert(selected_secondary_index(3, 2000) == 1);
  assert(selected_secondary_index(3, 6000) == 0);
  assert(wifi_signal_level(false, -40) == 0);
  assert(wifi_signal_level(true, -80) == 0);
  assert(wifi_signal_level(true, -78) == 1);
  assert(wifi_signal_level(true, -67) == 2);
  assert(wifi_signal_level(true, -55) == 3);
  bool wifi_ever_connected = false;
  assert(!update_wifi_startup_gate(false, wifi_ever_connected));
  assert(!wifi_ever_connected);
  assert(update_wifi_startup_gate(true, wifi_ever_connected));
  assert(wifi_ever_connected);
  assert(update_wifi_startup_gate(false, wifi_ever_connected));
  assert(wifi_ever_connected);

  FixedText<8> short_text;
  assert(!short_text.assign("123456789"));
  assert(short_text.truncated());
  assert(std::strcmp(short_text.c_str(), "1234567") == 0);

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
  assert(snapshot.add_secondary({"Greenhouse", true, 246, 61, 75}));
  assert(snapshot.add_secondary({"Balcony", true, 198, 52, 245}));
  snapshot.sunrise = "06:42";
  snapshot.sunset = "19:51";
  snapshot.sun_progress_valid = true;
  snapshot.sun_progress_percent = 40;
  snapshot.wifi_valid = true;
  snapshot.wifi_dbm = -58;
  snapshot.ip_address = "192.168.87.44";

  LayoutOptions options;
  Scene scene;
  build_scene(snapshot, options, scene);
  assert(!scene.overflowed());
  assert(scene_contains(scene, "Greenhouse"));
  assert(!scene_contains(scene, "Balcony"));
  assert(scene_contains(scene, "Garden"));
  assert(scene_contains(scene, "WiFi -58 dBm"));
  const auto *day_rise = find_text(scene, "Rise 06:42");
  const auto *day_set = find_text(scene, "Set 19:51");
  assert(day_rise != nullptr && day_rise->x1 == 14);
  assert(day_set != nullptr && day_set->x1 == 226);
  assert(sun_progress_commands(scene) == 3);
  assert(wifi_icon_commands(scene, ColorRole::ACCENT) == 7);
  assert(wifi_icon_commands(scene, ColorRole::MUTED) == 4);

  snapshot.now_ms = 2000;
  snapshot.is_night = true;
  build_scene(snapshot, options, scene);
  assert(scene_contains(scene, "Balcony"));
  assert(!scene_contains(scene, "Greenhouse"));
  const auto *night_set = find_text(scene, "Set 19:51");
  const auto *night_rise = find_text(scene, "Rise 06:42");
  assert(night_set != nullptr && night_set->x1 == 14);
  assert(night_rise != nullptr && night_rise->x1 == 226);

  options.show_network = false;
  options.show_sun = false;
  build_scene(snapshot, options, scene);
  assert(!scene_contains(scene, "WiFi"));
  assert(!scene_contains(scene, "Rise"));
  assert(wifi_icon_commands(scene, ColorRole::ACCENT) == 0);
  assert(wifi_icon_commands(scene, ColorRole::MUTED) == 0);

  options.show_network = true;
  snapshot.wifi_valid = false;
  build_scene(snapshot, options, scene);
  assert(scene_contains(scene, "WiFi -- dBm"));
  assert(wifi_icon_commands(scene, ColorRole::WARNING) == 6);

  snapshot.sun_progress_valid = false;
  build_scene(snapshot, options, scene);
  assert(sun_progress_commands(scene) == 0);

  for (const auto &command : scene) {
    assert(command.x1 >= 0 && command.x1 <= 240);
    assert(command.y1 >= 0 && command.y1 <= 240);
  }

  ScreenSnapshot degraded;
  degraded.primary = {"Garden", true, -54, 83, 90061};
  LayoutOptions degraded_options;
  degraded_options.show_secondary = false;
  degraded_options.show_sun = false;
  degraded_options.show_network = false;
  build_scene(degraded, degraded_options, scene);
  assert(scene_contains(scene, "Waiting for Home Assistant time"));
  assert(scene_contains(scene, "Weather unavailable"));
  assert(scene_contains(scene, "-5.4°C  83%"));
  assert(scene_contains(scene, "1d 1h ago"));

  ScreenSnapshot full_secondaries;
  for (size_t index = 0; index < ScreenSnapshot::MAX_SECONDARIES; index++) {
    assert(full_secondaries.add_secondary({"Station", true, 200, 50, 0}));
  }
  assert(!full_secondaries.add_secondary({"Overflow", true, 200, 50, 0}));
  assert(full_secondaries.secondary_count == ScreenSnapshot::MAX_SECONDARIES);
  assert(full_secondaries.secondary_overflow);

  Scene full_scene;
  DrawCommand command;
  for (size_t index = 0; index < Scene::CAPACITY; index++) {
    assert(full_scene.push(command));
  }
  assert(!full_scene.push(command));
  assert(full_scene.size() == Scene::CAPACITY);
  assert(full_scene.overflowed());

  Scene full_text;
  char oversized[Scene::TEXT_CAPACITY + 1U];
  std::memset(oversized, 'x', sizeof(oversized) - 1U);
  oversized[sizeof(oversized) - 1U] = '\0';
  assert(!full_text.push_text(command, oversized));
  assert(full_text.size() == 0U);
  assert(full_text.overflowed());

  Scene startup_scene;
  build_startup_scene(startup_scene);
  assert(!startup_scene.overflowed());
  assert(startup_scene.size() <= 20U);
  assert(scene_contains(startup_scene, "Connecting WiFi..."));
  assert(wifi_icon_commands(startup_scene, ColorRole::WARNING) == 6U);
  assert(!scene_contains(startup_scene, "Garden"));

  // Setup is complete. Rebuilding the production scene must never call new.
  options = {};
  snapshot.wifi_valid = true;
  const size_t allocations_before = allocation_count;
  for (size_t iteration = 0; iteration < 1000U; iteration++) {
    snapshot.now_ms = static_cast<uint32_t>(iteration * 1000U);
    build_scene(snapshot, options, scene);
    assert(!scene.overflowed());
  }
  assert(allocation_count == allocations_before);

  for (size_t iteration = 0; iteration < 1000U; iteration++) {
    build_startup_scene(startup_scene);
    assert(!startup_scene.overflowed());
  }
  assert(allocation_count == allocations_before);

  size_t emitted_commands = 0;
  for (size_t iteration = 0; iteration < 1000U; iteration++) {
    snapshot.now_ms = static_cast<uint32_t>(iteration * 1000U);
    emitted_commands = 0;
    emit_scene(
        snapshot, options,
        [](void *context, const DrawCommand &, const char *) {
          (*static_cast<size_t *>(context))++;
          return true;
        },
        &emitted_commands);
    assert(emitted_commands != 0U);
  }
  assert(allocation_count == allocations_before);

  for (size_t iteration = 0; iteration < 1000U; iteration++) {
    emitted_commands = 0;
    emit_startup_scene(
        [](void *context, const DrawCommand &, const char *) {
          (*static_cast<size_t *>(context))++;
          return true;
        },
        &emitted_commands);
    assert(emitted_commands <= 20U);
  }
  assert(allocation_count == allocations_before);
}
