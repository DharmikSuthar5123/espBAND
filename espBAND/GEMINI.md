# Project Documentation: espBAND

## Overview
espBAND is an Android application that enables communication between an Android device and an ESP32 over Bluetooth Low Energy (BLE). The application synchronizes media playback metadata (track title and state) from the Android device to the ESP32 and supports media playback control commands sent from the ESP32.

## Architectural Components

### 1. BLE Communication (`BleManager.java`)
- Manages the BLE GATT connection with the ESP32.
- Handles connection states, service discovery, and data transmission.
- Uses specific UUIDs for service and characteristics (`TRACK_CHAR_UUID` and `CONTROL_CHAR_UUID`).

### 2. Media Synchronization (`TimeSyncActivity.java`)
- Acts as the primary interface for syncing media data.
- **Auto-Sync:** Continuously synchronizes media metadata (title and playback state) every 1 second.
- **Media Controller:** Utilizes `MediaSessionManager` and `MediaController.TransportControls` to interface with Android media sessions.
- **Commands:** Listens for incoming media control commands from the ESP32 via BLE notifications:
    - `PP`: Play/Pause
    - `NX`: Next Track
    - `PV`: Previous Track
    - `VU`: Volume Up
    - `VD`: Volume Down

### 3. Permissions & Services (`AndroidManifest.xml`)
- Declares necessary permissions for BLE scanning/connecting, location, and notification access.
- Implements `MediaNotificationService` as a `NotificationListenerService` to facilitate media session access.

## Current Configuration & Workflows

### Auto-Connect
- Upon first successful connection to a device, the application saves the device's MAC address in `SharedPreferences` (`BlePrefs`).
- On subsequent launches, the application automatically attempts to connect to the saved device if Bluetooth is available.

### Track Synchronization Format
- Metadata is transmitted to the ESP32 as a string in the format: `TrackName*|*state` (e.g., `Song Title*|*playing`).

## Troubleshooting
- **SecurityException:** If "Access Denied" appears, ensure the app is granted "Notification Access" in the Android device's Special App Access settings.
- **Media Control:** Control commands are dispatched using `TransportControls` (media) or `AudioManager` (volume) for reliability. If commands fail, verify the ESP32 is sending valid commands.
