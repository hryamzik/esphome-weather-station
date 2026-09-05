// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <cstdint>

namespace weather_station_decoder {

struct Oregon2Reading {
  uint16_t sensor_model{0};
  uint8_t channel{0};
  uint8_t rolling_code{0};
  bool battery_low{false};
  int16_t temperature_tenths_c{0};
  uint8_t humidity_percent{0};
  uint32_t raw_fixed_data{0};
  uint32_t raw_variable_data{0};
};

class Oregon2Decoder {
 public:
  bool decode(const int32_t *pulses, size_t pulse_count, Oregon2Reading &result) const;
};

}  // namespace weather_station_decoder
