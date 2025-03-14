# E-paper Dashboard

## Description

This project consists of a dashboard displayed on an E-Paper screen, showing the most important information of the day: weather conditions, rain probability, bus stops (in this case, using GTT stops), and a special calendar synchronization between two different calendars. The calendars can belong to the same person or two individuals who want to share their tasks quickly and efficiently on a daily basis.

The electronics, HTTP server, and HTTP client are implemented in C and C++, while the frontend of the configuration website is written in HTML, CSS, and JavaScript.

The tasks are managed using a multithreading approach with FreeRTOS.

## Demo

TODO

## Installation

### Prerequisites

- ESP-IDF installed and properly configured
- ESP32 toolchain
- Python 3 with `pip` installed
- An ngrok account with a static domain configured
- A Google Calendar account with API access enabled
- A static IP configured for the ESP32 in the router
- A Google account with a valid email and Google Calendar API enabled

## Configuration

### Software

1. Rename `.env.example` to `.env` and `start_ngrok.sh.example` to `start_ngrok.sh`
2. Run `fetch.py`, and in a separate shell, execute `./start_ngrok.sh`
3. Place a `cacert.pem` file containing the necessary SSL certificates in the `spiffs` directory to enable HTTPS communication

To compile and flash the firmware:

```sh
idf.py build
idf.py flash
idf.py monitor
```

### Hardware

TODO

# Project structure
The project is structured as follow:
- `components/`
    - `Adafruit-GFX/`- A graphics library used for drawing on displays. [Github](https://github.com/martinberlin/Adafruit-GFX-Library-ESP-IDF.git)
    - `CalEPD/` - A library for controlling e-paper displays. [Github](https://github.com/martinberlin/CalEPD.git)
    - `client/` - Manages communication with external APIs, including HTTPS requests and OAuth2 authentication.
    - `env_var/` - Handles environment variables from a `.env` file
    - `peripherals/` - TODO, handles sensors
    - `screen/` - Manages the display logic, including rendering the timeline and updating visuals.
    - `server/` - Implements the embedded HTTP server for handling requests
    - `utils/` - Utility functions and helper modules used throughout the project.
- `main/` - Entry point
- `spiffs/` - Stores files in the SPIFFS (SPI Flash File System), used for configuration and assets
- `partitions.csv` - Defines the partition table for the firmware.
- `.env.example` - Example of `.env`
- `feth.py` - Converts `.env` into a header file
- `start_ngrok.sh.example` - Performs port forwarding to obtain the redirect code from the Google API

## Acknowledgments

A special thanks to:
- [madbob](https://github.com/madbob) for the GTT API
- [martinberlin](https://github.com/martinberlin) for the e-paper utilities


## License

This project is licensed under the **GNU General Public License v3.0 (GPL-3.0)**.  
You are free to use, modify, and distribute this software, but **any modifications must also be released under the same license**.  

See the [LICENSE](LICENSE) file for more details.  
