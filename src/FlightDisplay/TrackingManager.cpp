/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file TrackingManager.cpp
 * @brief Manages Region of Interest (ROI) tracking selection processing
 * @author QGroundControl Development Team
 */

#include "TrackingManager.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "QGC.h"
#include "VehicleLinkManager.h"
#include "LinkInterface.h"
#include "MAVLinkProtocol.h"
#include "SettingsManager.h"
#include "AppSettings.h"
#include <QtCore/QDateTime>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostAddress>

void TrackingManager::sendROIToVehicle(float x, float y, float w, float h)
{
    // Get the active vehicle
    Vehicle* vehicle = MultiVehicleManager::instance()->activeVehicle();
    
    if (!vehicle) {
        qWarning() << "TrackingManager: No active vehicle - using UDP fallback mode";
        sendROIViaUDP(x, y, w, h);
        return;
    }

    // Get the primary link
    SharedLinkInterfacePtr sharedLink = vehicle->vehicleLinkManager()->primaryLink().lock();
    if (!sharedLink) {
        qWarning() << "TrackingManager: Primary link gone!";
        return;
    }

    // Prepare MAVLink message
    mavlink_message_t msg;
    mavlink_debug_float_array_t debugArray;

    // Set the message name to "TARGET" (case sensitive as per requirements)
    std::strncpy(debugArray.name, "TARGET", sizeof(debugArray.name) - 1);
    debugArray.name[sizeof(debugArray.name) - 1] = '\0'; // Ensure null termination

    // Initialize data array (size 58 = MAVLINK_MSG_DEBUG_FLOAT_ARRAY_FIELD_DATA_LEN)
    constexpr size_t arraySize = MAVLINK_MSG_DEBUG_FLOAT_ARRAY_FIELD_DATA_LEN;
    for (size_t i = 0; i < arraySize; ++i) {
        debugArray.data[i] = 0.0f;
    }

    // Pack ROI data in first 4 elements
    debugArray.data[0] = x;
    debugArray.data[1] = y;
    debugArray.data[2] = w;
    debugArray.data[3] = h;

    // Set timestamp (microseconds since UNIX epoch)
    debugArray.time_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch() * 1000;
    debugArray.array_id = 0;

    // Pack the message
    mavlink_msg_debug_float_array_encode_chan(
        MAVLinkProtocol::instance()->getSystemId(),
        MAVLinkProtocol::getComponentId(),
        sharedLink->mavlinkChannel(),
        &msg,
        &debugArray
    );

    // Send on link thread-safe
    vehicle->sendMessageOnLinkThreadSafe(sharedLink.get(), msg);

    qDebug() << "TrackingManager: Sent DEBUG_FLOAT_ARRAY message 'TARGET' with ROI data";
}

void TrackingManager::sendROIViaUDP(float x, float y, float w, float h)
{
    // Static socket to avoid recreating it every time
    static QUdpSocket* udpSocket = nullptr;
    if (!udpSocket) {
        udpSocket = new QUdpSocket();
        qDebug() << "TrackingManager: Created UDP socket for debug mode";
    }

    // Prepare MAVLink message
    mavlink_message_t msg;
    mavlink_debug_float_array_t debugArray;

    // Set the message name to "TARGET"
    std::strncpy(debugArray.name, "TARGET", sizeof(debugArray.name) - 1);
    debugArray.name[sizeof(debugArray.name) - 1] = '\0';

    // Initialize data array
    constexpr size_t arraySize = MAVLINK_MSG_DEBUG_FLOAT_ARRAY_FIELD_DATA_LEN;
    for (size_t i = 0; i < arraySize; ++i) {
        debugArray.data[i] = 0.0f;
    }

    // Pack ROI data in first 4 elements
    debugArray.data[0] = x;
    debugArray.data[1] = y;
    debugArray.data[2] = w;
    debugArray.data[3] = h;

    // Set timestamp (microseconds since UNIX epoch)
    debugArray.time_usec = QDateTime::currentDateTime().currentMSecsSinceEpoch() * 1000;
    debugArray.array_id = 0;

    // Pack the message - using system ID 255, component ID 0, and channel 0
    mavlink_msg_debug_float_array_encode(
        255,  // System ID (GCS)
        0,    // Component ID
        &msg,
        &debugArray
    );

    // Serialize the message to a byte buffer
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    unsigned int len = mavlink_msg_to_send_buffer(buffer, &msg);

    // Get target IP from settings
    QString targetIPString = SettingsManager::instance()->appSettings()->roiUdpTargetIP()->rawValue().toString();
    QHostAddress targetIP(targetIPString);
    quint16 targetPort = 14560;
    
    qint64 bytesSent = udpSocket->writeDatagram(reinterpret_cast<const char*>(buffer), len, targetIP, targetPort);
    
    if (bytesSent > 0) {
        qDebug() << "TrackingManager: Sent UDP Bypass Packet to" << targetIP.toString() << ":" << targetPort 
                 << "(" << bytesSent << "bytes) - ROI:" << x << y << w << h;
    } else {
        qWarning() << "TrackingManager: Failed to send UDP packet";
    }
}
