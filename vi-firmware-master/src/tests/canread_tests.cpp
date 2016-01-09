#include <check.h>
#include <stdint.h>
#include <string>
#include "signals.h"
#include "can/canutil.h"
#include "can/canread.h"
#include "can/canwrite.h"
#include "pipeline.h"
#include "config.h"

namespace usb = openxc::interface::usb;
namespace can = openxc::can;

using openxc::can::read::booleanDecoder;
using openxc::can::read::ignoreDecoder;
using openxc::can::read::stateDecoder;
using openxc::can::read::noopDecoder;
using openxc::can::read::publishBooleanMessage;
using openxc::can::read::publishNumericalMessage;
using openxc::can::read::publishStringMessage;
using openxc::can::read::publishVehicleMessage;
using openxc::pipeline::Pipeline;
using openxc::signals::getSignalCount;
using openxc::signals::getSignals;
using openxc::signals::getCanBuses;
using openxc::signals::getMessages;
using openxc::signals::getMessageCount;
using openxc::config::getConfiguration;

extern bool USB_PROCESSED;
extern size_t SENT_BYTES;

const CanMessage TEST_MESSAGE = {
    id: 0,
    format: STANDARD,
    data: {0xeb},
};

extern unsigned long FAKE_TIME;
extern void initializeVehicleInterface();

QUEUE_TYPE(uint8_t)* OUTPUT_QUEUE = &getConfiguration()->usb.endpoints[
        IN_ENDPOINT_INDEX].queue;

bool queueEmpty() {
    return QUEUE_EMPTY(uint8_t, OUTPUT_QUEUE);
}


void setup() {
    FAKE_TIME = 1000;
    USB_PROCESSED = false;
    SENT_BYTES = 0;
    initializeVehicleInterface();
    getConfiguration()->payloadFormat = openxc::payload::PayloadFormat::JSON;
    usb::initialize(&getConfiguration()->usb);
    getConfiguration()->usb.configured = true;
    for(int i = 0; i < getSignalCount(); i++) {
        getSignals()[i].received = false;
        getSignals()[i].sendSame = true;
        getSignals()[i].frequencyClock = {0};
        getSignals()[i].decoder = NULL;
    }
}

START_TEST (test_passthrough_decoder)
{
    bool send = true;
    openxc_DynamicField decoded = noopDecoder(&getSignals()[0], getSignals(),
            getSignalCount(), &getConfiguration()->pipeline, 42.0, &send);
    ck_assert_int_eq(decoded.numeric_value, 42.0);
    fail_unless(send);
}
END_TEST

START_TEST (test_boolean_decoder)
{
    bool send = true;
    openxc_DynamicField decoded = booleanDecoder(&getSignals()[0], getSignals(), getSignalCount(),
                &getConfiguration()->pipeline, 1.0, &send);
    ck_assert(decoded.boolean_value);
    fail_unless(send);
    decoded = booleanDecoder(&getSignals()[0], getSignals(), getSignalCount(),
                &getConfiguration()->pipeline, 0.5, &send);
    ck_assert(decoded.boolean_value);
    fail_unless(send);
    decoded = booleanDecoder(&getSignals()[0], getSignals(), getSignalCount(),
                &getConfiguration()->pipeline, 0, &send);
    ck_assert(!decoded.boolean_value);
    fail_unless(send);
}
END_TEST

START_TEST (test_ignore_decoder)
{
    bool send = true;
    ignoreDecoder(&getSignals()[0], getSignals(), getSignalCount(),
            &getConfiguration()->pipeline, 1.0, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_state_decoder)
{
    bool send = true;
    openxc_DynamicField decoded = stateDecoder(&getSignals()[1], getSignals(),
                getSignalCount(), &getConfiguration()->pipeline, 2, &send);
    ck_assert_str_eq(decoded.string_value, getSignals()[1].states[1].name);
    fail_unless(send);
    stateDecoder(&getSignals()[1], getSignals(), getSignalCount(),
            &getConfiguration()->pipeline, 42, &send);
    fail_if(send);
}
END_TEST

START_TEST (test_send_numerical)
{
    fail_unless(queueEmpty());
    publishNumericalMessage("test", 42, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot, "{\"name\":\"test\",\"value\":42}\0");
}
END_TEST

START_TEST (test_preserve_float_precision)
{
    fail_unless(queueEmpty());
    float value = 42.5;
    publishNumericalMessage("test", value, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":42.500000}\0");
}
END_TEST

START_TEST (test_send_boolean)
{
    fail_unless(queueEmpty());
    publishBooleanMessage("test", false, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":false}\0");
}
END_TEST

START_TEST (test_send_string)
{
    fail_unless(queueEmpty());
    publishStringMessage("test", "string", &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"string\"}\0");
}
END_TEST

START_TEST (test_send_evented_boolean)
{
    fail_unless(queueEmpty());

    openxc_DynamicField value = {0};
    value.has_type = true;
    value.type = openxc_DynamicField_Type_STRING;
    value.has_string_value = true;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = {0};
    event.has_type = true;
    event.type = openxc_DynamicField_Type_BOOL;
    event.has_boolean_value = true;
    event.boolean_value = false;

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":false}\0");
}
END_TEST

