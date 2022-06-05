


## Build & run:

```
C:\Users\bdomo\.platformio\penv\Scripts\Activate.ps1
$env:WIFI_SSID='\"yourssid\"'
$env:WIFI_PASS='\"yourpw\"'

pio run -t upload 
pio device monitor
```

## CA Cert

azure-sdk-for-c\sdk\samples\iot\aziot_esp32\New-TrustedCertHeader.ps1
https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-x509ca-concept


https://github.com/Azure/azure-sdk-for-c-arduino/blob/main/examples/Azure_IoT_Central_ESP32/readme.md
https://github.com/Azure/azure-sdk-for-c-arduino/tree/main/examples/Azure_IoT_Hub_ESP32

Platformio
https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html

Platformio libs:
https://docs.platformio.org/en/latest/projectconf/section_env_library.html#lib-deps
https://registry.platformio.org/search

## Troubleshooting:

Windows 11 - upload doesn't work - serial driver issue
Need to install driver as written below
https://community.platformio.org/t/esp32-not-detected-as-device-in-upload-port/23676/3

