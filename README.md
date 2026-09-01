# ESP32-Smart-RFID-Clock

An advanced, Wi-Fi connected alarm clock built on the **ESP32**, designed for those who struggle to get out of bed. It forces you to physically get up and scan an RFID tag to turn off the alarm. The device features a web interface for easy configuration, NTP time synchronization, and high-quality I2S audio playback for custom alarms.

## ✨ Features

- **RFID Alarm Dismissal**: Turn off your morning alarm by scanning an RFID card or tag. Place the tag in another room to force yourself out of bed!
- **High-Quality Audio**: Uses a MAX98357A I2S amplifier for clear, loud `.wav` file playback.
- **Custom Sounds**: Upload custom `.wav` files (up to 500KB) directly via the built-in web interface. Stored safely in the ESP32's LittleFS storage.
- **Web Interface**: Manage your alarms, upload sounds, and check Wi-Fi/NTP status from your phone or computer browser.
- **Wi-Fi & NTP Time Sync**: Always maintains accurate time synced via internet NTP servers. No manual time setting required.
- **Built-in Fallback Melody**: Includes a built-in melody if no custom sounds are uploaded.

## 🛠 Hardware Requirements

- **ESP32** Development Board (e.g., NodeMCU-32S, ESP32 WROOM)
- **RFID-RC522** Module (SPI Interface)
- **MAX98357A** I2S Audio Amplifier Breakout
- **4 Ohm or 8 Ohm Speaker** (3W recommended)
- **RFID Tags/Cards** (13.56MHz Mifare)
- Jumper wires and breadboard/PCB

## 🔌 Pinout & Wiring

For detailed step-by-step wiring instructions, please see the [Hardware_Pinout.md](./Hardware_Pinout.md) guide included in this repository.

### Quick Summary:
- **RFID-RC522**: Connects to the ESP32's VSPI pins (`GPIO 5`, `18`, `19`, `23`) and `GPIO 22` for Reset. **(3.3V Power Only)**
- **MAX98357A**: Connects to the ESP32's I2S pins (`GPIO 25`, `26`, `27`). 

## 🚀 Installation & Setup

1. **Clone the repository:**
   Download or clone this project to your local machine.

2. **Configure Wi-Fi Credentials:**
   - Copy `env.example.h` and rename it to `env.h`
   - Open `env.h` and enter your local Wi-Fi `SSID` and `PASSWORD`.

3. **Open in Arduino IDE:**
   - Ensure you have the **ESP32 Board Package** installed in your Arduino IDE (version 2.0.x or higher).
   - Open `SmartRFIDAlarmClock.ino`.

4. **Install Required Libraries:**
   Make sure you have installed the following libraries via the Arduino Library Manager:
   - **MFRC522** by GithubCommunity (for the RFID module)
   - Other core libraries (`LittleFS`, `WiFi`, `WebServer`, `driver/i2s.h`) are included by default in the ESP32 core package.

5. **Upload the Code:**
   - Select your ESP32 board and COM port.
   - Click **Upload**.

6. **Upload the LittleFS Filesystem (Optional but recommended):**
   - If you have default WAV files you wish to pre-load, place them in a folder named `data` inside the sketch directory.
   - Use the **ESP32 Sketch Data Upload** tool to upload them to the LittleFS partition. (You can also just upload sounds via the web interface later).

## 📱 Usage

1. **Power On**: Once powered, the ESP32 will connect to your configured Wi-Fi network and synchronize the time.
2. **Access the Web Interface**: Open the Serial Monitor at `115200` baud to find the IP address of the clock, or check your router's DHCP list. Enter that IP address in your web browser.
3. **Configure**: Use the web dashboard to set your alarm time, toggle the alarm on/off, and upload new `.wav` ringtones.
4. **Wake Up**: When the alarm sounds, the only way to silence it is to bring your designated RFID tag near the RC522 reader.

## 📄 License
This project is open-source. Feel free to modify, distribute, and build upon it!
