# 🔌 Hardware Pinout & Wiring Guide

This document provides a comprehensive wiring guide for the Smart RFID Alarm Clock. The project is based on an **ESP32** microcontroller and interfaces with an **RFID-RC522** module for card reading and a **MAX98357A I2S Amplifier** for audio output.

## 📌 Pin Layout Overview

| Component | Pin Name | ESP32 Pin | Notes |
| :--- | :--- | :--- | :--- |
| **RFID-RC522** | SDA (SS) | `GPIO 5` | SPI Chip Select |
| | RST | `GPIO 22` | Reset Pin |
| | SCK | `GPIO 18` | SPI Clock |
| | MISO | `GPIO 19` | SPI Master In Slave Out |
| | MOSI | `GPIO 23` | SPI Master Out Slave In |
| | 3.3V | `3.3V` | **Must be 3.3V** (5V will damage it) |
| | GND | `GND` | Ground |
| | | | |
| **MAX98357A** (I2S Audio)| BCLK | `GPIO 27` | Bit Clock |
| | LRC | `GPIO 26` | Left/Right Word Clock |
| | DIN | `GPIO 25` | Data In (Connected to DOUT of ESP32) |
| | VIN | `5V` or `3.3V`| VCC/Power |
| | GND | `GND` | Ground |

---

## 🛠 Detailed Wiring Instructions

### 1. RFID RC522 Module (SPI Interface)
The RFID module uses the standard VSPI bus of the ESP32.

*   **SDA (SS)** ➡️ Connect to **GPIO 5**
*   **SCK** ➡️ Connect to **GPIO 18**
*   **MOSI** ➡️ Connect to **GPIO 23**
*   **MISO** ➡️ Connect to **GPIO 19**
*   **IRQ** ➡️ *Not connected* (Not required for this project)
*   **GND** ➡️ Connect to ESP32 **GND**
*   **RST** ➡️ Connect to **GPIO 22**
*   **3.3V** ➡️ Connect to ESP32 **3.3V** ⚠️ *(Critical: Do not connect to 5V)*

### 2. MAX98357A I2S Amplifier (Audio)
The amplifier converts digital I2S audio into an analog signal for the speaker. 

*   **LRC** ➡️ Connect to **GPIO 26**
*   **BCLK** ➡️ Connect to **GPIO 27**
*   **DIN** ➡️ Connect to **GPIO 25**
*   **GND** ➡️ Connect to ESP32 **GND**
*   **VIN** ➡️ Connect to ESP32 **VIN / 5V** (Can also work on 3.3V, but 5V provides better volume and audio quality)

**Speaker Connection:**
*   Connect the positive and negative terminals of your speaker directly to the **+** and **-** screw terminals on the MAX98357A board.

---

> [!NOTE] 
> If you decide to change these pins in the future, remember to update the corresponding definitions in `Pins.h` before recompiling the project.
