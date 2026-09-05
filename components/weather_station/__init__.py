# SPDX-License-Identifier: GPL-3.0-or-later

import esphome.codegen as cg
from esphome.components import binary_sensor, remote_base, sensor, text_sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_CHANNEL,
    CONF_HUMIDITY,
    CONF_ID,
    CONF_NAME,
    CONF_TEMPERATURE,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_HUMIDITY,
    DEVICE_CLASS_TEMPERATURE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_SECOND,
)

AUTO_LOAD = ["binary_sensor", "sensor", "text_sensor"]
DEPENDENCIES = ["remote_receiver"]

CONF_AGE = "age"
CONF_BATTERY_LOW = "battery_low"
CONF_DIAGNOSTICS = "diagnostics"
CONF_IGNORE = "ignore"
CONF_LAST_UNKNOWN_SELECTOR = "last_unknown_selector"
CONF_MODEL = "model"
CONF_PRIMARY = "primary"
CONF_PROTOCOL = "protocol"
CONF_PROTOCOLS = "protocols"
CONF_RECENT_UNKNOWN_COUNT = "recent_unknown_count"
CONF_ROLLING_CODE = "rolling_code"
CONF_SELECTOR = "selector"
CONF_STATIONS = "stations"
CONF_UNKNOWN_WINDOW = "unknown_window"

SUPPORTED_PROTOCOLS = {"oregon2": 0}
MODEL_CAPABILITIES = {
    ("oregon2", 0x1D20): {
        CONF_TEMPERATURE,
        CONF_HUMIDITY,
        CONF_CHANNEL,
        CONF_ROLLING_CODE,
        CONF_BATTERY_LOW,
    }
}

weather_station_ns = cg.esphome_ns.namespace("weather_station")
WeatherStationComponent = weather_station_ns.class_(
    "WeatherStationComponent",
    cg.Component,
    remote_base.RemoteReceiverListener,
)

SELECTOR_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_PROTOCOL): cv.one_of(*SUPPORTED_PROTOCOLS, lower=True),
        cv.Required(CONF_MODEL): cv.hex_uint16_t,
        cv.Required(CONF_CHANNEL): cv.int_range(min=1, max=255),
        cv.Optional(CONF_ROLLING_CODE): cv.hex_uint8_t,
    }
)

STATION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.validate_id_name,
        cv.Required(CONF_NAME): cv.string_strict,
        cv.Optional(CONF_PRIMARY, default=False): cv.boolean,
        cv.Required(CONF_SELECTOR): SELECTOR_SCHEMA,
        cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_HUMIDITY): sensor.sensor_schema(
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_HUMIDITY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_CHANNEL): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_ROLLING_CODE): sensor.sensor_schema(
            accuracy_decimals=0,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_BATTERY_LOW): binary_sensor.binary_sensor_schema(
            icon="mdi:battery-alert",
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional(CONF_AGE): sensor.sensor_schema(
            unit_of_measurement=UNIT_SECOND,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
    }
)

DIAGNOSTICS_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_LAST_UNKNOWN_SELECTOR): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:text-box-search-outline",
        ),
        cv.Optional(CONF_RECENT_UNKNOWN_COUNT): sensor.sensor_schema(
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            icon="mdi:radar",
        ),
        cv.Optional(
            CONF_UNKNOWN_WINDOW, default="5min"
        ): cv.positive_time_period_milliseconds,
    }
)


def _selectors_overlap(left, right):
    if (
        left[CONF_PROTOCOL] != right[CONF_PROTOCOL]
        or left[CONF_MODEL] != right[CONF_MODEL]
        or left[CONF_CHANNEL] != right[CONF_CHANNEL]
    ):
        return False
    return (
        CONF_ROLLING_CODE not in left
        or CONF_ROLLING_CODE not in right
        or left[CONF_ROLLING_CODE] == right[CONF_ROLLING_CODE]
    )


def _validate_selector_supported(selector, context):
    key = (selector[CONF_PROTOCOL], selector[CONF_MODEL])
    if key not in MODEL_CAPABILITIES:
        raise cv.Invalid(
            f"{context} selects unsupported {key[0]} model 0x{key[1]:04X}"
        )
    if key == ("oregon2", 0x1D20) and selector[CONF_CHANNEL] not in (1, 2, 3):
        raise cv.Invalid(f"{context} selects invalid Oregon2 THGR122N channel")
    return MODEL_CAPABILITIES[key]


