// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "./src/telemetryDefinitions.h"
#include "./src/telemetryGlobalVariables.h"
#include "./src/weidosTasks.h"
#include "./src/propertiesDefinitions.h"
#include "./src/propertiesGlobalVariables.h"
#include <RTClib.h>

#include <stdarg.h>
#include <stdlib.h>

#include <az_core.h>
#include <az_iot.h>

#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"

#include <az_precondition_internal.h>

/* --- Defines --- */
#define AZURE_PNP_MODEL_ID "dtmi:conexiones:EnergyMeter_6bm;1"

#define SAMPLE_DEVICE_INFORMATION_NAME "deviceInformation"

// The next couple properties are in KiloBytes.
#define SAMPLE_TOTAL_STORAGE_PROPERTY_VALUE 4096
#define SAMPLE_TOTAL_MEMORY_PROPERTY_VALUE 8192


static az_span COMMAND_NAME_TOGGLE_LED_1 = AZ_SPAN_FROM_STR("ToggleLed1");
static az_span COMMAND_NAME_TOGGLE_LED_2 = AZ_SPAN_FROM_STR("ToggleLed2");
static az_span COMMAND_NAME_DISPLAY_TEXT = AZ_SPAN_FROM_STR("DisplayText");
#define COMMAND_RESPONSE_CODE_ACCEPTED 202
#define COMMAND_RESPONSE_CODE_REJECTED 404

#define WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS "telemetryFrequencySecs"
#define WRITABLE_PROPERTY_RESPONSE_SUCCESS "success"

#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define TRIPLE_DECIMAL_PLACE_DIGITS 3

/* --- Function Checks and Returns --- */
#define RESULT_OK 0
#define RESULT_ERROR __LINE__

