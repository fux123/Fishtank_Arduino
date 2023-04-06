The goal of this project was to design a controller and develop a system for it, that
would allow to monitor and control conditions inside an fish tank. For this purpose,
an open-source smart home management software called ”Home Assistant” was used.
The controller worked on the NodeMCU ESP8266 development kit connected to the
local Wi-Fi network, which allowed the exchange of data with Home Assistant via the
MQTT protocol. User could select the conditions settings in the Home Assistant’s main
panel, and the controller kept them by controlling aquarium’s accesories.

This code is an ESP8266 microcontroller program that reads and controls various sensors and relays,
and communicates with an MQTT server to publish and subscribe to messages on different topics. 
The program reads the water level sensor every three minutes and publishes it to the MQTT broker.
It subscribes to topics for setting temperature, light, and filter and turns on or off the corresponding relays.
It reads the temperature sensor value and publishes it to the MQTT broker if it has changed by at least 0.5°C.
It turns on or off the heater based on the temperature threshold set in EEPROM.
