#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>

// WiFi credentials
#define wifi_ssid "Multiplay_6F54"
#define wifi_password "ZTENQAJGCW01384"

// MQTT server credentials
#define mqtt_server "192.168.1.69"
#define mqtt_user "admin"
#define mqtt_password "admin"

// MQTT topics
#define temperature_topic "sensor/temperature"
#define waterLvl_topic "sensor/waterLvl"
#define setTemp_topic "setting/temperature"
#define setLight_topic "setting/light"
#define setFilter_topic "setting/filter"

// Relay pins
#define GRZALKA 0 // Heater relay
#define OSWIETLENIE 4 // Lighting relay
#define FILTER 5 // Filter relay

// Water level sensor pins
#define waterLvlPower 14
#define waterLvlPin A0

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(2);
DallasTemperature sensors(&oneWire);

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length);

// Setup function
void setup() {
  Serial.begin(115200);
  sensors.begin(); 
  setup_wifi();
  
  // Connect to MQTT server
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Set relay pins as outputs and turn them off
  pinMode(GRZALKA, OUTPUT);// Relay 1
  pinMode(OSWIETLENIE, OUTPUT);// Relay 2
  pinMode(FILTER, OUTPUT);// Relay 3
  pinMode(waterLvlPower, OUTPUT);
  
  digitalWrite(GRZALKA, HIGH);
  digitalWrite(OSWIETLENIE, HIGH);
  digitalWrite(FILTER, HIGH);
  digitalWrite(waterLvlPower, LOW);
}

// Function to connect to WiFi network
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  // Wait for WiFi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to reconnect to MQTT server
void reconnect() {
  if (WiFi.status() != WL_CONNECTED) {// Check if WiFi is connected
    setup_wifi();
  }
  while (!client.connected()) {// Try to connect to MQTT server
    Serial.print("Attempting MQTT connection..."); 
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)){
      Serial.println("connected");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Variables
long lastMsg = 0;
long waterLvlTimer = 0;
int waterLvl = 0;
float temp = 0.0;
int targetTemp =0;
bool light=0;
bool filter=0;

// The main loop function
void loop() {

  // Check WiFi connection status, reconnect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  // Reconnect MQTT client if necessary
  if (!client.connected()) {
    reconnect();
  }

  // Keep the MQTT client running
  client.loop();

  // Check if it's been one second since the last iteration of the main loop
  long now = millis();
  if (now - lastMsg > 1000) {

    lastMsg = now;

    // Read water level sensor value and publish it if 3 minutes have passed
    if (now - waterLvlTimer > 180000) {
      digitalWrite(waterLvlPower, HIGH);  // Power up the sensor
      delay(10);              
      waterLvl = analogRead(waterLvlPin);    // Read the water level value
      digitalWrite(waterLvlPower, LOW);   // Turn off the power
      
      Serial.print("Water level: ");
      Serial.println(waterLvl);
      
      // Publish water level to MQTT broker
      client.publish(waterLvl_topic, String(waterLvl).c_str(), true);
      
      waterLvlTimer = now; // Reset the timer
    }

    // Subscribe to topics for setting temperature, light, and filter
    client.subscribe(setTemp_topic);
    client.subscribe(setLight_topic);
    client.subscribe(setFilter_topic);

    // Read temperature sensor value
    sensors.requestTemperatures();
    float newTemp = sensors.getTempCByIndex(0);
    
    // Check if the temperature has changed by at least 0.5°C
    if (newTemp < temp - 0.5 || newTemp > temp + 0.5) {
      temp = newTemp;
      //Serial.print("New temperature:");
      //Serial.println(String(temp).c_str());
      
      // Publish temperature to MQTT broker
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    // Turn on/off the heater based on the temperature threshold set in EEPROM
    if (temp < EEPROM.read(2)) {
      digitalWrite(GRZALKA, LOW);
    }
    if (temp >= EEPROM.read(2)) {
      digitalWrite(GRZALKA, HIGH);
    }

    // Turn on/off the light based on the value set in EEPROM
    if (EEPROM.read(0) == 1) {
      digitalWrite(OSWIETLENIE, LOW);
    }
    if (EEPROM.read(0) == 0) {
      digitalWrite(OSWIETLENIE, HIGH);
    }

    // Turn on/off the filter based on the value set in EEPROM
    if (EEPROM.read(1) == 1) {
      digitalWrite(FILTER, LOW);
    }
    if (EEPROM.read(1) == 0) {
      digitalWrite(FILTER, HIGH);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Check which topic has been subscribed
  if (strcmp(topic, setLight_topic) == 0) {
    // If the subscribed topic is for controlling the light
    Serial.print(topic);
    bool payloadL = atoi((char *)payload);
    // Convert the received payload to a boolean value
    if (payloadL == 1) {
      Serial.println("on");
      // Turn the light on and save the status to EEPROM
      light = 1;
      EEPROM.write(0, light);
      EEPROM.commit();
    }
    if (payloadL == 0) {
      Serial.println("off");
      // Turn the light off and save the status to EEPROM
      light = 0;
      EEPROM.write(0, light);
      EEPROM.commit();
    }
  }

  if (strcmp(topic, setFilter_topic) == 0) {
    // If the subscribed topic is for controlling the filter
    Serial.print(topic);
    bool payloadF = atoi((char *)payload);
    // Convert the received payload to a boolean value
    if (payloadF == 1) {
      Serial.println("on");
      // Turn the filter on and save the status to EEPROM
      filter = 1;
      EEPROM.write(1, filter);
      EEPROM.commit();
    }
    if (payloadF == 0) {
      Serial.println("off");
      // Turn the filter off and save the status to EEPROM
      filter = 0;
      EEPROM.write(1, filter);
      EEPROM.commit();
    }
  }
  
  if (strcmp(topic, setTemp_topic) == 0) {
    // If the subscribed topic is for setting the target temperature
    Serial.print(topic);
    targetTemp = atof((char *)payload);
    // Convert the received payload to a floating point value
    EEPROM.write(2, targetTemp);
    EEPROM.commit();
    Serial.println("\n");
    Serial.print(targetTemp);
  }
}