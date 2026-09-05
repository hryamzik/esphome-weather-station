# SPDX-License-Identifier: GPL-3.0-or-later

import esphome.codegen as cg
from esphome.components import font, sensor, text_sensor, time, weather_station
import esphome.config_validation as cv
from esphome.const import CONF_ID


DEPENDENCIES = ["display", "weather_station", "wifi"]

CONF_CONDITION_ID = "condition_id"
CONF_FONTS = "fonts"
CONF_HOUR_FORMAT = "hour_format"
CONF_IP_ADDRESS_ID = "ip_address_id"
CONF_LARGE = "large"
CONF_MEDIUM = "medium"
CONF_SECTIONS = "sections"
CONF_SHOW_AM_PM = "show_am_pm"
CONF_SHOW_CONDITION = "condition"
CONF_SHOW_DATE = "date"
CONF_SHOW_NETWORK = "network"
CONF_SHOW_PRIMARY = "primary"
CONF_SHOW_SECONDARY = "secondary"
CONF_SHOW_SUN = "sun"
CONF_SHOW_TIME = "time"
CONF_SMALL = "small"
CONF_STALE_AFTER = "stale_after"
CONF_SUN_PROGRESS_ID = "sun_progress_id"
CONF_SUN_STATE_ID = "sun_state_id"
CONF_SUNRISE_ID = "sunrise_id"
CONF_SUNSET_ID = "sunset_id"
CONF_TIME_ID = "time_id"
CONF_WEATHER_STATION_ID = "weather_station_id"
CONF_WEATHER_TEMPERATURE_ID = "weather_temperature_id"
CONF_WIFI_SIGNAL_ID = "wifi_signal_id"

weather_station_screen_ns = cg.esphome_ns.namespace("weather_station_screen")
WeatherStationScreen = weather_station_screen_ns.class_("WeatherStationScreen")


def _positive_time_period_seconds(value):
    period = cv.positive_time_period_milliseconds(value)
    seconds = (period.total_milliseconds + 999) // 1000
    if seconds == 0:
        raise cv.Invalid("time period must be greater than zero")
    if seconds > 0xFFFFFFFF:
        raise cv.Invalid("time period is too large to store as seconds")
    return seconds


FONT_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_SMALL): cv.use_id(font.Font),
        cv.Required(CONF_MEDIUM): cv.use_id(font.Font),
        cv.Required(CONF_LARGE): cv.use_id(font.Font),
    }
)

SECTIONS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_SHOW_TIME, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_DATE, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_CONDITION, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_PRIMARY, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_SECONDARY, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_SUN, default=True): cv.boolean,
        cv.Optional(CONF_SHOW_NETWORK, default=True): cv.boolean,
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WeatherStationScreen),
        cv.Required(CONF_WEATHER_STATION_ID): cv.use_id(
            weather_station.WeatherStationComponent
        ),
        cv.Required(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
        cv.Required(CONF_FONTS): FONT_SCHEMA,
        cv.Optional(CONF_HOUR_FORMAT, default="12h"): cv.one_of(
            "12h", "24h", lower=True
        ),
        cv.Optional(CONF_SHOW_AM_PM, default=False): cv.boolean,
        cv.Optional(
            CONF_STALE_AFTER, default="5min"
        ): _positive_time_period_seconds,
        cv.Optional(CONF_CONDITION_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_WEATHER_TEMPERATURE_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_SUN_STATE_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_SUNRISE_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_SUNSET_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_SUN_PROGRESS_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_WIFI_SIGNAL_ID): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_IP_ADDRESS_ID): cv.use_id(text_sensor.TextSensor),
        cv.Optional(CONF_SECTIONS, default={}): SECTIONS_SCHEMA,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    station = await cg.get_variable(config[CONF_WEATHER_STATION_ID])
    clock = await cg.get_variable(config[CONF_TIME_ID])
    fonts = config[CONF_FONTS]
    small = await cg.get_variable(fonts[CONF_SMALL])
    medium = await cg.get_variable(fonts[CONF_MEDIUM])
    large = await cg.get_variable(fonts[CONF_LARGE])

    cg.add(var.set_weather_station(station))
    cg.add(var.set_time(clock))
    cg.add(var.set_fonts(small, medium, large))
    cg.add(var.set_use_24_hour(config[CONF_HOUR_FORMAT] == "24h"))
    cg.add(var.set_show_am_pm(config[CONF_SHOW_AM_PM]))
    cg.add(var.set_stale_after_seconds(config[CONF_STALE_AFTER]))

    optional_setters = (
        (CONF_CONDITION_ID, var.set_condition),
        (CONF_WEATHER_TEMPERATURE_ID, var.set_weather_temperature),
        (CONF_SUN_STATE_ID, var.set_sun_state),
        (CONF_SUNRISE_ID, var.set_sunrise),
        (CONF_SUNSET_ID, var.set_sunset),
        (CONF_SUN_PROGRESS_ID, var.set_sun_progress),
        (CONF_WIFI_SIGNAL_ID, var.set_wifi_signal),
        (CONF_IP_ADDRESS_ID, var.set_ip_address),
    )
    for key, setter in optional_setters:
        if key in config:
            value = await cg.get_variable(config[key])
            cg.add(setter(value))

    sections = config[CONF_SECTIONS]
    cg.add(var.set_show_time(sections[CONF_SHOW_TIME]))
    cg.add(var.set_show_date(sections[CONF_SHOW_DATE]))
    cg.add(var.set_show_condition(sections[CONF_SHOW_CONDITION]))
    cg.add(var.set_show_primary(sections[CONF_SHOW_PRIMARY]))
    cg.add(var.set_show_secondary(sections[CONF_SHOW_SECONDARY]))
    cg.add(var.set_show_sun(sections[CONF_SHOW_SUN]))
    cg.add(var.set_show_network(sections[CONF_SHOW_NETWORK]))
