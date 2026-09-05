// SPDX-License-Identifier: GPL-3.0-or-later

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "display_layout.h"

namespace {

using namespace weather_station_display;

std::string escape_xml(const std::string &value) {
  std::string escaped;
  for (char character : value) {
    if (character == '&') {
      escaped += "&amp;";
    } else if (character == '<') {
      escaped += "&lt;";
    } else if (character == '>') {
      escaped += "&gt;";
    } else {
      escaped += character;
    }
  }
  return escaped;
}

ScreenSnapshot base_snapshot() {
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
      {"Balcony", true, 198, 52, 245},
      {"Workshop", false, 0, 0, 0}};
  snapshot.sunrise = "06:42";
  snapshot.sunset = "19:51";
  snapshot.wifi_valid = true;
  snapshot.wifi_dbm = -58;
  snapshot.ip_address = "192.168.87.44";
  return snapshot;
}

ScreenSnapshot scenario(const std::string &name, LayoutOptions &options) {
  auto snapshot = base_snapshot();
  if (name == "day") {
    snapshot.condition = "sunny";
    snapshot.hour = 10;
    snapshot.minute = 8;
    snapshot.now_ms = 0;
  } else if (name == "night") {
    snapshot.condition = "clear-night";
    snapshot.is_night = true;
    snapshot.hour = 22;
    snapshot.minute = 14;
    snapshot.now_ms = 2000;
  } else if (name == "long-age") {
    snapshot.condition = "rainy";
    snapshot.primary.age_seconds = 15U * 86400U + 6U * 3600U;
    snapshot.secondaries[0].age_seconds = 3U * 86400U + 4U * 3600U;
    snapshot.now_ms = 0;
  } else if (name == "secondary-cycle-a") {
    snapshot.now_ms = 0;
  } else if (name == "secondary-cycle-b") {
    snapshot.now_ms = 2000;
    options.use_24_hour = true;
  } else {
    throw std::runtime_error("unknown preview scenario: " + name);
  }
  return snapshot;
}

void write_svg(const Scene &scene, const std::string &path) {
  std::ofstream output(path);
  if (!output.good()) {
    throw std::runtime_error("cannot open preview output: " + path);
  }
  output << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"480\" height=\"480\" "
            "viewBox=\"0 0 240 240\">\n";
  output << "<title>ESPHome Weather Station 240x240 preview</title>\n";
  output << "<style>text{font-family:Roboto,Arial,sans-serif}</style>\n";
  for (const auto &command : scene) {
    const char *color = color_hex(command.color);
    switch (command.kind) {
      case CommandKind::TEXT: {
        const int size = command.font == FontRole::LARGE
                             ? 24
                             : (command.font == FontRole::MEDIUM ? 16 : 11);
        const char *anchor = command.align == TextAlign::CENTER
                                 ? "middle"
                                 : (command.align == TextAlign::RIGHT ? "end" : "start");
        output << "<text x=\"" << command.x1 << "\" y=\"" << command.y1
               << "\" fill=\"" << color << "\" font-size=\"" << size
               << "\" font-weight=\""
               << (command.font == FontRole::LARGE ? "700" : "500")
               << "\" text-anchor=\"" << anchor
               << "\" dominant-baseline=\"hanging\">"
               << escape_xml(command.text) << "</text>\n";
        break;
      }
      case CommandKind::LINE:
        output << "<line x1=\"" << command.x1 << "\" y1=\"" << command.y1
               << "\" x2=\"" << command.x2 << "\" y2=\"" << command.y2
               << "\" stroke=\"" << color << "\" stroke-width=\"2\"/>\n";
        break;
      case CommandKind::RECTANGLE:
      case CommandKind::FILLED_RECTANGLE:
        output << "<rect x=\"" << command.x1 << "\" y=\"" << command.y1
               << "\" width=\"" << command.x2 << "\" height=\"" << command.y2
               << "\" rx=\"3\" "
               << (command.kind == CommandKind::FILLED_RECTANGLE ? "fill=\"" : "fill=\"none\" stroke=\"")
               << color << "\"/>\n";
        break;
      case CommandKind::CIRCLE:
      case CommandKind::FILLED_CIRCLE:
        output << "<circle cx=\"" << command.x1 << "\" cy=\"" << command.y1
               << "\" r=\"" << command.radius << "\" "
               << (command.kind == CommandKind::FILLED_CIRCLE ? "fill=\"" : "fill=\"none\" stroke=\"")
               << color << "\"/>\n";
        break;
    }
  }
  output << "</svg>\n";
}

}  // namespace

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "usage: preview_renderer SCENARIO OUTPUT.svg\n";
    return 2;
  }
  try {
    LayoutOptions options;
    const auto snapshot = scenario(argv[1], options);
    write_svg(build_scene(snapshot, options), argv[2]);
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
