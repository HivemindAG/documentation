#include <ESP8266WiFi.h>                                         // general library for ESP8266 module
#include <MQTTClient.h>                                          // MQTT functionality for M2M communication
#include <DHT.h>                                                 // library for DHT22 humidity and temparature sensor
#include <ArduinoJson.h>                                         // this library enables sending, receiving and parsing json objects

StaticJsonBuffer<200> jsonBuffer;                                // setting up a static json buffer

#define DHTPIN D2                                                // the pin, the DHT22 is connected to
#define LED D0                                                   // the pin, the DHT22 is connected to
#define DHTTYPE DHT22                                            // the type of the humidity and temparature sensor (DHT22)
DHT dht(DHTPIN, DHTTYPE);                                        // initialising humidity and temparature sensor

const char* ssid     = "YourWifiName";                           // the wifi name (SSID)
const char* password = "YourWifiPassword";                       // the wifi password
char brokerAddress[] = "mqtt.hivemind.ch";                       // MQTT broker address
String myPublishTopic = "/up/XXXX/id/XXXX";                      // MQTT topic the sensor is listening to 
String mySubscribedTopic = "/down/XXXX/alarm";                   // MQTT topic the sensor is listening to

WiFiClient net;                                                  // creating wifi object
MQTTClient theMqttClient;                                        // creating MQTT object

unsigned long lastMillis = 0;                                    // variable used for timer

void connect() {
  Serial.print("checking wifi...");                              // serial output
  while (WiFi.status() != WL_CONNECTED) {                        // try to connect to wifi until success
    Serial.print(".");                                           // serial output
    delay(1000);                                                 // wait for next atempt
  }

  Serial.print("\nconnecting...");                               // serial output
  while (!theMqttClient.connect("1122")) {                       // try to connect to MQTT brokerœ
    Serial.print(".");                                           // serial output
    delay(1000);                                                 // wait for next atempt
  }

  Serial.println("\nconnected!");                                // serial output
  theMqttClient.subscribe(mySubscribedTopic);                    // subscribe to MQTT topic
}

void setup() {
  pinMode(LED, OUTPUT);          
  Serial.begin(115200);                                          // establishing serial connection for debugging
  Serial.println("Startup");                                     // serial output
  WiFi.begin(ssid, password);                                    // starting wifi connection
  theMqttClient.begin(brokerAddress, 8883, net);                      // setting up MQTT client to MQTT broker address and wifi
  connect();                                                     // connecting to MQTT broker
}

void loop() {
  theMqttClient.loop();                                          // loop of MQTT client – actually listening

  if (!theMqttClient.connected()) {                              // if MQTT connection lost
    connect();                                                   // reconnect
  }

  if (millis() - lastMillis > 2000) {                            // timer for reading sensor data
    lastMillis = millis();                                       // reset timer
    sendSensorData();                                            // method call
  }

}

void sendSensorData() {
  float theTemparature = dht.readTemperature();                  // read and store temparature
  float theHumidity = dht.readHumidity();                        // read and store humidity

  if (isnan(theHumidity) || isnan(theTemparature)) {             // if data is invalid
    Serial.println("Failed to read from DHT sensor!");           // serial output
    return;
  }

  jsonBuffer = StaticJsonBuffer<200>();                          // clear (kind of) the json buffer
  JsonObject& root = jsonBuffer.createObject();                  // create json object

  root["temperature"] = theTemparature;                          // adding name and value to json
  root["humidity"] = theHumidity;                                // adding name and value to json

  String strObj;                                                 // creating string object
  root.printTo(strObj);                                          // parsing the json to the string object

  theMqttClient.publish(myPublishTopic, strObj);                 // publish (send) JSON data on MQTT
  Serial.println(strObj);                                        // serial output
}

void messageReceived(String topic, String payload, char * bytes, unsigned int length) {

  Serial.println(payload);
  
  StaticJsonBuffer<50> jsonBuffer;
  JsonObject& rx = jsonBuffer.parseObject(payload);
  
  unsigned int ledValue = rx["alarm"];
  
  digitalWrite(LED, ledValue);
  
  delay(10);
}


