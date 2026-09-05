// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "oregon2_decoder.h"
#include "station_router.h"

namespace {

std::string field(const std::string &content, const std::string &name) {
  const std::string prefix = name + ": ";
  const size_t start = content.find(prefix);
  assert(start != std::string::npos);
  const size_t value_start = start + prefix.size();
  const size_t end = content.find('\n', value_start);
  return content.substr(value_start, end - value_start);
}

std::vector<int32_t> load_binraw(const std::string &path) {
  std::ifstream input(path);
  assert(input.good());
  const std::string content{
      std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
  const size_t bit_count = std::stoul(field(content, "Bit_RAW"));
  const int32_t te = std::stoi(field(content, "TE"));

  std::istringstream data(field(content, "Data_RAW"));
  std::vector<uint8_t> bytes;
  std::string byte;
  while (data >> byte) {
    bytes.push_back(static_cast<uint8_t>(std::stoul(byte, nullptr, 16)));
  }

  std::vector<int32_t> pulses;
  int previous = -1;
  size_t run = 0;
  for (size_t index = 0; index < bit_count; index++) {
    const int level = (bytes[index / 8] >> (7 - index % 8)) & 1;
    if (previous != -1 && level != previous) {
      pulses.push_back((previous ? 1 : -1) * static_cast<int32_t>(run * te));
      run = 0;
    }
    previous = level;
    run++;
  }
  if (run != 0) {
    pulses.push_back((previous ? 1 : -1) * static_cast<int32_t>(run * te));
  }
  return pulses;
}

}  // namespace

int main(int argc, char **argv) {
  assert(argc == 2);
  const auto pulses = load_binraw(argv[1]);
  weather_station_decoder::Oregon2Decoder decoder;
  weather_station_decoder::Oregon2Reading reading;

  assert(decoder.decode(pulses.data(), pulses.size(), reading));
  assert(reading.sensor_model == 0x1D20);
  assert(reading.channel == 1);
  assert(reading.rolling_code == 0x8B);
  assert(!reading.battery_low);
  assert(reading.temperature_tenths_c == 303);
  assert(reading.humidity_percent == 39);
  assert(reading.raw_fixed_data == 0x1D2018B0);

  using namespace weather_station_domain;
  StationRouter router;
  assert(router.add_station(
      {"greenhouse", "Greenhouse", {Protocol::OREGON2, 0x1D20, 2, true, 0x42}, false}));
  assert(router.add_station(
      {"garden", "Garden", {Protocol::OREGON2, 0x1D20, 1, false, 0}, false}));
  assert(!router.add_station(
      {"ambiguous", "Ambiguous", {Protocol::OREGON2, 0x1D20, 1, true, 0x8B}, false}));
  assert(!router.add_station(
      {"garden", "Duplicate ID", {Protocol::OREGON2, 0x1D20, 3, false, 0}, false}));

  const auto fixture_reading = from_oregon2(
      reading.sensor_model,
      reading.channel,
      reading.rolling_code,
      reading.temperature_tenths_c,
      reading.humidity_percent,
      reading.battery_low);
  const auto garden_route = router.route(fixture_reading, 1000);
  assert(garden_route.kind == RouteKind::CONFIGURED);
  assert(garden_route.station_index == 1);
  assert(router.primary_station_index() == 1);
  assert(router.state(1).heard);
  assert(router.state(1).age_seconds(6500) == 5);

  const auto greenhouse = from_oregon2(0x1D20, 2, 0x42, 215, 45, false);
  const auto greenhouse_route = router.route(greenhouse, 7000);
  assert(greenhouse_route.kind == RouteKind::CONFIGURED);
  assert(greenhouse_route.station_index == 0);
  assert(router.primary_station_index() == 1);

  router.add_ignore({Protocol::OREGON2, 0x1D20, 2, true, 0x42});
  assert(router.route(greenhouse, 8000).kind == RouteKind::IGNORED);
  assert(router.state(0).last_seen_ms == 7000);

  const auto unknown = from_oregon2(0x1D20, 3, 0x77, 199, 50, false);
  assert(router.route(unknown, 10000).kind == RouteKind::UNKNOWN);
  assert(
      router.last_unknown_yaml() ==
      "selector:\n  protocol: oregon2\n  model: 0x1D20\n  channel: 3\n"
      "  rolling_code: 0x77");
  assert(router.route(unknown, 11000).kind == RouteKind::UNKNOWN);
  assert(router.recent_unknown_count(12000, 5000) == 1);
  assert(router.recent_unknown_count(16000, 5000) == 1);
  assert(router.recent_unknown_count(16001, 5000) == 0);

  StationRouter explicit_primary;
  assert(explicit_primary.add_station(
      {"one", "One", {Protocol::OREGON2, 0x1D20, 1, false, 0}, true}));
  assert(explicit_primary.add_station(
      {"two", "Two", {Protocol::OREGON2, 0x1D20, 2, false, 0}, false}));
  assert(explicit_primary.primary_station_index() == 0);
  assert(!explicit_primary.add_station(
      {"three", "Three", {Protocol::OREGON2, 0x1D20, 3, false, 0}, true}));

  auto inverted_pulses = pulses;
  for (auto &pulse : inverted_pulses) {
    pulse = -pulse;
  }
  assert(decoder.decode(inverted_pulses.data(), inverted_pulses.size(), reading));
  assert(reading.temperature_tenths_c == 303);
  assert(reading.humidity_percent == 39);

  auto rmt_shaped_pulses = pulses;
  rmt_shaped_pulses.push_back(-10000);
  assert(decoder.decode(rmt_shaped_pulses.data(), rmt_shaped_pulses.size(), reading));
  assert(reading.temperature_tenths_c == 303);

  const int32_t noise[] = {100, -120, 3000, -50};
  assert(!decoder.decode(noise, 4, reading));
}