START_TEST (test_send_evented_string)
{
    fail_unless(queueEmpty());

    openxc_DynamicField value = {0};
    value.has_type = true;
    value.type = openxc_DynamicField_Type_STRING;
    value.has_string_value = true;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = {0};
    event.has_type = true;
    event.type = openxc_DynamicField_Type_STRING;
    event.has_string_value = true;
    strcpy(event.string_value, "event");

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":\"event\"}\0");
}
END_TEST

START_TEST (test_send_evented_float)
{
    fail_unless(queueEmpty());
    openxc_DynamicField value = {0};
    value.has_type = true;
    value.type = openxc_DynamicField_Type_STRING;
    value.has_string_value = true;
    strcpy(value.string_value, "value");

    openxc_DynamicField event = {0};
    event.has_type = true;
    event.type = openxc_DynamicField_Type_NUM;
    event.has_numeric_value = true;
    event.numeric_value = 43.0;

    publishVehicleMessage("test", &value, &event, &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"test\",\"value\":\"value\",\"event\":43}\0");
}
END_TEST

START_TEST (test_passthrough_force_send_changed)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: getMessages()[2].id,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34}
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    message.data[0] = 0x56;
    message.data[1] = 0x78;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_limited_frequency)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: getMessages()[1].id,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34}
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    FAKE_TIME += 2000;
    can::read::passthroughMessage(&getCanBuses()[0], &message, getMessages(),
            getMessageCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_passthrough_message)
{
    fail_unless(queueEmpty());
    CanMessage message = {
        id: 42,
        format: CanMessageFormat::STANDARD,
        data: {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF1},
        length: 8
    };
    can::read::passthroughMessage(&getCanBuses()[0], &message, NULL, 0,
            &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"bus\":1,\"id\":42,\"data\":\"0x123456789abcdef1\"}\0");
}
END_TEST

openxc_DynamicField floatDecoder(CanSignal* signal, CanSignal* signals, int signalCount,
        Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = {0};
    decodedValue.has_type = true;
    decodedValue.type = openxc_DynamicField_Type_NUM;
    decodedValue.has_numeric_value = true;
    decodedValue.numeric_value = 42;
    return decodedValue;
}

