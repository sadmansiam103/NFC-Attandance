# ESP32 NFC Attendance Management System

An NFC-based attendance management system built with the **ESP32 DevKit V1** and **PN532 NFC module**. The project is designed to identify users through NFC cards, manage registered users through a local web-based admin panel, store records in the ESP32's LittleFS filesystem, and send attendance data to Google Sheets.

## Overview

Traditional attendance systems often require manual entry or expensive dedicated equipment. This project explores a low-cost, microcontroller-based alternative using NFC technology and Wi-Fi connectivity.

Users scan their NFC cards at the reader. The ESP32 reads the card's unique identifier (UID), checks whether the card belongs to a registered user, provides audio feedback through a buzzer, and sends attendance data to a Google Sheets spreadsheet through Google Apps Script.

The system also includes local data storage and a web-based administration interface hosted directly on the ESP32.

## Features

- **NFC-based identification:** Reads NFC card UIDs using the PN532 module.
- **User management:** Supports adding and removing registered users.
- **Local data storage:** Uses LittleFS to store user information on the ESP32.
- **Web-based administration:** Hosts a local admin interface for managing users.
- **Google Sheets integration:** Sends attendance data to a spreadsheet through Google Apps Script.
- **Audio feedback:** Uses a buzzer to indicate card-scanning events.
- **Wi-Fi connectivity:** Connects the ESP32 to a wireless network for online attendance logging.
- **Standalone local storage:** Retains a local user database independently of Google Sheets.

## Hardware Requirements

| Component | Purpose |
|---|---|
| ESP32 DevKit V1 | Main microcontroller and Wi-Fi connectivity |
| PN532 NFC Module | Reads NFC card identifiers |
| Compatible NFC Cards or Tags | User identification |
| Buzzer | Audio feedback |
| Breadboard and Jumper Wires | Circuit assembly |
| USB Power Supply | Powers and programs the ESP32 |

## Software and Technologies

- Arduino IDE
- C/C++
- ESP32 Arduino Core
- SPI communication
- Adafruit PN532 library
- LittleFS filesystem
- ArduinoJson
- ESP32 WebServer
- Wi-Fi and HTTPClient libraries
- Google Apps Script
- Google Sheets

## System Architecture

```text
        NFC Card / Tag
               |
               v
         PN532 NFC Reader
               |
           SPI Interface
               |
               v
          ESP32 DevKit V1
               |
       +-------+--------+
       |       |        |
       v       v        v
    LittleFS  Buzzer   Wi-Fi
       |                |
       v                v
  Local User DB    Google Apps Script
                        |
                        v
                  Google Sheets

       ESP32 Web Server
               |
               v
       Admin Web Interface
       User Management
```

## How It Works

1. **Card scanning:** A user places an NFC card near the PN532 reader.
2. **UID extraction:** The PN532 reads the card's unique identifier and passes it to the ESP32.
3. **User verification:** The ESP32 checks the UID against registered users stored in LittleFS.
4. **Access feedback:** The buzzer provides feedback based on whether the card is recognized.
5. **Attendance logging:** For a recognized user, the ESP32 sends attendance information to Google Apps Script when network connectivity is available.
6. **Cloud recording:** Google Apps Script processes the request and records the attendance information in Google Sheets.
7. **User administration:** An administrator can access the ESP32-hosted web interface to add or remove users.

## PN532 to ESP32 Wiring

The following pin assignments are the planned SPI configuration for the ESP32 DevKit V1.

| PN532 Pin | ESP32 GPIO |
|---|---:|
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| SS / SDA | GPIO 5 |
| VCC | 3.3V, subject to module specifications |
| GND | GND |

**Buzzer:** Connect the buzzer's control terminal to GPIO 27 when using the proposed configuration. Check the buzzer's voltage and current requirements before connecting it directly.

**Important:** Set the PN532 module to SPI mode. Pin labels and power requirements can vary between PN532 breakout boards, so verify your module's documentation before wiring it.

## Installation and Setup

### 1. Install the required software

Install the Arduino IDE and configure the ESP32 board package.

Install the following libraries through the Arduino IDE Library Manager where available:

- Adafruit PN532
- ArduinoJson

The ESP32-specific libraries for Wi-Fi, SPI, HTTP requests, WebServer, and LittleFS are generally provided by the ESP32 Arduino Core.

### 2. Configure the ESP32

Open the project in Arduino IDE and select the appropriate ESP32 board and serial port.

Configure the Wi-Fi credentials and any project-specific settings in the source code.

### 3. Configure Google Sheets

1. Create a Google spreadsheet for attendance records.
2. Open Google Apps Script from the spreadsheet.
3. Add the server-side script that receives attendance requests.
4. Deploy the script as a web app.
5. Copy the deployment URL into the ESP32 firmware configuration.
6. Configure the script's access permissions appropriately and test the endpoint.

Keep any credentials or sensitive configuration out of public repositories.

### 4. Upload the firmware

Connect the ESP32 to your computer through USB and upload the firmware using Arduino IDE.

Initialize the LittleFS filesystem and ensure that the expected user database file exists before testing card registration and attendance logging.

### 5. Test the system

- Scan a registered NFC card.
- Scan an unregistered NFC card.
- Verify buzzer feedback.
- Test adding and removing users through the admin interface.
- Confirm that attendance requests reach Google Apps Script.
- Verify that records appear in Google Sheets.
- Test local user verification when Wi-Fi is unavailable.

## Data Storage

The ESP32 uses LittleFS for local data storage. A JSON file such as `users.json` can hold registered user information and their associated NFC UIDs.

Google Sheets serves as the remote attendance record, while LittleFS maintains the local user database.

The exact JSON schema, attendance columns, and timestamp handling depend on the firmware and Google Apps Script implementation.

## Security Considerations

- Restrict access to the local administration interface.
- Add authentication before exposing user-management functions.
- Validate requests received by Google Apps Script.
- Avoid publishing Wi-Fi passwords, private deployment URLs, or other credentials.
- Remember that an NFC UID alone is not strong authentication because some cards can be cloned or their identifiers emulated.
- Use HTTPS for communication with Google Apps Script.

## Current Scope

The project is designed around NFC identification, local user management, ESP32-hosted administration, and Google Sheets attendance logging.

Features such as administrator authentication, dedicated IN/OUT tracking, detailed attendance-history views, and robust offline-to-online synchronization require separate implementation and verification before they can be described as completed features.

## Future Improvements

- Add secure administrator authentication.
- Implement automatic attendance status tracking.
- Add attendance history and reporting.
- Improve offline attendance queuing and synchronization.
- Add duplicate-scan prevention.
- Introduce configurable attendance rules.
- Improve the admin interface for mobile devices.
- Add export and backup options for attendance records.

## Project Goals

This project aims to demonstrate how embedded systems, NFC technology, local storage, web interfaces, and cloud-based spreadsheets can work together to create an affordable attendance management solution.

## Author

**Sadman Siam**

Electrical and Electronic Engineering Student  
American International University-Bangladesh (AIUB)

---

*Built as an embedded systems project exploring NFC, ESP32, and IoT-based attendance management.*
