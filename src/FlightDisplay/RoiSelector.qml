/****************************************************************************
 *
 * (c) 2009-2026 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

/**
 * @file RoiSelector.qml
 * @brief Region of Interest (ROI) selection tool for object tracking
 * @author QGroundControl Development Team
 */

import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools

Item {
    id: root

    // Signal emitted when ROI selection is complete
    // Parameters are normalized coordinates (0.0 to 1.0)
    signal roiSelected(real x, real y, real w, real h)

    // Private properties for tracking drag state
    property real _startX: 0.0
    property real _startY: 0.0
    property real _endX: 0.0
    property real _endY: 0.0
    property bool _isDragging: false

    // Mouse interaction area
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: false

        onPressed: function(mouse) {
            if (!root.visible) return
            _startX = mouse.x
            _startY = mouse.y
            _endX = mouse.x
            _endY = mouse.y
            _isDragging = true
        }

        onPositionChanged: function(mouse) {
            if (_isDragging) {
                _endX = mouse.x
                _endY = mouse.y
            }
        }

        onReleased: function(mouse) {
            if (_isDragging) {
                _endX = mouse.x
                _endY = mouse.y
                _isDragging = false

                // Calculate normalized coordinates
                calculateAndEmitROI()
            }
        }

        onCanceled: {
            _isDragging = false
        }
    }

    // Visual feedback rectangle
    Rectangle {
        id: selectionBox
        visible: _isDragging
        color: "transparent"
        border.color: "#FF0000"  // Red
        border.width: 2

        x: Math.min(_startX, _endX)
        y: Math.min(_startY, _endY)
        width: Math.abs(_endX - _startX)
        height: Math.abs(_endY - _startY)
    }

    // Calculate and emit normalized ROI coordinates
    function calculateAndEmitROI() {
        // Ensure we have valid dimensions
        if (root.width <= 0 || root.height <= 0) {
            console.warn("RoiSelector: Invalid parent dimensions")
            return
        }

        // Calculate bounding box in screen coordinates
        var minX = Math.min(_startX, _endX)
        var minY = Math.min(_startY, _endY)
        var maxX = Math.max(_startX, _endX)
        var maxY = Math.max(_startY, _endY)

        // Clamp to valid screen bounds
        minX = Math.max(0, Math.min(minX, root.width))
        minY = Math.max(0, Math.min(minY, root.height))
        maxX = Math.max(0, Math.min(maxX, root.width))
        maxY = Math.max(0, Math.min(maxY, root.height))

        // Calculate normalized coordinates (0.0 to 1.0)
        var normX = minX / root.width
        var normY = minY / root.height
        var normW = (maxX - minX) / root.width
        var normH = (maxY - minY) / root.height

        // Only emit if we have a valid selection (non-zero area)
        if (normW > 0.001 && normH > 0.001) {
            roiSelected(normX, normY, normW, normH)
        } else {
            console.log("RoiSelector: Selection too small, ignoring")
        }
    }
}
