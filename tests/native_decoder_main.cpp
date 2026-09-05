// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "oregon2_decoder.h"

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
