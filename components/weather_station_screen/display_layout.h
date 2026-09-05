// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace weather_station_display {

template<size_t Capacity> class FixedText {
 public:
  FixedText() = default;
  FixedText(const char *value) { this->assign(value); }

  FixedText &operator=(const char *value) {
    this->assign(value);
    return *this;
  }

  bool assign(const char *value) {
    if (value == nullptr) {
      this->clear();
      return true;
    }
    const size_t source_length = std::strlen(value);
    const size_t copied = source_length < Capacity - 1U ? source_length : Capacity - 1U;
    std::memcpy(this->data_.data(), value, copied);
    this->data_[copied] = '\0';
    this->length_ = static_cast<uint8_t>(copied);
    this->truncated_ = source_length != copied;
    return !this->truncated_;
  }

  bool append(const char *value) {
    if (value == nullptr) {
      return true;
    }
    const size_t source_length = std::strlen(value);
    const size_t available = Capacity - 1U - this->length_;
    const size_t copied = source_length < available ? source_length : available;
    std::memcpy(this->data_.data() + this->length_, value, copied);
    this->length_ += copied;
    this->data_[this->length_] = '\0';
    this->truncated_ = this->truncated_ || source_length != copied;
    return source_length == copied;
  }

  void clear() {
    this->data_[0] = '\0';
    this->length_ = 0;
    this->truncated_ = false;
  }

  const char *c_str() const { return this->data_.data(); }
  bool empty() const { return this->length_ == 0U; }
  size_t size() const { return this->length_; }
  bool truncated() const { return this->truncated_; }

 private:
  static_assert(Capacity > 1U, "FixedText needs room for a terminator");
  std::array<char, Capacity> data_{};
  uint8_t length_{0};
  bool truncated_{false};
};

enum class CommandKind : uint8_t {
  TEXT,
  LINE,
  RECTANGLE,
  FILLED_RECTANGLE,
  CIRCLE,
  FILLED_CIRCLE,
};

enum class ColorRole : uint8_t {
  BACKGROUND,
  CARD,
  TEXT,
  MUTED,
  ACCENT,
  SUN,
  CLOUD,
  RAIN,
  WARNING,
};

enum class FontRole : uint8_t {
  SMALL,
  MEDIUM,
  LARGE,
};

enum class TextAlign : uint8_t {
  LEFT,
  CENTER,
  RIGHT,
};

struct DrawCommand {
  CommandKind kind{CommandKind::TEXT};
  int16_t x1{0};
  int16_t y1{0};
  int16_t x2{0};
  int16_t y2{0};
  int16_t radius{0};
  ColorRole color{ColorRole::TEXT};
  FontRole font{FontRole::SMALL};
  TextAlign align{TextAlign::LEFT};
  uint16_t text_offset{0};
};

struct StationView {
  FixedText<24> name;
  bool heard{false};
  int16_t temperature_tenths_c{0};
  uint8_t humidity_percent{0};
  uint32_t age_seconds{0};
};

struct ScreenSnapshot {
  bool time_valid{false};
  uint8_t hour{0};
  uint8_t minute{0};
  uint16_t year{0};
  uint8_t month{0};
  uint8_t day{0};
  uint8_t day_of_week{1};
  FixedText<32> condition;
  bool is_night{false};
  StationView primary;
  static constexpr size_t MAX_SECONDARIES = 8;
  std::array<StationView, MAX_SECONDARIES> secondaries{};
  size_t secondary_count{0};
  bool secondary_overflow{false};
  FixedText<24> sunrise;
  FixedText<24> sunset;
  bool sun_progress_valid{false};
  uint8_t sun_progress_percent{0};
  bool wifi_valid{false};
  int16_t wifi_dbm{0};
  FixedText<40> ip_address;
  uint32_t now_ms{0};

  bool add_secondary(const StationView &station) {
    if (this->secondary_count == this->secondaries.size()) {
      this->secondary_overflow = true;
      return false;
    }
    this->secondaries[this->secondary_count++] = station;
    return true;
  }
};

struct LayoutOptions {
  bool use_24_hour{false};
  bool show_time{true};
  bool show_date{true};
  bool show_condition{true};
  bool show_primary{true};
  bool show_secondary{true};
  bool show_sun{true};
  bool show_network{true};
};

class Scene {
 public:
  static constexpr size_t CAPACITY = 52;
  static constexpr size_t TEXT_CAPACITY = 384;

  void clear() {
    this->size_ = 0;
    this->text_size_ = 0;
    this->overflow_ = false;
  }
  bool push(const DrawCommand &command) {
    if (this->size_ == this->commands_.size()) {
      this->overflow_ = true;
      return false;
    }
    this->commands_[this->size_++] = command;
    return true;
  }
  bool push_text(DrawCommand command, const char *text) {
    const size_t length = std::strlen(text) + 1U;
    if (this->size_ == this->commands_.size() ||
        this->text_size_ + length > this->text_.size()) {
      this->overflow_ = true;
      return false;
    }
    command.text_offset = static_cast<uint16_t>(this->text_size_);
    std::memcpy(this->text_.data() + this->text_size_, text, length);
    this->text_size_ += length;
    this->commands_[this->size_++] = command;
    return true;
  }
  const char *text(const DrawCommand &command) const {
    return this->text_.data() + command.text_offset;
  }
  size_t size() const { return this->size_; }
  constexpr size_t capacity() const { return this->commands_.size(); }
  size_t text_size() const { return this->text_size_; }
  constexpr size_t text_capacity() const { return this->text_.size(); }
  bool overflowed() const { return this->overflow_; }
  const DrawCommand *begin() const { return this->commands_.data(); }
  const DrawCommand *end() const { return this->commands_.data() + this->size_; }

 private:
  std::array<DrawCommand, CAPACITY> commands_{};
  std::array<char, TEXT_CAPACITY> text_{};
  size_t size_{0};
  size_t text_size_{0};
  bool overflow_{false};
};

void format_time(
    uint8_t hour, uint8_t minute, bool use_24_hour, char *output, size_t output_size);
void format_date(
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t day_of_week,
    char *output,
    size_t output_size);
void format_age(uint32_t age_seconds, bool heard, char *output, size_t output_size);
void format_station_values(
    const StationView &station, char *output, size_t output_size);
void humanize_condition(const char *condition, char *output, size_t output_size);
size_t selected_secondary_index(size_t count, uint32_t now_ms);
uint8_t wifi_signal_level(bool available, int16_t rssi_dbm);
bool update_wifi_startup_gate(bool wifi_connected, bool &wifi_ever_connected);
using DrawCommandCallback =
    bool (*)(void *context, const DrawCommand &command, const char *text);
void emit_scene(
    const ScreenSnapshot &snapshot,
    const LayoutOptions &options,
    DrawCommandCallback callback,
    void *context);
void emit_startup_scene(DrawCommandCallback callback, void *context);
void build_scene(
    const ScreenSnapshot &snapshot, const LayoutOptions &options, Scene &scene);
void build_startup_scene(Scene &scene);
const char *color_hex(ColorRole color);

}  // namespace weather_station_display
