# Morpheuz 2.0 - Sleep & Activity Tracker for Pebble

Morpheuz 2.0 is a powerful health and activity monitoring application for Pebble smartwatches. It combines advanced sleep analysis with background step counting and provides native integration with smart home ecosystems like Home Assistant.

## Key Features

*   **Background Step Counting**: Uses Pebble's background worker (or internal sensors) to track steps continuously, even when the main application is closed.
*   **Intelligent Sleep Monitoring**: Automatically analyzes wrist movement to categorize sleep into **Deep Sleep**, **Light Sleep**, and **Awake** phases based on configurable thresholds.
*   **7-Day Wrist History**: Includes built-in bar charts to visualize your activity and sleep quality trends over the last week directly on the watch.
*   **Home Assistant Integration**: Seamlessly pushes your activity data (steps, sleep duration, and status) to a Home Assistant sensor via REST API using long-lived access tokens.
*   **Smart Sync & Reset Logic**: To ensure data integrity, the daily stats are archived and reset only upon the first Bluetooth connection of the day, allowing you to check yesterday's totals before they are synchronized and cleared.
*   **Automatic Sleep Mode**: The app can be configured to start monitoring automatically when Bluetooth connection is lost, making it ideal for users who leave their phone charging in another room overnight.
*   **Minimalist UI**: A clean, high-contrast interface designed for quick readability, showing daily steps and current sleep status at a glance.

## Home Assistant Integration

The app communicates with Home Assistant through the PebbleKit JS layer. It updates an entity (default: `sensor.igi_activity`) with the following data:
*   **State**: Total daily steps.
*   **Attributes**:
    *   `deep_sleep_min`: Total minutes of deep sleep.
    *   `light_sleep_min`: Total minutes of light sleep.
    *   `awake_min`: Minutes detected as awake during a sleep session.
    *   `total_sleep_min`: Combined deep and light sleep duration.
    *   `snoozes`: Number of alarm snoozes registered.
    *   `friendly_name`: "Igi Activity Monitor".

## Configuration

Configuration is managed through a dedicated offline page (Pebble Clay):
1.  **HA URL**: The base URL of your Home Assistant instance (e.g., `http://192.168.1.10:8123`).
2.  **HA Token**: Your Long-Lived Access Token generated from the Home Assistant profile page.

## Controls

*   **UP**: View the 7-day Steps History chart.
*   **SELECT**: Open a confirmation menu to manually reset daily steps.
*   **DOWN**: View the 7-day Sleep History chart.
*   **BACK (Long Press)**: Close the application.

## Technical Specifications

*   **Platforms**: Compatible with Pebble Rectangular (Aplite, Basalt) and Round (Chalk) displays.
*   **Persistence**: Uses Persistent Storage to maintain data logs locally.
*   **Connectivity**: Uses `appmessage` for watch-to-phone communication and `XMLHttpRequest` for REST API calls.

## Credits

Original code by James Fowler. Custom enhancements, Home Assistant integration, and background worker logic by Igino.

---
*Morpheuz 2.0 - Tracking your movement, protecting your rest.*