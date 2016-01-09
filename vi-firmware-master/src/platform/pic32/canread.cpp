#include "can/canread.h"
#include "canutil_pic32.h"
#include "signals.h"
#include "util/log.h"
#include "power.h"

namespace power = openxc::power;

using openxc::util::log::debug;
using openxc::signals::getCanBuses;

static CanMessage receiveCanMessage(CanBus* bus) {
    CAN::RxMessageBuffer* message = CAN_CONTROLLER(bus)->getRxMessage(
            CAN::CHANNEL1);

    CanMessage result = {
        id: message->msgSID.SID,
        format: CanMessageFormat::STANDARD,
        data: {0},
        length: (uint8_t) message->msgEID.DLC
    };
    memcpy(result.data, message->data, CAN_MESSAGE_SIZE);

    if(message->msgEID.IDE) {
        result.id <<= 18;
        result.id += message->msgEID.EID;
        result.format = CanMessageFormat::EXTENDED;
    }
    return result;
}

void openxc::can::pic32::handleCanInterrupt(CanBus* bus) {
    // handle the bus activity wake from sleep event
    if((CAN_CONTROLLER(bus)->getModuleEvent() &
                CAN::BUS_ACTIVITY_WAKEUP_EVENT) != 0
            && CAN_CONTROLLER(bus)->getPendingEventCode()
                == CAN::WAKEUP_EVENT) {
        power::handleWake();
    }

    // handle the receive message event
    if((CAN_CONTROLLER(bus)->getModuleEvent() & CAN::RX_EVENT) != 0
            && CAN_CONTROLLER(bus)->getPendingEventCode()
            == CAN::CHANNEL1_EVENT) {
        // Clear the event so we give up control of the CPU
        CAN_CONTROLLER(bus)->enableChannelEvent(CAN::CHANNEL1,
                CAN::RX_CHANNEL_NOT_EMPTY, false);

        CanMessage message = receiveCanMessage(bus);
        if(!QUEUE_PUSH(CanMessage, &bus->receiveQueue, message)) {
            // An exception to the "don't leave commented out code" rule,
            // this log statement is useful for debugging performance issues
            // but if left enabled all of the time, it can can slown down
            // the interrupt handler so much that it locks up the device in
            // permanent interrupt handling land.
            //
            // debug("Dropped CAN message with ID 0x%02x -- queue is full with %d",
                    // message.id, QUEUE_LENGTH(CanMessage, &bus->receiveQueue));
            ++bus->messagesDropped;
        }

        /* Call the CAN::updateChannel() function to let the CAN module know
         * that the message processing is done. Enable the event so that the
         * CAN module generates an interrupt when the event occurs.*/
        CAN_CONTROLLER(bus)->updateChannel(CAN::CHANNEL1);
        CAN_CONTROLLER(bus)->enableChannelEvent(CAN::CHANNEL1,
                CAN::RX_CHANNEL_NOT_EMPTY, true);
    }
}
