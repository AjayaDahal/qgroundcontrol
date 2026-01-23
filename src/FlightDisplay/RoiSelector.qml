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
    property real _pressX: 0.0      // Initial press position
    property real _pressY: 0.0
    property real _currentX: 0.0    // Current mouse position
    property real _currentY: 0.0
    property bool _isDragging: false

    // Computed properties for normalized rectangle (always positive dimensions)
    property real _rectX: Math.min(_pressX, _currentX)
    property real _rectY: Math.min(_pressY, _currentY)
    property real _rectWidth: Math.abs(_currentX - _pressX)
    property real _rectHeight: Math.abs(_currentY - _pressY)

    // Mouse interaction area
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: true
        hoverEnabled: false

        onPressed: function(mouse) {
            if (!root.visible) return
            
            // Store initial press position
            _pressX = mouse.x
            _pressY = mouse.y
            _currentX = mouse.x
            _currentY = mouse.y
            _isDragging = true
        }

        onPositionChanged: function(mouse) {
            if (_isDragging) {
                // Update current position - computed properties handle normalization
                _currentX = mouse.x
                _currentY = mouse.y
            }
        }

        onReleased: function(mouse) {
            if (_isDragging) {
                // Update final position
                _currentX = mouse.x
                _currentY = mouse.y
                _isDragging = false

                // Use the normalized rectangle properties for ROI calculation
                calculateAndEmitROI()
            }
        }

        onCanceled: {
            _isDragging = false
        }
    }

    // Visual feedback rectangle - uses computed normalized properties
    Rectangle {
        id: selectionBox
        visible: _isDragging
        color: "transparent"
        border.color: "#FF0000"  // Red
        border.width: 2

        // Use computed properties that are always normalized (positive dimensions, true top-left)
        x: _rectX
        y: _rectY
        width: _rectWidth
        height: _rectHeight
    }

    // Calculate and emit normalized ROI coordinates using the corrected rectangle properties
    function calculateAndEmitROI() {
        // Ensure we have valid dimensions
        if (root.width <= 0 || root.height <= 0) {
            console.warn("RoiSelector: Invalid parent dimensions")
            return
        }

        // Use the already-normalized rectangle properties
        // These are guaranteed to have positive dimensions and correct top-left corner
        var screenX = _rectX
        var screenY = _rectY
        var screenW = _rectWidth
        var screenH = _rectHeight

        // Clamp to valid screen bounds
        screenX = Math.max(0, Math.min(screenX, root.width))
        screenY = Math.max(0, Math.min(screenY, root.height))
        var maxX = Math.min(screenX + screenW, root.width)
        var maxY = Math.min(screenY + screenH, root.height)
        
        // Recalculate width/height after clamping
        screenW = maxX - screenX
        screenH = maxY - screenY

        // Calculate normalized coordinates (0.0 to 1.0)
        var normX = screenX / root.width
        var normY = screenY / root.height
        var normW = screenW / root.width
        var normH = screenH / root.height

        // Debug output
        console.log("RoiSelector: Screen coords:", screenX, screenY, screenW, screenH)
        console.log("RoiSelector: Normalized:", normX.toFixed(3), normY.toFixed(3), normW.toFixed(3), normH.toFixed(3))

        // Only emit if we have a valid selection (non-zero area)
        if (normW > 0.001 && normH > 0.001) {
            roiSelected(normX, normY, normW, normH)
        } else {
            console.log("RoiSelector: Selection too small, ignoring")
        }
    }
}