START_TEST (test_translate_ignore_decoder_still_received)
{
    getSignals()[0].decoder = ignoreDecoder;
    fail_if(getSignals()[0].received);
    can::read::translateSignal(&getSignals()[0], &TEST_MESSAGE, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    fail_unless(getSignals()[0].received);
}
END_TEST

START_TEST (test_default_decoder)
{
    can::read::translateSignal(&getSignals()[0], &TEST_MESSAGE, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    fail_unless(getSignals()[0].received);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":-19990}\0");
}
END_TEST

START_TEST (test_translate_respects_send_value)
{
    getSignals()[0].decoder = ignoreDecoder;
    can::read::translateSignal(&getSignals()[0], &TEST_MESSAGE, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    fail_unless(getSignals()[0].received);
}
END_TEST

START_TEST (test_translate_many_signals)
{
    getConfiguration()->pipeline.uart = NULL;
    ck_assert_int_eq(0, SENT_BYTES);
    for(int i = 7; i < 23; i++) {
        can::read::translateSignal(&getSignals()[i],
                &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
        fail_unless(getSignals()[i].received);
    }
    fail_unless(USB_PROCESSED);
    // 8 signals sent
    ck_assert_int_eq(10 * 29 + 2, SENT_BYTES);
    // 6 in the output queue
    fail_if(queueEmpty());
    ck_assert_int_eq(6 * 29, QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE));
}
END_TEST

START_TEST (test_translate_float)
{
    getSignals()[0].decoder = floatDecoder;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    fail_unless(getSignals()[0].received);

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":42}\0");
}
END_TEST

int frequencyTestCounter = 0;
openxc_DynamicField floatDecoderFrequencyTest(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    frequencyTestCounter++;
    return floatDecoder(signal, signals, signalCount, pipeline, value, send);
}

START_TEST (test_decoder_called_every_time_with_nonzero_frequency)
{
    frequencyTestCounter = 0;
    getSignals()[0].frequencyClock.frequency = 1;
    getSignals()[0].decoder = floatDecoderFrequencyTest;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

START_TEST (test_decoder_called_every_time_with_unlimited_frequency)
{
    frequencyTestCounter = 0;
    getSignals()[0].decoder = floatDecoderFrequencyTest;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    ck_assert_int_eq(frequencyTestCounter, 2);
}
END_TEST

openxc_DynamicField stringDecoder(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = {0};
    decodedValue.has_type = true;
    decodedValue.type = openxc_DynamicField_Type_STRING;
    decodedValue.has_string_value = true;
    strcpy(decodedValue.string_value, "foo");
    return decodedValue;
}

START_TEST (test_translate_string)
{
    getSignals()[0].decoder = stringDecoder;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":\"foo\"}\0");
}
END_TEST

START_TEST (test_always_send_first)
{
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_unlimited_frequency)
{
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

START_TEST (test_limited_frequency)
{
    getSignals()[0].frequencyClock.frequency = 1;
    FAKE_TIME = 2000;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
    // mock waiting 1 second
    FAKE_TIME += 1000;
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
}
END_TEST

openxc_DynamicField preserveDecoder(CanSignal* signal, CanSignal* signals,
        int signalCount, Pipeline* pipeline, float value, bool* send) {
    openxc_DynamicField decodedValue = {0};
    decodedValue.has_type = true;
    decodedValue.type = openxc_DynamicField_Type_NUM;
    decodedValue.has_numeric_value = true;
    decodedValue.numeric_value = signal->lastValue;
    return decodedValue;
}

START_TEST (test_preserve_last_value)
{
    can::read::translateSignal(&getSignals()[0],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());
    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);

    CanMessage message = {
        id: 0,
        format: STANDARD,
        data: {0x12, 0x34, 0x12, 0x30},
    };
    getSignals()[0].decoder = preserveDecoder;
    can::read::translateSignal(&getSignals()[0], &message, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"torque_at_transmission\",\"value\":-19990}\0");
}
END_TEST

START_TEST (test_dont_send_same)
{
    getSignals()[2].sendSame = false;
    getSignals()[2].decoder = booleanDecoder;
    can::read::translateSignal(&getSignals()[2], &TEST_MESSAGE, getSignals(),
            getSignalCount(), &getConfiguration()->pipeline);
    fail_if(queueEmpty());

    uint8_t snapshot[QUEUE_LENGTH(uint8_t, OUTPUT_QUEUE) + 1];
    QUEUE_SNAPSHOT(uint8_t, OUTPUT_QUEUE, snapshot, sizeof(snapshot));
    snapshot[sizeof(snapshot) - 1] = NULL;
    ck_assert_str_eq((char*)snapshot,
            "{\"name\":\"brake_pedal_status\",\"value\":true}\0");

    QUEUE_INIT(uint8_t, OUTPUT_QUEUE);
    can::read::translateSignal(&getSignals()[2],
            &TEST_MESSAGE, getSignals(), getSignalCount(), &getConfiguration()->pipeline);
    fail_unless(queueEmpty());
}
END_TEST

Suite* canreadSuite(void) {
    Suite* s = suite_create("canread");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_passthrough_decoder);
    tcase_add_test(tc_core, test_boolean_decoder);
    tcase_add_test(tc_core, test_ignore_decoder);
    tcase_add_test(tc_core, test_state_decoder);
    suite_add_tcase(s, tc_core);

    TCase *tc_sending = tcase_create("sending");
    tcase_add_checked_fixture(tc_sending, setup, NULL);
    tcase_add_test(tc_sending, test_send_numerical);
    tcase_add_test(tc_sending, test_preserve_float_precision);
    tcase_add_test(tc_sending, test_send_boolean);
    tcase_add_test(tc_sending, test_send_string);
    tcase_add_test(tc_sending, test_send_evented_boolean);
    tcase_add_test(tc_sending, test_send_evented_string);
    tcase_add_test(tc_sending, test_send_evented_float);
    tcase_add_test(tc_sending, test_passthrough_message);
    tcase_add_test(tc_sending, test_passthrough_limited_frequency);
    tcase_add_test(tc_sending, test_passthrough_force_send_changed);
    suite_add_tcase(s, tc_sending);

    TCase *tc_translate = tcase_create("translate");
    tcase_add_checked_fixture(tc_translate, setup, NULL);
    tcase_add_test(tc_translate, test_translate_float);
    tcase_add_test(tc_translate, test_translate_string);
    tcase_add_test(tc_translate, test_limited_frequency);
    tcase_add_test(tc_translate, test_unlimited_frequency);
    tcase_add_test(tc_translate, test_always_send_first);
    tcase_add_test(tc_translate, test_preserve_last_value);
    tcase_add_test(tc_translate, test_translate_ignore_decoder_still_received);
    tcase_add_test(tc_translate, test_default_decoder);
    tcase_add_test(tc_translate, test_dont_send_same);
    tcase_add_test(tc_translate, test_translate_respects_send_value);
    tcase_add_test(tc_translate,
            test_decoder_called_every_time_with_nonzero_frequency);
    tcase_add_test(tc_translate,
            test_decoder_called_every_time_with_unlimited_frequency);
    tcase_add_test(tc_translate,
            test_translate_many_signals);
    suite_add_tcase(s, tc_translate);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = canreadSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
