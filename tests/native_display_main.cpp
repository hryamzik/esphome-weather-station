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

const weather_station_display::DrawCommand *find_text_at_x(
    const weather_station_display::Scene &scene, const char *text, int x) {
  const auto match = std::find_if(
      scene.begin(), scene.end(), [&](const auto &command) {
        return command.kind == weather_station_display::CommandKind::TEXT &&
               command.x1 == x && std::strcmp(scene.text(command), text) == 0;
      });
  return match == scene.end() ? nullptr : match;
}

size_t sun_progress_commands(const weather_station_display::Scene &scene) {
  return static_cast<size_t>(
      std::count_if(scene.begin(), scene.end(), [](const auto &command) {
        return command.kind != weather_station_display::CommandKind::TEXT &&
               command.x1 >= 79 && command.x1 <= 161 &&
               command.y1 >= 200 && command.y1 <= 212;
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

int estimated_text_width(
    const weather_station_display::DrawCommand &command, const char *text) {
  const bool large =
      command.font == weather_station_display::FontRole::LARGE ||
      command.font == weather_station_display::FontRole::LARGE_REGULAR;
  const int pixels_per_character =
      large ? 11
            : (command.font == weather_station_display::FontRole::MEDIUM ? 8
                                                                         : 5);
  size_t characters = 0;
  for (const auto *byte = reinterpret_cast<const unsigned char *>(text);
       *byte != '\0'; ++byte) {
    if ((*byte & 0xC0U) != 0x80U) {
      ++characters;
    }
  }
  return static_cast<int>(characters) * pixels_per_character;
}

int text_left(
    const weather_station_display::DrawCommand &command, const char *text) {
  const int width = estimated_text_width(command, text);
  if (command.align == weather_station_display::TextAlign::CENTER) {
    return command.x1 - width / 2;
  }
  return command.align == weather_station_display::TextAlign::RIGHT
             ? command.x1 - width
             : command.x1;
}

int text_right(
    const weather_station_display::DrawCommand &command, const char *text) {
  return text_left(command, text) + estimated_text_width(command, text);
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
  format_time(0, 5, false, false, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "12:05") == 0);
  format_time(12, 0, false, true, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "12:00 PM") == 0);
  format_time(23, 7, true, true, formatted, sizeof(formatted));
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

  format_phase_remaining(10, 8, "19:51", formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Left 09:43") == 0);
  format_phase_remaining(22, 14, "06:42", formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Left 08:28") == 0);
  format_phase_remaining(19, 51, "19:51", formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Left 00:00") == 0);
  format_phase_remaining(10, 8, "not-a-time", formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Left --:--") == 0);
  format_phase_remaining(10, 8, nullptr, formatted, sizeof(formatted));
  assert(std::strcmp(formatted, "Left --:--") == 0);

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
  snapshot.weather_temperature_valid = true;
  snapshot.weather_temperature_tenths_c = 184;
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
  assert(scene_contains(scene, "18.4°C"));
  assert(!scene_contains(scene, "WiFi"));
  assert(!scene_contains(scene, "dBm"));
  const auto *date = find_text(scene, "Sat, Sep 5, 2026");
  const auto *ip_address = find_text(scene, "192.168.87.44");
  const auto *day_remaining = find_text(scene, "Left 05:14");
  assert(date != nullptr && date->font == FontRole::MEDIUM);
  assert(
      ip_address != nullptr && ip_address->x1 == 14 &&
      ip_address->y1 == 221 && ip_address->align == TextAlign::LEFT &&
      ip_address->color == ColorRole::MUTED);
  assert(
      day_remaining != nullptr && day_remaining->x1 == 226 &&
      day_remaining->y1 == 221 && day_remaining->align == TextAlign::RIGHT &&
      day_remaining->color == ColorRole::ACCENT);
  assert(
      text_right(*ip_address, scene.text(*ip_address)) <
      text_left(*day_remaining, scene.text(*day_remaining)));
  const auto *day_rise = find_text(scene, "Rise 06:42");
  const auto *day_set = find_text(scene, "Set 19:51");
  assert(day_rise != nullptr && day_rise->x1 == 14);
  assert(day_set != nullptr && day_set->x1 == 226);
  assert(sun_progress_commands(scene) == 3);
  assert(wifi_icon_commands(scene, ColorRole::ACCENT) == 7);
  assert(wifi_icon_commands(scene, ColorRole::MUTED) == 4);

  snapshot.primary.age_seconds = 300;
  snapshot.secondaries[0].age_seconds = 300;
  build_scene(snapshot, options, scene);
  const auto *fresh_primary_values = find_text(scene, "21.3°C  47%");
  const auto *fresh_primary_age = find_text_at_x(scene, "5m ago", 14);
  const auto *fresh_secondary_name = find_text(scene, "Greenhouse");
  const auto *fresh_secondary_values = find_text(scene, "24.6°C  61% ·");
  const auto *fresh_secondary_age = find_text_at_x(scene, "5m ago", 226);
  assert(
      fresh_primary_values != nullptr &&
      fresh_primary_values->color == ColorRole::TEXT);
  assert(
      fresh_primary_age != nullptr &&
      fresh_primary_age->color == ColorRole::MUTED);
  assert(
      fresh_secondary_values != nullptr &&
      fresh_secondary_values->color == ColorRole::TEXT);
  assert(
      fresh_secondary_age != nullptr &&
      fresh_secondary_age->color == ColorRole::TEXT);
  assert(fresh_secondary_name != nullptr);
  assert(
      text_right(*fresh_secondary_name, scene.text(*fresh_secondary_name)) <
      text_left(*fresh_secondary_values, scene.text(*fresh_secondary_values)));
  assert(
      text_right(*fresh_secondary_values, scene.text(*fresh_secondary_values)) <
      text_left(*fresh_secondary_age, scene.text(*fresh_secondary_age)));

  snapshot.primary.age_seconds = 301;
  snapshot.secondaries[0].age_seconds = 301;
  build_scene(snapshot, options, scene);
  const auto *stale_primary_values = find_text(scene, "21.3°C  47%");
  const auto *stale_primary_age = find_text_at_x(scene, "5m ago", 14);
  const auto *stale_secondary_values = find_text(scene, "24.6°C  61% ·");
  const auto *stale_secondary_age = find_text_at_x(scene, "5m ago", 226);
  assert(
      stale_primary_values != nullptr &&
      stale_primary_values->color == ColorRole::MUTED);
  assert(
      stale_primary_age != nullptr &&
      stale_primary_age->color == ColorRole::WARNING);
  assert(
      stale_secondary_values != nullptr &&
      stale_secondary_values->color == ColorRole::MUTED);
  assert(
      stale_secondary_age != nullptr &&
      stale_secondary_age->color == ColorRole::WARNING);
  assert(!scene_contains(scene, "STALE"));

  snapshot.primary.heard = false;
  snapshot.secondaries[0].heard = false;
  build_scene(snapshot, options, scene);
  const auto *unheard_primary_values = find_text(scene, "--.-°C  --%");
  const auto *unheard_primary_age = find_text_at_x(scene, "waiting", 14);
  const auto *unheard_secondary_age = find_text_at_x(scene, "waiting", 226);
  assert(
      unheard_primary_values != nullptr &&
      unheard_primary_values->color == ColorRole::TEXT);
  assert(
      unheard_primary_age != nullptr &&
      unheard_primary_age->color == ColorRole::MUTED);
  assert(
      unheard_secondary_age != nullptr &&
      unheard_secondary_age->color == ColorRole::TEXT);
  snapshot.primary = {"Garden", true, 213, 47, 18};
  snapshot.secondaries[0] = {"Greenhouse", true, 246, 61, 75};

  snapshot.weather_temperature_valid = false;
  build_scene(snapshot, options, scene);
  assert(!scene_contains(scene, "18.4°C"));
  snapshot.weather_temperature_valid = true;

  snapshot.now_ms = 2000;
  snapshot.is_night = true;
  snapshot.hour = 22;
  snapshot.minute = 14;
  build_scene(snapshot, options, scene);
  assert(scene_contains(scene, "Balcony"));
  assert(!scene_contains(scene, "Greenhouse"));
  const auto *night_set = find_text(scene, "Set 19:51");
  const auto *night_rise = find_text(scene, "Rise 06:42");
  const auto *night_remaining = find_text(scene, "Left 08:28");
  assert(night_set != nullptr && night_set->x1 == 14);
  assert(night_rise != nullptr && night_rise->x1 == 226);
  assert(
      night_remaining != nullptr &&
      night_remaining->color == ColorRole::SUN &&
      night_remaining->x1 == 226);

  options.show_network = false;
  options.show_sun = false;
  build_scene(snapshot, options, scene);
  assert(!scene_contains(scene, "WiFi"));
  assert(!scene_contains(scene, "Rise"));
  assert(wifi_icon_commands(scene, ColorRole::ACCENT) == 0);
  assert(wifi_icon_commands(scene, ColorRole::MUTED) == 0);

  options.show_network = true;
  snapshot.wifi_valid = false;
  snapshot.ip_address.clear();
  build_scene(snapshot, options, scene);
  assert(scene_contains(scene, "IP unavailable"));
  assert(!scene_contains(scene, "WiFi"));
  assert(!scene_contains(scene, "dBm"));
  assert(wifi_icon_commands(scene, ColorRole::WARNING) == 6);

  snapshot.sun_progress_valid = false;
  build_scene(snapshot, options, scene);
  assert(sun_progress_commands(scene) == 0);

  ScreenSnapshot worst_case = snapshot;
  worst_case.time_valid = true;
  worst_case.hour = 12;
  worst_case.minute = 59;
  worst_case.condition = "partlycloudy";
  worst_case.weather_temperature_valid = true;
  worst_case.weather_temperature_tenths_c = -123;
  worst_case.primary = {
      "Long primary station name", true, -123, 100, 1324800};
  worst_case.wifi_valid = true;
  worst_case.ip_address = "2001:0db8:85a3:0000:0000:8a2e:0370:7334";
  options.show_sun = true;
  options.show_am_pm = true;
  build_scene(worst_case, options, scene);
  const auto *clock = find_text(scene, "12:59 PM");
  const auto *condition = find_text(scene, "Partly Cloudy");
  const auto *weather_temperature = find_text(scene, "-12.3°C");
  const auto *primary_values = find_text(scene, "-12.3°C  100%");
  const auto *primary_age = find_text(scene, "15d 8h ago");
  const auto *long_ip =
      find_text(scene, "2001:0db8:85a3:0000:0000:...");
  const auto *worst_remaining = find_text(scene, "Left 17:43");
  assert(
      clock != nullptr && clock->x1 == 120 &&
      text_right(*clock, scene.text(*clock)) < 206);
  assert(condition != nullptr && weather_temperature != nullptr);
  assert(weather_temperature->font == FontRole::LARGE_REGULAR);
  assert(
      text_right(*condition, scene.text(*condition)) <
      text_left(*weather_temperature, scene.text(*weather_temperature)));
  assert(primary_values != nullptr && primary_age != nullptr);
  assert(primary_age->x1 == 14 && primary_age->y1 > 110);
  assert(
      text_right(*primary_age, scene.text(*primary_age)) <
      text_left(*primary_values, scene.text(*primary_values)));
  assert(long_ip != nullptr && worst_remaining != nullptr);
  assert(
      text_right(*long_ip, scene.text(*long_ip)) <
      text_left(*worst_remaining, scene.text(*worst_remaining)));

  for (const auto &command : scene) {
    assert(command.x1 >= 0 && command.x1 <= 240);
    assert(command.y1 >= 0 && command.y1 <= 240);
    if (command.kind == CommandKind::TEXT) {
      const bool large =
          command.font == FontRole::LARGE ||
          command.font == FontRole::LARGE_REGULAR;
      const int height =
          large ? 29 : (command.font == FontRole::MEDIUM ? 20 : 14);
      assert(text_left(command, scene.text(command)) >= 0);
      assert(text_right(command, scene.text(command)) <= 240);
      assert(command.y1 + height <= 240);
    } else if (
        command.kind == CommandKind::RECTANGLE ||
        command.kind == CommandKind::FILLED_RECTANGLE) {
      assert(command.x1 + command.x2 <= 240);
      assert(command.y1 + command.y2 <= 240);
    } else if (command.kind == CommandKind::LINE) {
      assert(command.x2 >= 0 && command.x2 <= 240);
      assert(command.y2 >= 0 && command.y2 <= 240);
    } else {
      assert(command.x1 - command.radius >= 0);
      assert(command.x1 + command.radius <= 240);
      assert(command.y1 - command.radius >= 0);
      assert(command.y1 + command.radius <= 240);
    }
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
