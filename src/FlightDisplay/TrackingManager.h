/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file TrackingManager.h
 * @brief Manages Region of Interest (ROI) tracking selection processing
 * @author QGroundControl Development Team
 */

#pragma once

#include <QObject>
#include <QDebug>
#include <cstring>

// Forward declarations
class QGCApplication;
class MultiVehicleManager;
class Vehicle;

/**
 * @class TrackingManager
 * @brief Handles normalized ROI coordinates from QML and prepares them for transmission
 */
class TrackingManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject for memory management
     */
    explicit TrackingManager(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    /**
     * @brief Destructor
     */
    ~TrackingManager() override = default;

    // Disable copy and move operations
    TrackingManager(const TrackingManager&) = delete;
    TrackingManager& operator=(const TrackingManager&) = delete;
    TrackingManager(TrackingManager&&) = delete;
    TrackingManager& operator=(TrackingManager&&) = delete;

    /**
     * @brief Process ROI selection from QML
     * @param x Normalized X coordinate (0.0 to 1.0)
     * @param y Normalized Y coordinate (0.0 to 1.0)
     * @param w Normalized width (0.0 to 1.0)
     * @param h Normalized height (0.0 to 1.0)
     * 
     * Validates the selection and prepares it for transmission to the vehicle.
     * Rejects selections that are too small (< 1% in either dimension).
     */
    Q_INVOKABLE void processSelection(float x, float y, float w, float h)
    {
        // Sanity check: ignore tiny accidental clicks
        constexpr float kMinimumSelectionSize = 0.01f;
        
        if (w < kMinimumSelectionSize || h < kMinimumSelectionSize) {
            qDebug() << "TrackingManager: Selection too small, ignoring."
                     << "Width:" << w << "Height:" << h;
            return;
        }

        // Additional bounds checking
        if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f ||
            w < 0.0f || w > 1.0f || h < 0.0f || h > 1.0f) {
            qWarning() << "TrackingManager: Invalid normalized coordinates."
                       << "x:" << x << "y:" << y << "w:" << w << "h:" << h;
            return;
        }

        // Ensure selection doesn't exceed bounds
        if (x + w > 1.0f || y + h > 1.0f) {
            qWarning() << "TrackingManager: Selection exceeds normalized bounds."
                       << "x:" << x << "y:" << y << "w:" << w << "h:" << h;
            return;
        }

        // Log the valid selection
        qDebug() << QString("Transmitting ROI: [%1, %2, %3, %4]")
                    .arg(x, 0, 'f', 3)
                    .arg(y, 0, 'f', 3)
                    .arg(w, 0, 'f', 3)
                    .arg(h, 0, 'f', 3);

        // Send ROI via MAVLink DEBUG_FLOAT_ARRAY
        sendROIToVehicle(x, y, w, h);
        
        emit roiProcessed(x, y, w, h);
    }

signals:
    /**
     * @brief Emitted when a valid ROI has been processed
     * @param x Normalized X coordinate
     * @param y Normalized Y coordinate
     * @param w Normalized width
     * @param h Normalized height
     */
    void roiProcessed(float x, float y, float w, float h);

private:
    /**
     * @brief Send ROI data to the active vehicle via MAVLink DEBUG_FLOAT_ARRAY
     * @param x Normalized X coordinate
     * @param y Normalized Y coordinate
     * @param w Normalized width
     * @param h Normalized height
     */
    void sendROIToVehicle(float x, float y, float w, float h);
    
    /**
     * @brief Send ROI data via UDP when no vehicle is connected (debug mode)
     * @param x Normalized X coordinate
     * @param y Normalized Y coordinate
     * @param w Normalized width
     * @param h Normalized height
     */
    void sendROIViaUDP(float x, float y, float w, float h);
};