#define EXIT_IF_TRUE(condition, retcode, message, ...) \
  do                                                   \
  {                                                    \
    if (condition)                                     \
    {                                                  \
      LogError(message, ##__VA_ARGS__);                \
      return retcode;                                  \
    }                                                  \
  } while (0)

#define EXIT_IF_AZ_FAILED(azresult, retcode, message, ...) \
  EXIT_IF_TRUE(az_result_failed(azresult), retcode, message, ##__VA_ARGS__)

/* --- Data --- */
#define DATA_BUFFER_SIZE 4096

static uint8_t data_buffer[DATA_BUFFER_SIZE];
static uint32_t telemetry_send_count = 0;

static size_t telemetry_frequency_in_seconds = 60; // With default frequency of once in 10 seconds.
static time_t last_telemetry_send_time = INDEFINITE_TIME;

static bool led1_on = false;
static bool led2_on = false;

/* --- Function Prototypes --- */
/* Please find the function implementations at the bottom of this file */
static int generate_telemetry_payload(
    uint8_t* payload_buffer,
    size_t payload_buffer_size,
    size_t* payload_buffer_length);
static int generate_device_info_payload(
    az_iot_hub_client const* hub_client,
    uint8_t* payload_buffer,
    size_t payload_buffer_size,
    size_t* payload_buffer_length);
static int consume_properties_and_generate_response(
    azure_iot_t* azure_iot,
    az_span properties,
    uint8_t* buffer,
    size_t buffer_size,
    size_t* response_length);

/* --- Public Functions --- */
void azure_pnp_init() {}

const az_span azure_pnp_get_model_id() { return AZ_SPAN_FROM_STR(AZURE_PNP_MODEL_ID); }

void azure_pnp_set_telemetry_frequency(size_t frequency_in_seconds)
{
  telemetry_frequency_in_seconds = frequency_in_seconds;
  LogInfo("Telemetry frequency set to once every %d seconds.", telemetry_frequency_in_seconds);
}

/* Application-specific data section */

int azure_pnp_send_telemetry(azure_iot_t* azure_iot)
{
  _az_PRECONDITION_NOT_NULL(azure_iot);

  time_t now = time(NULL);

  if (now == INDEFINITE_TIME)
  {
    LogError("Failed getting current time for controlling telemetry.");
    return RESULT_ERROR;
  }
  else if (
      last_telemetry_send_time == INDEFINITE_TIME
      || difftime(now, last_telemetry_send_time) >= telemetry_frequency_in_seconds)
  {
    size_t payload_size;

    last_telemetry_send_time = now;

    if (generate_telemetry_payload(data_buffer, DATA_BUFFER_SIZE, &payload_size) != RESULT_OK)
    {
      LogError("Failed generating telemetry payload.");
      return RESULT_ERROR;
    }

    if (azure_iot_send_telemetry(azure_iot, az_span_create(data_buffer, payload_size)) != 0)
    {
      LogError("Failed sending telemetry.");
      return RESULT_ERROR;
    }
  }

  return RESULT_OK;
}

int azure_pnp_send_device_info(azure_iot_t* azure_iot, uint32_t request_id)
{
  _az_PRECONDITION_NOT_NULL(azure_iot);

  int result;
  size_t length;

  result = generate_device_info_payload(
      &azure_iot->iot_hub_client, data_buffer, DATA_BUFFER_SIZE, &length);
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed generating telemetry payload.");

  result = azure_iot_send_properties_update(
      azure_iot, request_id, az_span_create(data_buffer, length));
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed sending reported properties update.");

  return RESULT_OK;
}

int azure_pnp_handle_command_request(azure_iot_t* azure_iot, command_request_t command)
{
  _az_PRECONDITION_NOT_NULL(azure_iot);

  uint16_t response_code;

  if (az_span_is_content_equal(command.command_name, COMMAND_NAME_TOGGLE_LED_1))
  {
    led1_on = !led1_on;
    LogInfo("LED 1 state: %s", (led1_on ? "ON" : "OFF"));
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else if (az_span_is_content_equal(command.command_name, COMMAND_NAME_TOGGLE_LED_2))
  {
    led2_on = !led2_on;
    LogInfo("LED 2 state: %s", (led2_on ? "ON" : "OFF"));
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else if (az_span_is_content_equal(command.command_name, COMMAND_NAME_DISPLAY_TEXT))
  {
    // The payload comes surrounded by quotes, so to remove them we offset the payload by 1 and its
    // size by 2.
    LogInfo(
        "OLED display: %.*s", az_span_size(command.payload) - 2, az_span_ptr(command.payload) + 1);
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else
  {
    LogError(
        "Command not recognized (%.*s).",
        az_span_size(command.command_name),
        az_span_ptr(command.command_name));
    response_code = COMMAND_RESPONSE_CODE_REJECTED;
  }

  return azure_iot_send_command_response(
      azure_iot, command.request_id, response_code, AZ_SPAN_EMPTY);
}

int azure_pnp_handle_properties_update(
    azure_iot_t* azure_iot,
    az_span properties,
    uint32_t request_id)
{
  _az_PRECONDITION_NOT_NULL(azure_iot);
  _az_PRECONDITION_VALID_SPAN(properties, 1, false);

  int result;
  size_t length;

  result = consume_properties_and_generate_response(
      azure_iot, properties, data_buffer, DATA_BUFFER_SIZE, &length);
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed generating properties ack payload.");

  result = azure_iot_send_properties_update(
      azure_iot, request_id, az_span_create(data_buffer, length));
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed sending reported properties update.");

  return RESULT_OK;
}

/* --- Internal Functions --- */
static float simulated_get_temperature() { return 21.0; }

static float simulated_get_humidity() { return 88.0; }

static float simulated_get_ambientLight() { return 700.0; }

static void simulated_get_pressure_altitude(float* pressure, float* altitude)
{
  *pressure = 55.0;
  *altitude = 700.0;
}

static void simulated_get_magnetometer(
    int32_t* magneticFieldX,
    int32_t* magneticFieldY,
    int32_t* magneticFieldZ)
{
  *magneticFieldX = 2000;
  *magneticFieldY = 3000;
  *magneticFieldZ = 4000;
}

static void simulated_get_pitch_roll_accel(
    int32_t* pitch,
    int32_t* roll,
    int32_t* accelerationX,
    int32_t* accelerationY,
    int32_t* accelerationZ)
{
  *pitch = 30;
  *roll = 90;
  *accelerationX = 33;
  *accelerationY = 44;
  *accelerationZ = 55;
}

static int generate_telemetry_payload(
    uint8_t* payload_buffer,
    size_t payload_buffer_size,
    size_t* payload_buffer_length)
{
  az_json_writer jw;
  az_result rc;
  az_span payload_buffer_span = az_span_create(payload_buffer, payload_buffer_size);
  az_span json_span;

  clearData();
  getData();
  computeData();
  
  rc = az_json_writer_init(&jw, payload_buffer_span, NULL);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed initializing json writer for telemetry.");

  rc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed setting telemetry json root.");


  //########################              ENERGY METER TELEMETRY           #########################
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_VOLTAGE_LN));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgVoltageLN property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgVoltageLN, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgVoltageLN property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L1L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1L2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL1L2, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1L2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L2L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL2L3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL2L3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL2L3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTAGE_L1L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1L3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, voltageL1L3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltageL1L3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_VOLTAGE_LL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgVoltageLL property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgVoltageLL, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgVoltageLL property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CURRENT_L1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL1 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, currentL1, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL1 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CURRENT_L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, currentL2, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CURRENT_L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, currentL3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentL3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CURRENT_NEUTRAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentNeutral property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, currentNeutral, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentNeutral property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_CURRENTL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgCurrentL property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgCurrentL, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgCurrentL property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CURRENT_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, currentTotal, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding currentTotal property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_POWER_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realPowerL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_POWER_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realPowerL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_POWER_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realPowerL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_POWER_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realPowerTotal, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realPowerTotal property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_POWER_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentPowerL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_POWER_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentPowerL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_POWER_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentPowerL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_POWER_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentPowerTotal, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentPowerTotal property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_POWER_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactivePowerL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_POWER_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactivePowerL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_POWER_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactivePowerL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_POWER_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactivePowerTotal, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactivePowerTotal property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_COS_PHI_L1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL1 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, cosPhiL1, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL1 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_COS_PHI_L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, cosPhiL2, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_COS_PHI_L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, cosPhiL3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding cosPhiL3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_COS_PHI));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgCosPhi property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgCosPhi, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgCosPhi property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_FREQUENCY));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding frequency property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, frequency, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding frequency property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ROT_FIELD));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding rotField property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, rotField, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding rotField property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_ENERGY_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realEnergyL1N, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_ENERGY_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realEnergyL2N, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_ENERGY_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realEnergyL3N, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REAL_ENERGY_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, realEnergyTotal, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding realEnergyTotal property value to telemetry payload. ");




  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_ENERGY_L1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL1 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentEnergyL1, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL1 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_ENERGY_L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentEnergyL2, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_ENERGY_L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentEnergyL3, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyL3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_APPARENT_ENERGY_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, apparentEnergyTotal, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding apparentEnergyTotal property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_ENERGY_L1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL1 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactiveEnergyL1, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL1 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_ENERGY_L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactiveEnergyL2, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_ENERGY_L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactiveEnergyL3, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyL3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_REACTIVE_ENERGY_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, reactiveEnergyTotal, TRIPLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding reactiveEnergyTotal property value to telemetry payload. ");




  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_THD_VOLTS_LN));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgTHDVoltsLN, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgTHDVoltsLN property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_CURRENT_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDCurrentL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_CURRENT_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDCurrentL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_CURRENT_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDCurrentL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDCurrentL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_THD_CURRENT_LN));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgTHDCurrentLN property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgTHDCurrentLN, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgTHDCurrentLN property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L1L2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1L2 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL1L2, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1L2 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L2L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL2L3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL2L3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL2L3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_THD_VOLTS_L1L3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1L3 property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, THDVoltsL1L3, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding THDVoltsL1L3 property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AVG_THD_VOLTS_LL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgTHDVoltsLL property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, avgTHDVoltsLL, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding avgTHDVoltsLL property value to telemetry payload. ");



  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_POWER_FACTOR_L1N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL1N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, powerFactorL1N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL1N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_POWER_FACTOR_L2N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL2N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, powerFactorL2N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL2N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_POWER_FACTOR_L3N));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL3N property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, powerFactorL3N, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorL3N property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_POWER_FACTOR_TOTAL));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorTotal property name to telemetry payload.");
  rc = az_json_writer_append_double(&jw, powerFactorTotal, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding powerFactorTotal property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_COM_STATUS));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding comStatus property name to telemetry payload.");
  rc = az_json_writer_append_int32(&jw, comStatus);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding comStatus property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TIMESTAMP));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding timestamp property name to telemetry payload.");
  //rc = az_json_writer_append_int32(&jw, unixTimestamp);
  DateTime date = DateTime(timestamp);
  char dateFormat[] = "YYYY-MM-DDThh:mm:ss.000Z";
  date.toString(dateFormat);
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(dateFormat));

  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding timestamp property value to telemetry payload. ");




  rc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing telemetry json payload.");

  payload_buffer_span = az_json_writer_get_bytes_used_in_destination(&jw);

  if ((payload_buffer_size - az_span_size(payload_buffer_span)) < 1)
  {
    LogError("Insufficient space for telemetry payload null terminator.");
    return RESULT_ERROR;
  }

  payload_buffer[az_span_size(payload_buffer_span)] = null_terminator;
  *payload_buffer_length = az_span_size(payload_buffer_span);

  return RESULT_OK;
}

