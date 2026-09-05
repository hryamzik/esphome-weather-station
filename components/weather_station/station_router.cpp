// SPDX-License-Identifier: GPL-3.0-or-later

#include "station_router.h"

#include <cstdio>

namespace weather_station_domain {

bool StationSelector::matches(const StationSelector &observed) const {
  return this->protocol == observed.protocol && this->model == observed.model &&
         this->channel == observed.channel &&
         (!this->has_rolling_code ||
          (observed.has_rolling_code && this->rolling_code == observed.rolling_code));
}

bool selectors_overlap(const StationSelector &left, const StationSelector &right) {
  if (left.protocol != right.protocol || left.model != right.model ||
      left.channel != right.channel) {
    return false;
  }
  return !left.has_rolling_code || !right.has_rolling_code ||
         left.rolling_code == right.rolling_code;
}

uint32_t StationState::age_seconds(uint32_t now_ms) const {
  if (!this->heard) {
    return 0;
  }
  return (now_ms - this->last_seen_ms) / 1000U;
}

bool StationRouter::add_station(const StationDefinition &station) {
  for (const auto &existing : this->stations_) {
    if (existing.id == station.id || selectors_overlap(existing.selector, station.selector)) {
      return false;
    }
  }
  if (station.primary && this->explicit_primary_ != NO_STATION) {
    return false;
  }

  const size_t index = this->stations_.size();
  this->stations_.push_back(station);
  this->states_.emplace_back();
  if (station.primary) {
    this->explicit_primary_ = index;
  }
  return true;
}

void StationRouter::add_ignore(const StationSelector &selector) {
  this->ignores_.push_back(selector);
}

RouteResult StationRouter::route(const DecodedReading &reading, uint32_t now_ms) {
  for (const auto &ignored : this->ignores_) {
    if (ignored.matches(reading.identity)) {
      return {RouteKind::IGNORED, NO_STATION};
    }
  }

  for (size_t index = 0; index < this->stations_.size(); index++) {
    if (!this->stations_[index].selector.matches(reading.identity)) {
      continue;
    }
    auto &state = this->states_[index];
    state.heard = true;
    state.last_seen_ms = now_ms;
    state.reading = reading;
    if (this->explicit_primary_ == NO_STATION && this->fallback_primary_ == NO_STATION) {
      this->fallback_primary_ = index;
    }
    return {RouteKind::CONFIGURED, index};
  }

  this->last_unknown_yaml_ = selector_yaml_(reading.identity);
  bool known_unknown = false;
  for (auto &unknown : this->unknowns_) {
    if (unknown.selector.protocol == reading.identity.protocol &&
        unknown.selector.model == reading.identity.model &&
        unknown.selector.channel == reading.identity.channel &&
        unknown.selector.has_rolling_code == reading.identity.has_rolling_code &&
        (!unknown.selector.has_rolling_code ||
         unknown.selector.rolling_code == reading.identity.rolling_code)) {
      unknown.last_seen_ms = now_ms;
      known_unknown = true;
      break;
    }
  }
  if (!known_unknown) {
    this->unknowns_.push_back({reading.identity, now_ms});
  }
  return {RouteKind::UNKNOWN, NO_STATION};
}

size_t StationRouter::primary_station_index() const {
  return this->explicit_primary_ != NO_STATION ? this->explicit_primary_
                                               : this->fallback_primary_;
}

size_t StationRouter::recent_unknown_count(uint32_t now_ms, uint32_t window_ms) {
  this->prune_unknowns_(now_ms, window_ms);
  return this->unknowns_.size();
}

void StationRouter::prune_unknowns_(uint32_t now_ms, uint32_t window_ms) {
  size_t write = 0;
  for (size_t read = 0; read < this->unknowns_.size(); read++) {
    if (now_ms - this->unknowns_[read].last_seen_ms <= window_ms) {
      if (write != read) {
        this->unknowns_[write] = this->unknowns_[read];
      }
      write++;
    }
  }
  this->unknowns_.resize(write);
}

std::string StationRouter::selector_yaml_(const StationSelector &selector) {
  const char *protocol = selector.protocol == Protocol::OREGON2 ? "oregon2" : "unknown";
  char yaml[160];
  if (selector.has_rolling_code) {
    std::snprintf(
        yaml,
        sizeof(yaml),
        "selector:\n  protocol: %s\n  model: 0x%04X\n  channel: %u\n"
        "  rolling_code: 0x%02X",
        protocol,
        selector.model,
        selector.channel,
        selector.rolling_code);
  } else {
    std::snprintf(
        yaml,
        sizeof(yaml),
        "selector:\n  protocol: %s\n  model: 0x%04X\n  channel: %u",
        protocol,
        selector.model,
        selector.channel);
  }
  return yaml;
}

DecodedReading from_oregon2(
    uint16_t model,
    uint8_t channel,
    uint8_t rolling_code,
    int16_t temperature_tenths_c,
    uint8_t humidity_percent,
    bool battery_low) {
  DecodedReading reading;
  reading.identity = {
      Protocol::OREGON2, model, channel, true, rolling_code};
  reading.capabilities = CAP_TEMPERATURE | CAP_HUMIDITY | CAP_CHANNEL |
                         CAP_ROLLING_CODE | CAP_BATTERY_LOW;
  reading.temperature_tenths_c = temperature_tenths_c;
  reading.humidity_percent = humidity_percent;
  reading.battery_low = battery_low;
  return reading;
}

}  // namespace weather_station_domain
