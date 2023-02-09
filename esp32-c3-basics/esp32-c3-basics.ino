// =====================================================================================================================
//
// esp32-c3-basics
//
//    - SPDX-License-Identifier: GPL-2.0-only
//
// =====================================================================================================================
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

// baud rate used for arduino ide serial monitor
static const uint32_t SERIAL_MONITOR_BAUD = 115200;

// pin definitions for 'GOOUUU-ESP32-C3'
static const uint8_t PIN_BOOT_BUTTON   = 9;  // boot button right of usb port
static const uint8_t PIN_RGB_LED_RED   = 3;  // rgb led - red   [low active]
static const uint8_t PIN_RGB_LED_GREEN = 4;  // rgb led - green [low active]
static const uint8_t PIN_RGB_LED_BLUE  = 5;  // rgb led -  blue [low active]
static const uint8_t PIN_TX_LED_BLUE   = 21; // blue tx led     [high active]

// mqtt definitions
static const uint32_t MQTT_CONNECT_INTERVAL      = 10000;
static const char*    MQTT_SERVER_ADDRESS        = "test.mosquitto.org";
static const uint16_t MQTT_SERVER_PORT           = 1883;
static const char*    MQTT_PUB_TOPIC_TEMPERATURE = "/Temperature";

// loop interval
static const uint32_t LOOP_INTERVAL = 60000;

// function declarations
void gpio_setup();
void wifi_setup();
void mqtt_reconnect();
void mqtt_publish(int8_t temperature);

// global variable definitions
static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);



// =====================================================================================================================
// function: setup()
// =====================================================================================================================
void setup() {

  // setup serial monitor and add setup delay to allow
  // the press of boot button for wifi reset during start
  Serial.begin(SERIAL_MONITOR_BAUD);
  Serial.println("Starting ESP32-C3 setup in 3s");
  delay(3000);
  Serial.println("Setting up ESP32-C3");

  // setup pins
  gpio_setup();

  // setup wifi
  wifi_setup();

  // setup mqtt
  mqtt_setup();

  Serial.println("Finished ESP32-C3 setup");
}



// =====================================================================================================================
// function: loop()
// =====================================================================================================================
void loop() {

  // esp32 internal temperature
  int8_t temperature = (int8_t)temperatureRead();

  // send empty line for separation
  Serial.println();

  // send status of wifi connection to serial monitor
  if( WiFi.isConnected() ) {
    Serial.print("ESP32-C3 (hostname ");
    Serial.print(WiFi.getHostname());
    Serial.print(") connected to ");
    Serial.print(WiFi.SSID());
    Serial.print(" with ip ");
    Serial.print(WiFi.localIP());
    Serial.print(" (rssi: ");
    Serial.print(WiFi.RSSI());
    Serial.println(")");
  } else {
    Serial.println("ESP32-C3 NOT connected");
  }

  // send temperature to serial monitor
  Serial.print("ESP32-C3 temperature: ");
  Serial.print(temperature);
  Serial.println("°C");

  // reconnect mqtt if needed and publish temperature
  if(!mqttClient.connected()){
    mqtt_reconnect();
  }
  mqtt_publish(temperature);
  mqttClient.loop();

  // wait loop interval
  delay(LOOP_INTERVAL);
}



// =====================================================================================================================
// function: gpio_setup()
// =====================================================================================================================
void gpio_setup(){

  // setup boot button pin as input using internal pullup
  pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

  // setup rgb led pins and set initially high to turn led off
  pinMode(PIN_RGB_LED_RED, OUTPUT);
  digitalWrite(PIN_RGB_LED_RED, HIGH);

  pinMode(PIN_RGB_LED_GREEN, OUTPUT);
  digitalWrite(PIN_RGB_LED_GREEN, HIGH);

  pinMode(PIN_RGB_LED_BLUE, OUTPUT);
  digitalWrite(PIN_RGB_LED_BLUE, HIGH);
}



// =====================================================================================================================
// function: wifi_setup()
// =====================================================================================================================
void wifi_setup(){

  // set station mode - do not act as access point
  WiFi.mode(WIFI_STA);

  // create instance of wifi manager
  WiFiManager wm;

  // reset wifi manager configuration if boot button is pressed on startup
  // to allow this to happen within the right time a bootup delay was added
  // in setup()
  if(LOW == digitalRead(PIN_BOOT_BUTTON)) {
    Serial.println("Resetting wifi manager configuration");
    wm.resetSettings();
  }

  // connect to stored network or launch wifi manager
  if(! wm.autoConnect("ESP32-C3 AP", "esp32-password") ){
    Serial.println("Failed to connect to wifi");
  } else {
    Serial.println("Successfully connected to wifi");
  }
}



// =====================================================================================================================
// function: mqtt_setup()
// =====================================================================================================================
void mqtt_setup(){

  mqttClient.setServer(MQTT_SERVER_ADDRESS, MQTT_SERVER_PORT);
}



// =====================================================================================================================
// function: mqtt_reconnect()
// =====================================================================================================================
void mqtt_reconnect(){

  do {
    if(! mqttClient.connect(WiFi.getHostname())){
      Serial.println("Failed to connect to MQTT server");
      delay(MQTT_CONNECT_INTERVAL);
    } else {
      Serial.println("Successfully connected to MQTT server");
    }
  } while(!mqttClient.connected());
}



// =====================================================================================================================
// function: mqtt_publish()
// =====================================================================================================================
void mqtt_publish(int8_t temperature){

  char szMqttTopic[128];
  snprintf(szMqttTopic, sizeof(szMqttTopic), "%s%s", WiFi.getHostname(), MQTT_PUB_TOPIC_TEMPERATURE);

  char szTemperature[4];
  snprintf(szTemperature, sizeof(szTemperature), "%hhi", temperature);

  if(! mqttClient.publish(szMqttTopic, szTemperature)) {
    Serial.println("Failed to publish temperature");
  } else {
    Serial.println("Successfully published temperature");
  }
}



// =====================================================================================================================
// EOF - end of file
// =====================================================================================================================
