# SPDX-License-Identifier: GPL-3.0-or-later

import esphome.codegen as cg
from esphome.components import binary_sensor, remote_base, sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CHANNEL,
    CONF_HUMIDITY,
    CONF_ID,
    CONF_TEMPERATURE,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

AUTO_LOAD = ["binary_sensor", "sensor"]
DEPENDENCIES = ["remote_receiver"]

CONF_BATTERY_LOW = "battery_low"
CONF_ROLLING_CODE = "rolling_code"

weather_station_ns = cg.esphome_ns.namespace("weather_station")
WeatherStationComponent = weather_station_ns.class_(
    "WeatherStationComponent",
    cg.Component,
    remote_base.RemoteReceiverListener,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WeatherStationComponent),
            cv.Required(remote_base.CONF_RECEIVER_ID): cv.use_id(
                remote_base.RemoteReceiverBase
            ),
            cv.Required(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Required(CONF_HUMIDITY): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_HUMIDITY,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Required(CONF_CHANNEL): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Required(CONF_ROLLING_CODE): sensor.sensor_schema(
                accuracy_decimals=0,
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
            cv.Required(CONF_BATTERY_LOW): binary_sensor.binary_sensor_schema(
                icon="mdi:battery-alert",
                entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_LISTENER_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)

    temperature = await sensor.new_sensor(config[CONF_TEMPERATURE])
    cg.add(var.set_temperature_sensor(temperature))
    humidity = await sensor.new_sensor(config[CONF_HUMIDITY])
    cg.add(var.set_humidity_sensor(humidity))
    channel = await sensor.new_sensor(config[CONF_CHANNEL])
    cg.add(var.set_channel_sensor(channel))
    rolling_code = await sensor.new_sensor(config[CONF_ROLLING_CODE])
    cg.add(var.set_rolling_code_sensor(rolling_code))
    battery_low = await binary_sensor.new_binary_sensor(config[CONF_BATTERY_LOW])
    cg.add(var.set_battery_low_sensor(battery_low))
