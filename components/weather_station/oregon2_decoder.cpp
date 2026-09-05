// SPDX-License-Identifier: GPL-3.0-or-later
//
// Oregon2 behavior was checked against the Weather Station application in
// flipperdevices/flipperzero-good-faps at commit
// 55328b486971e49078a955a7c086e4463fa6843b. This implementation was written
// independently from published protocol behavior and the verified fixture.

#include "oregon2_decoder.h"

#include <cstdlib>
#include <vector>

namespace weather_station_decoder {
namespace {

constexpr uint32_t OREGON2_PREAMBLE = 0b1111111111111110101;
constexpr uint32_t OREGON2_PREAMBLE_MASK = (1U << 19U) - 1U;
constexpr uint16_t THGR122N_MODEL = 0x1D20;

uint32_t reverse_nibble_bits(uint32_t value) {
  value = ((value & 0x55555555U) << 1U) | ((value & 0xAAAAAAAAU) >> 1U);
  return ((value & 0x33333333U) << 2U) | ((value & 0xCCCCCCCCU) >> 2U);
}

bool bcd_digit(uint32_t value) { return value <= 9U; }

bool decode_fields(uint32_t fixed_raw, uint32_t variable_raw, Oregon2Reading &result) {
  const uint32_t fixed = reverse_nibble_bits(fixed_raw);
  const uint32_t variable = reverse_nibble_bits(variable_raw);
  const uint16_t model = static_cast<uint16_t>(fixed >> 16U);
  if (model != THGR122N_MODEL) {
    return false;
  }

  const uint8_t channel_code = static_cast<uint8_t>((fixed >> 12U) & 0x0FU);
  uint8_t channel = 0;
  if (channel_code == 1U) {
    channel = 1;
  } else if (channel_code == 2U) {
    channel = 2;
  } else if (channel_code == 4U) {
    channel = 3;
  } else {
    return false;
  }

  // THGR122N variable data contains six payload nibbles followed by two
  // integrity/postamble nibbles. Nibbles are transmitted least-significant
  // bit first.
  const uint32_t payload = variable >> 8U;
  const uint8_t humidity_code = static_cast<uint8_t>(payload & 0xFFU);
  const uint16_t temperature_code = static_cast<uint16_t>((payload >> 8U) & 0xFFFFU);
  const uint8_t humidity_tens = humidity_code & 0x0FU;
  const uint8_t humidity_ones = (humidity_code >> 4U) & 0x0FU;
  const uint8_t temperature_tens = (temperature_code >> 4U) & 0x0FU;
  const uint8_t temperature_ones = (temperature_code >> 8U) & 0x0FU;
  const uint8_t temperature_tenths = (temperature_code >> 12U) & 0x0FU;
  const uint8_t temperature_sign = temperature_code & 0x0FU;

  if (!bcd_digit(humidity_tens) || !bcd_digit(humidity_ones) ||
      !bcd_digit(temperature_tens) || !bcd_digit(temperature_ones) ||
      !bcd_digit(temperature_tenths) ||
      (temperature_sign != 0U && temperature_sign != 8U)) {
    return false;
  }

  const uint8_t humidity = static_cast<uint8_t>(humidity_tens * 10U + humidity_ones);
  int16_t temperature = static_cast<int16_t>(
      temperature_tens * 100U + temperature_ones * 10U + temperature_tenths);
  if (temperature_sign == 8U) {
    temperature = -temperature;
  }
  if (humidity > 100U || temperature < -600 || temperature > 700) {
    return false;
  }

  result.sensor_model = model;
  result.channel = channel;
  result.rolling_code = static_cast<uint8_t>((fixed >> 4U) & 0xFFU);
  result.battery_low = (fixed & 0x04U) != 0;
  result.temperature_tenths_c = temperature;
  result.humidity_percent = humidity;
  result.raw_fixed_data = fixed;
  result.raw_variable_data = variable;
  return true;
}

bool decode_slots(const std::vector<uint8_t> &slots, bool inverted, Oregon2Reading &result) {
  for (size_t phase = 0; phase < 4; phase++) {
    uint32_t preamble = 0;
    size_t valid_bits = 0;
    std::vector<uint8_t> decoded;
    decoded.reserve(slots.size() / 4U);

    for (size_t index = phase; index + 3U < slots.size(); index += 4U) {
      uint8_t sample[4];
      bool invalid = false;
      for (size_t offset = 0; offset < 4; offset++) {
        if (slots[index + offset] > 1U) {
          invalid = true;
          break;
        }
        sample[offset] = slots[index + offset] ^ static_cast<uint8_t>(inverted);
      }

      uint8_t bit = 2;
      if (!invalid && sample[0] == 0U && sample[1] == 1U && sample[2] == 1U &&
          sample[3] == 0U) {
        bit = 1;
      } else if (
          !invalid && sample[0] == 1U && sample[1] == 0U && sample[2] == 0U &&
          sample[3] == 1U) {
        bit = 0;
      }
      decoded.push_back(bit);
    }

    for (size_t index = 0; index < decoded.size(); index++) {
      if (decoded[index] > 1U) {
        preamble = 0;
        valid_bits = 0;
        continue;
      }
      preamble = ((preamble << 1U) | decoded[index]) & OREGON2_PREAMBLE_MASK;
      valid_bits++;
      if (valid_bits < 19U || preamble != OREGON2_PREAMBLE) {
        continue;
      }
      if (index + 64U >= decoded.size()) {
        continue;
      }

      uint32_t fixed_raw = 0;
      uint32_t variable_raw = 0;
      bool payload_valid = true;
      for (size_t payload_index = 0; payload_index < 64; payload_index++) {
        const uint8_t payload_bit = decoded[index + 1U + payload_index];
        if (payload_bit > 1U) {
          payload_valid = false;
          break;
        }
        if (payload_index < 32U) {
          fixed_raw = (fixed_raw << 1U) | payload_bit;
        } else {
          variable_raw = (variable_raw << 1U) | payload_bit;
        }
      }
      if (payload_valid && decode_fields(fixed_raw, variable_raw, result)) {
        return true;
      }
    }
  }
  return false;
}

}  // namespace

bool Oregon2Decoder::decode(
    const int32_t *pulses, size_t pulse_count, Oregon2Reading &result) const {
  if (pulses == nullptr || pulse_count == 0) {
    return false;
  }

  std::vector<uint8_t> slots;
  slots.reserve(pulse_count * 2U);
  for (size_t index = 0; index < pulse_count; index++) {
    const uint32_t duration =
        static_cast<uint32_t>(pulses[index] < 0 ? -static_cast<int64_t>(pulses[index])
                                               : pulses[index]);
    if (duration < 150U) {
      continue;
    }

    uint8_t slot_count = 0;
    if (duration <= 700U) {
      slot_count = 1;
    } else if (duration <= 1250U) {
      slot_count = 2;
    } else {
      // Inter-message gaps and incomplete leading pulses break alignment.
      slots.insert(slots.end(), 4U, 2U);
      continue;
    }
    const uint8_t level = pulses[index] > 0 ? 1U : 0U;
    slots.insert(slots.end(), slot_count, level);
  }

  return decode_slots(slots, false, result) || decode_slots(slots, true, result);
}

}  // namespace weather_station_decoder