static int generate_device_info_payload(
    az_iot_hub_client const* hub_client,
    uint8_t* payload_buffer,
    size_t payload_buffer_size,
    size_t* payload_buffer_length)
{
  az_json_writer jw;
  az_result rc;
  az_span payload_buffer_span = az_span_create(payload_buffer, payload_buffer_size);
  az_span json_span;

  rc = az_json_writer_init(&jw, payload_buffer_span, NULL);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed initializing json writer for telemetry.");

  rc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed setting telemetry json root.");

  rc = az_iot_hub_client_properties_writer_begin_component(
      hub_client, &jw, AZ_SPAN_FROM_STR(SAMPLE_DEVICE_INFORMATION_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed writting component name.");



  
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_ASSET_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_ASSET_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(asset));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_ASSET_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_ASSET_COMMENTS_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_ASSET_COMMENTS_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(assetComments));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_ASSET_COMMENTS_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_BRAND_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_BRAND_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(brand));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_BRAND_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_IDENTIFIER_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION1_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(identifier));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION1_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_LOCATION1_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION1_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(location1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION1_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_LOCATION2_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION2_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(location2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION2_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_LOCATION3_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION3_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(location3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION3_PROPERTY_NAME to payload. ");
  
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_LOCATION4_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION4_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(location4));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_LOCATION4_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_NUM_PHASES_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_NUMPHASES_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(numPhases));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_NUMPHASES_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_MODEL_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_MODEL_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(model));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_MODEL_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_PART_NUMBER_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_PART_NUMBER_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(partNumber));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_PART_NUMBER_PROPERTY_NAME to payload. ");
  
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_SERIAL_NUMBER_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_SERIAL_NUMBER_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(serialNumber));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_SERIAL_NUMBER_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_COM_TYPE_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_COM_TYPE_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(comType));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_COM_TYPE_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_IP_ADDRESS_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_IP_ADDRESS_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(ipAddress));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_IP_ADDRESS_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_TCP_PORT_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_TCP_PORT_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(tcpPort));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_TCP_PORT_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_MODBUS_ADDRESS_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_MODBUS_ADDRESS_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(modbusAddress));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_MODBUS_ADDRESS_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_RTU_BAUDRATE_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_BAUDRATE_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(rtuBaudrate));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_BAUDRATE_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_RTU_PARITY_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_PARITY_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(rtuParity));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_PARITY_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_RTU_STOP_BITS_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_STOP_BITS_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(rtuStopBits));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_RTU_STOP_BITS_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_CUSTOM_FIELD1_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD1_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(customField1));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD1_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_CUSTOM_FIELD2_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD2_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(customField2));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD2_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_CUSTOM_FIELD3_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD3_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(customField3));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD3_PROPERTY_NAME to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(EM_CUSTOM_FIELD4_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD4_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, az_span_create_from_str(customField4));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding EM_CUSTOM_FIELD4_PROPERTY_NAME to payload. ");



  rc = az_iot_hub_client_properties_writer_end_component(hub_client, &jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing component object.");

  rc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing telemetry json payload.");

  payload_buffer_span = az_json_writer_get_bytes_used_in_destination(&jw);

  if ((payload_buffer_size - az_span_size(payload_buffer_span)) < 1)
  {
    LogError("Insufficient space for telemetry payload null terminator.");
    return RESULT_ERROR;
  }

  payload_buffer[az_span_size(payload_buffer_span)] = null_terminator;
  *payload_buffer_length = az_span_size(payload_buffer_span);

  return RESULT_OK;
}