def _validate_station_config(config):
    enabled = set(config[CONF_PROTOCOLS])
    if len(enabled) != len(config[CONF_PROTOCOLS]):
        raise cv.Invalid("protocols must not contain duplicates")
    ids = set()
    primary_count = 0
    stations = config[CONF_STATIONS]
    for index, station in enumerate(stations):
        station_id = station[CONF_ID]
        if station_id in ids:
            raise cv.Invalid(f"duplicate station id '{station_id}'")
        ids.add(station_id)
        primary_count += int(station[CONF_PRIMARY])
        selector = station[CONF_SELECTOR]
        if selector[CONF_PROTOCOL] not in enabled:
            raise cv.Invalid(
                f"station '{station_id}' uses protocol "
                f"'{selector[CONF_PROTOCOL]}' which is not enabled in protocols"
            )
        capabilities = _validate_selector_supported(
            selector, f"station '{station_id}'"
        )
        requested_entities = {
            CONF_TEMPERATURE,
            CONF_HUMIDITY,
            CONF_CHANNEL,
            CONF_ROLLING_CODE,
            CONF_BATTERY_LOW,
        }.intersection(station)
        unsupported_entities = requested_entities - capabilities
        if unsupported_entities:
            raise cv.Invalid(
                f"station '{station_id}' configures unsupported entities: "
                f"{', '.join(sorted(unsupported_entities))}"
            )
        for previous in stations[:index]:
            if _selectors_overlap(previous[CONF_SELECTOR], selector):
                raise cv.Invalid(
                    f"station selectors for '{previous[CONF_ID]}' and "
                    f"'{station_id}' overlap; add distinct rolling_code values "
                    "or remove one selector"
                )
    if primary_count > 1:
        raise cv.Invalid("only one station may set primary: true")
    for selector in config[CONF_IGNORE]:
        if selector[CONF_PROTOCOL] not in enabled:
            raise cv.Invalid(
                f"ignore selector uses protocol '{selector[CONF_PROTOCOL]}' "
                "which is not enabled in protocols"
            )
        _validate_selector_supported(selector, "ignore selector")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WeatherStationComponent),
            cv.Required(remote_base.CONF_RECEIVER_ID): cv.use_id(
                remote_base.RemoteReceiverBase
            ),
            cv.Required(CONF_PROTOCOLS): cv.All(
                cv.ensure_list(cv.one_of(*SUPPORTED_PROTOCOLS, lower=True)),
                cv.Length(min=1),
            ),
            cv.Required(CONF_STATIONS): cv.All(
                cv.ensure_list(STATION_SCHEMA), cv.Length(min=1)
            ),
            cv.Optional(CONF_IGNORE, default=[]): cv.ensure_list(SELECTOR_SCHEMA),
            cv.Optional(CONF_DIAGNOSTICS): DIAGNOSTICS_SCHEMA,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(remote_base.REMOTE_LISTENER_SCHEMA),
    _validate_station_config,
)


async def _optional_sensor(config, key, factory):
    if key not in config:
        return cg.nullptr
    return await factory(config[key])


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await remote_base.register_listener(var, config)

    for protocol in config[CONF_PROTOCOLS]:
        cg.add(var.enable_protocol(SUPPORTED_PROTOCOLS[protocol]))

    for station in config[CONF_STATIONS]:
        selector = station[CONF_SELECTOR]
        temperature = await _optional_sensor(station, CONF_TEMPERATURE, sensor.new_sensor)
        humidity = await _optional_sensor(station, CONF_HUMIDITY, sensor.new_sensor)
        channel = await _optional_sensor(station, CONF_CHANNEL, sensor.new_sensor)
        rolling = await _optional_sensor(station, CONF_ROLLING_CODE, sensor.new_sensor)
        battery = await _optional_sensor(
            station, CONF_BATTERY_LOW, binary_sensor.new_binary_sensor
        )
        age = await _optional_sensor(station, CONF_AGE, sensor.new_sensor)
        has_rolling = CONF_ROLLING_CODE in selector
        cg.add(
            var.add_station(
                station[CONF_ID],
                station[CONF_NAME],
                station[CONF_PRIMARY],
                SUPPORTED_PROTOCOLS[selector[CONF_PROTOCOL]],
                selector[CONF_MODEL],
                selector[CONF_CHANNEL],
                has_rolling,
                selector.get(CONF_ROLLING_CODE, 0),
                temperature,
                humidity,
                channel,
                rolling,
                battery,
                age,
            )
        )

    for selector in config[CONF_IGNORE]:
        has_rolling = CONF_ROLLING_CODE in selector
        cg.add(
            var.add_ignore(
                SUPPORTED_PROTOCOLS[selector[CONF_PROTOCOL]],
                selector[CONF_MODEL],
                selector[CONF_CHANNEL],
                has_rolling,
                selector.get(CONF_ROLLING_CODE, 0),
            )
        )

    if CONF_DIAGNOSTICS in config:
        diagnostics = config[CONF_DIAGNOSTICS]
        cg.add(
            var.set_unknown_window_ms(
                diagnostics[CONF_UNKNOWN_WINDOW].total_milliseconds
            )
        )
        if CONF_LAST_UNKNOWN_SELECTOR in diagnostics:
            unknown_selector = await text_sensor.new_text_sensor(
                diagnostics[CONF_LAST_UNKNOWN_SELECTOR]
            )
            cg.add(var.set_last_unknown_selector_sensor(unknown_selector))
        if CONF_RECENT_UNKNOWN_COUNT in diagnostics:
            unknown_count = await sensor.new_sensor(
                diagnostics[CONF_RECENT_UNKNOWN_COUNT]
            )
            cg.add(var.set_recent_unknown_count_sensor(unknown_count))
