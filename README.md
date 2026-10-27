# Battery Monitoring System with ESP32

An ESP32 battery monitor with a built-in web dashboard: it reads pack voltage through a divider on the ADC, shows live battery percentage and voltage in the browser, and can drive a motor through an L298N with speed control - handy for testing how a load pulls the battery down.

## Demo

_Add a photo of the hardware or a screenshot of the dashboard here._

## Hardware

- ESP32 dev board (38-pin)
- Voltage divider on GPIO 35 (ADC), divider ratio 6.1 - sized for a 1S Li-ion/LiPo pack (4.2V full, 3.0V empty)
- L298N motor driver on GPIO 26 / 27 (motor used as a controllable test load)

## How it works

1. The ESP32 starts its own Wi-Fi access point (`ESP32_Battery`) - no router needed.
2. `esp32batterymgmtcode.ino` samples the ADC, converts to pack voltage through the divider ratio, and smooths it with a 10-reading moving average.
3. A web dashboard served by the ESP32 (port 80) shows a live battery bar, percentage and voltage, and motor controls.
4. Endpoints: `/` (dashboard), `/data` (JSON readings), `/motor` (start/stop), `/speed` (set motor speed).

## Setup

1. Wire the divider to GPIO 35 and the L298N inputs to GPIO 26/27.
2. Flash the sketch with the Arduino IDE or PlatformIO.
3. Connect your phone/laptop to the `ESP32_Battery` hotspot (default password `12345678` - change it in the sketch) and open the printed IP (usually `192.168.4.1`) in a browser.

## What this demonstrates

ESP32 ADC measurement with smoothing, analog front-end scaling (divider math), a self-hosted web dashboard on a microcontroller, and closed-loop thinking about testing a battery under a controllable load.