static int generate_properties_update_response(
    azure_iot_t* azure_iot,
    az_span component_name,
    int32_t frequency,
    int32_t version,
    uint8_t* buffer,
    size_t buffer_size,
    size_t* response_length)
{
  az_result azrc;
  az_json_writer jw;
  az_span response = az_span_create(buffer, buffer_size);

  azrc = az_json_writer_init(&jw, response, NULL);
  EXIT_IF_AZ_FAILED(
      azrc, RESULT_ERROR, "Failed initializing json writer for properties update response.");

  azrc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed opening json in properties update response.");

  // This Azure PnP Template does not have a named component,
  // so az_iot_hub_client_properties_writer_begin_component is not needed.

  azrc = az_iot_hub_client_properties_writer_begin_response_status(
      &azure_iot->iot_hub_client,
      &jw,
      AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS),
      (int32_t)AZ_IOT_STATUS_OK,
      version,
      AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_RESPONSE_SUCCESS));
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed appending status to properties update response.");

  azrc = az_json_writer_append_int32(&jw, frequency);
  EXIT_IF_AZ_FAILED(
      azrc, RESULT_ERROR, "Failed appending frequency value to properties update response.");

  azrc = az_iot_hub_client_properties_writer_end_response_status(&azure_iot->iot_hub_client, &jw);
  EXIT_IF_AZ_FAILED(
      azrc, RESULT_ERROR, "Failed closing status section in properties update response.");

  // This Azure PnP Template does not have a named component,
  // so az_iot_hub_client_properties_writer_end_component is not needed.

  azrc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed closing json in properties update response.");

  *response_length = az_span_size(az_json_writer_get_bytes_used_in_destination(&jw));

  return RESULT_OK;
}

