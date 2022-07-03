# IRA2020Arduino
Arduino code for the IRA 2020 PCB

Install
* Install Visual Studio Code
* In VSCode extensions install PlatformIO, Arduino
* Install CH340C driver from here: http://www.wch-ic.com/downloads/CH341SER_ZIP.html

Configure
* Change WIFI & PSK to your local Wifi settings
* in Platformio.ini change upload_port to your local COM port
* 

NATS Clients in WSL
* first install GO:  sudo apt install golang-go
* Install pub & sub: 
** go get github.com/nats-io/go-nats-examples/tools/nats-pub
** go get github.com/nats-io/go-nats-examples/tools/nats-sub
* Add ~/go/bin to PATH
* nats-pub -s nats://demo.nats.io area3001.blink 10
* ./nats-sub -s nats://demo.nats.io:4222 "area3001.*"

NATS Server from Source:
* go get github.com/nats-io/nats-streaming-server

SOME RGB Strings
*  AAQD/wAAAP8AAAD///// = 4 pixel RGB = R, G, B, Wh

SOME FX Strings
* AAAA/wAAAP8A = fast red/green loop
* AAoA/wAAAP8A = slow red/green loop
* AQoA////AAAA = random white pixels
* AgoA////AAD/ = blue/white fgnd/bgnd loop
* BHg3AKoAqgCq = Fire FX

ENCODING WEBSITE: https://base64.guru/converter/encode/hex

https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/


TODO:
* NAME NATS CB + EEPROM SPACE + FIXED length 32 bytes
* use header config to determine to which server to connect
