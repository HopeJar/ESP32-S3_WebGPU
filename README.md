# ESP32-S3_Mastery
The aim of this project is to use all the functionality of the ESP32-S3 in real applications in C++.

## Build and flash (ESP-IDF + WSL)
PlatformIO is not used. Use ESP-IDF directly.

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
