#include <check.h>
#include <stdint.h>
#include <string>

#include "commands/commands.h"
#include "payload/json.h"

namespace json = openxc::payload::json;

using openxc::commands::validate;

void setup() {
}

START_TEST (test_passthrough_response)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_PASSTHROUGH;
    message.command_response.has_status = true;
    message.command_response.status = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_passthrough_request)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    message.has_control_command = true;
    message.control_command.has_type = true;
    message.control_command.type = openxc_ControlCommand_Type_PASSTHROUGH;
    message.control_command.has_passthrough_mode_request = true;
    message.control_command.passthrough_mode_request.has_bus = true;
    message.control_command.passthrough_mode_request.bus = 1;
    message.control_command.passthrough_mode_request.has_enabled = true;
    message.control_command.passthrough_mode_request.enabled = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_af_bypass_response)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS;
    message.command_response.has_status = true;
    message.command_response.status = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_af_bypass_request)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    message.has_control_command = true;
    message.control_command.has_type = true;
    message.control_command.type = openxc_ControlCommand_Type_ACCEPTANCE_FILTER_BYPASS;
    message.control_command.has_acceptance_filter_bypass_command = true;
    message.control_command.acceptance_filter_bypass_command.has_bus = true;
    message.control_command.acceptance_filter_bypass_command.bus = 1;
    message.control_command.acceptance_filter_bypass_command.has_bypass = true;
    message.control_command.acceptance_filter_bypass_command.bypass = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_payload_format_response)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;
    message.command_response.has_status = true;
    message.command_response.status = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_payload_format_request)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    message.has_control_command = true;
    message.control_command.has_type = true;
    message.control_command.type = openxc_ControlCommand_Type_PAYLOAD_FORMAT;
    message.control_command.has_payload_format_command = true;
    message.control_command.payload_format_command.has_format = true;
    message.control_command.payload_format_command.format = openxc_PayloadFormatCommand_PayloadFormat_JSON;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
    message.control_command.payload_format_command.format = openxc_PayloadFormatCommand_PayloadFormat_PROTOBUF;
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_predefined_obd2_requests_response)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_COMMAND_RESPONSE;
    message.has_command_response = true;
    message.command_response.has_type = true;
    message.command_response.type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;
    message.command_response.has_status = true;
    message.command_response.status = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_predefined_obd2_requests_request)
{
    openxc_VehicleMessage message = {0};
    message.has_type = true;
    message.type = openxc_VehicleMessage_Type_CONTROL_COMMAND;
    message.has_control_command = true;
    message.control_command.has_type = true;
    message.control_command.type = openxc_ControlCommand_Type_PREDEFINED_OBD2_REQUESTS;
    message.control_command.has_predefined_obd2_requests_command = true;
    message.control_command.predefined_obd2_requests_command.has_enabled = true;
    message.control_command.predefined_obd2_requests_command.enabled = true;
    uint8_t payload[256] = {0};
    ck_assert(json::serialize(&message, payload, sizeof(payload)) > 0);
}
END_TEST

START_TEST (test_deserialize_can_message_write)
{
    uint8_t rawRequest[] = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}\0";
    openxc_VehicleMessage deserialized = {0};
    json::deserialize(rawRequest, sizeof(rawRequest), &deserialized);
    ck_assert(validate(&deserialized));
    ck_assert(!deserialized.can_message.has_frame_format);
}
END_TEST

START_TEST (test_deserialize_can_message_write_with_format)
{
    uint8_t rawRequest[] = "{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\", \"frame_format\": \"standard\"}\0";
    openxc_VehicleMessage deserialized = {0};
    json::deserialize(rawRequest, sizeof(rawRequest), &deserialized);
    ck_assert(validate(&deserialized));
    ck_assert(deserialized.can_message.has_frame_format);
    ck_assert_int_eq(openxc_CanMessage_FrameFormat_STANDARD,
            deserialized.can_message.frame_format);
}
END_TEST

START_TEST (test_deserialize_message_after_junk)
{
    uint8_t rawRequest[] = "prime\0{\"bus\": 1, \"id\": 42, \"data\": \"0x1234\"}\0";
    openxc_VehicleMessage deserialized = {0};
    ck_assert_int_eq(6, json::deserialize(rawRequest, sizeof(rawRequest), &deserialized));
    ck_assert(!validate(&deserialized));
}
END_TEST

Suite* suite(void) {
    Suite* s = suite_create("json_payload");
    TCase *tc_json_payload = tcase_create("json_payload");
    tcase_add_checked_fixture(tc_json_payload, setup, NULL);
    tcase_add_test(tc_json_payload, test_passthrough_request);
    tcase_add_test(tc_json_payload, test_passthrough_response);
    tcase_add_test(tc_json_payload, test_af_bypass_response);
    tcase_add_test(tc_json_payload, test_af_bypass_request);
    tcase_add_test(tc_json_payload, test_payload_format_response);
    tcase_add_test(tc_json_payload, test_payload_format_request);
    tcase_add_test(tc_json_payload, test_predefined_obd2_requests_response);
    tcase_add_test(tc_json_payload, test_predefined_obd2_requests_request);
    tcase_add_test(tc_json_payload, test_deserialize_can_message_write);
    tcase_add_test(tc_json_payload, test_deserialize_can_message_write_with_format);
    tcase_add_test(tc_json_payload, test_deserialize_message_after_junk);
    suite_add_tcase(s, tc_json_payload);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = suite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
