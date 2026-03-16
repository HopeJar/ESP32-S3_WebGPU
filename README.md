# ESP32-S3_Mastery
The aim of this project is to use all the functionality of the ESP32-S3 in real applications in C++.

## Build and flash (ESP-IDF + WSL)
PlatformIO is not used. Use ESP-IDF directly.

## HTTPS for WebGPU (recommended)
WebGPU requires a secure context, so serving the UI over HTTPS avoids the "WebGPU is not supported" error.

### Generate a local self-signed certificate (WSL)
```bash
cd /mnt/c/LocalRepo/ESP32-S3_Mastery/main/secrets/Hidden
openssl req -x509 -newkey rsa:2048 -keyout server_key.pem -out server_cert.pem -days 365 -nodes -subj "/CN=esp32.local"
```

### What this does
- The build system auto-embeds `server_cert.pem` and `server_key.pem` if they exist.
- The HTTP server will start HTTPS on port 443 and serve the WebGPU UI securely.
- If the files are missing, it falls back to HTTP on port 80.

### A) Device already attached to WSL
PowerShell (Admin):

```
usbipd bind --busid 1-2
usbipd attach --wsl Ubuntu --busid 1-2 --auto-attach
```

WSL (Ubuntu):

```
. ~/esp/esp-idf/export.sh
cd /mnt/c/LocalRepo/ESP32-S3_Mastery
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Exit monitor with `Ctrl+]`.

Clean builds:

```
idf.py fullclean
```

Or lighter clean:

```
idf.py clean
```

### B) Port missing (/dev/ttyACM0 not present)
PowerShell (Admin):

```
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl Ubuntu --busid <BUSID> --auto-attach
```

Then repeat steps in A.

Tip: optional shell aliases (`~/.bashrc`):

```
alias idfenv='. ~/esp/esp-idf/export.sh'
alias mon='idf.py -p /dev/ttyACM0 monitor'
```

Example:

```
idfenv && cd /mnt/c/LocalRepo/ESP32-S3_Mastery && idf.py build && idf.py -p /dev/ttyACM0 flash monitor
```
