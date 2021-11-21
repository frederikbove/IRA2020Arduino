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

NATS Server from Source:
* go get github.com/nats-io/nats-streaming-server

