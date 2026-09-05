// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace weather_station_domain {

enum class Protocol : uint8_t {
  OREGON2 = 0,
};

enum ReadingCapability : uint8_t {
  CAP_TEMPERATURE = 1U << 0U,
  CAP_HUMIDITY = 1U << 1U,
  CAP_CHANNEL = 1U << 2U,
  CAP_ROLLING_CODE = 1U << 3U,
  CAP_BATTERY_LOW = 1U << 4U,
};

struct StationSelector {
  Protocol protocol{Protocol::OREGON2};
  uint16_t model{0};
  uint8_t channel{0};
  bool has_rolling_code{false};
  uint8_t rolling_code{0};

  bool matches(const StationSelector &observed) const;
};

bool selectors_overlap(const StationSelector &left, const StationSelector &right);

struct DecodedReading {
  StationSelector identity;
  uint8_t capabilities{0};
  int16_t temperature_tenths_c{0};
  uint8_t humidity_percent{0};
  bool battery_low{false};
};

struct StationDefinition {
  std::string id;
  std::string name;
  StationSelector selector;
  bool primary{false};
};

struct StationState {
  bool heard{false};
  uint32_t last_seen_ms{0};
  DecodedReading reading;

  uint32_t age_seconds(uint32_t now_ms) const;
};

enum class RouteKind : uint8_t {
  IGNORED,
  CONFIGURED,
  UNKNOWN,
};

struct RouteResult {
  RouteKind kind{RouteKind::UNKNOWN};
  size_t station_index{static_cast<size_t>(-1)};
};

class StationRouter {
 public:
  static constexpr size_t NO_STATION = static_cast<size_t>(-1);

  bool add_station(const StationDefinition &station);
  void add_ignore(const StationSelector &selector);
  RouteResult route(const DecodedReading &reading, uint32_t now_ms);

  size_t station_count() const { return this->stations_.size(); }
  const StationDefinition &station(size_t index) const { return this->stations_[index]; }
  const StationState &state(size_t index) const { return this->states_[index]; }
  size_t primary_station_index() const;

  const std::string &last_unknown_yaml() const { return this->last_unknown_yaml_; }
  size_t recent_unknown_count(uint32_t now_ms, uint32_t window_ms);

 private:
  struct UnknownState {
    StationSelector selector;
    uint32_t last_seen_ms;
  };

  void prune_unknowns_(uint32_t now_ms, uint32_t window_ms);
  static std::string selector_yaml_(const StationSelector &selector);

  std::vector<StationDefinition> stations_;
  std::vector<StationState> states_;
  std::vector<StationSelector> ignores_;
  std::vector<UnknownState> unknowns_;
  std::string last_unknown_yaml_;
  size_t explicit_primary_{NO_STATION};
  size_t fallback_primary_{NO_STATION};
};

DecodedReading from_oregon2(
    uint16_t model,
    uint8_t channel,
    uint8_t rolling_code,
    int16_t temperature_tenths_c,
    uint8_t humidity_percent,
    bool battery_low);

}  // namespace weather_station_domain