static int consume_properties_and_generate_response(
    azure_iot_t* azure_iot,
    az_span properties,
    uint8_t* buffer,
    size_t buffer_size,
    size_t* response_length)
{
  int result;
  az_json_reader jr;
  az_span component_name;
  int32_t version = 0;

  az_result azrc = az_json_reader_init(&jr, properties, NULL);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed initializing json reader for properties update.");

  const az_iot_hub_client_properties_message_type message_type
      = AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED;

  azrc = az_iot_hub_client_properties_get_properties_version(
      &azure_iot->iot_hub_client, &jr, message_type, &version);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed writable properties version.");

  azrc = az_json_reader_init(&jr, properties, NULL);
  EXIT_IF_AZ_FAILED(
      azrc, RESULT_ERROR, "Failed re-initializing json reader for properties update.");

  while (az_result_succeeded(
      azrc = az_iot_hub_client_properties_get_next_component_property(
          &azure_iot->iot_hub_client,
          &jr,
          message_type,
          AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE,
          &component_name)))
  {
    if (az_json_token_is_text_equal(
            &jr.token, AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS)))
    {
      int32_t value;
      azrc = az_json_reader_next_token(&jr);
      EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed getting writable properties next token.");

      azrc = az_json_token_get_int32(&jr.token, &value);
      EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed getting writable properties int32_t value.");

      azure_pnp_set_telemetry_frequency((size_t)value);

      result = generate_properties_update_response(
          azure_iot, component_name, value, version, buffer, buffer_size, response_length);
      EXIT_IF_TRUE(
          result != RESULT_OK, RESULT_ERROR, "generate_properties_update_response failed.");
    }
    else
    {
      LogError(
          "Unexpected property received (%.*s).",
          az_span_size(jr.token.slice),
          az_span_ptr(jr.token.slice));
    }

    azrc = az_json_reader_next_token(&jr);
    EXIT_IF_AZ_FAILED(
        azrc, RESULT_ERROR, "Failed moving to next json token of writable properties.");

    azrc = az_json_reader_skip_children(&jr);
    EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed skipping children of writable properties.");

    azrc = az_json_reader_next_token(&jr);
    EXIT_IF_AZ_FAILED(
        azrc, RESULT_ERROR, "Failed moving to next json token of writable properties (again).");
  }

  return RESULT_OK;
}
