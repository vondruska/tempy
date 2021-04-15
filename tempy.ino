/*
 Tempy MQTT temperature with ESP8266 and DS18b20
 Original: Felix Klement
 Steven Vondruska
*/
#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include "OneWire.h"
#include "DallasTemperature.h"
#include "secrets.h"
#include "ArduinoJson.h"

#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>


#define ONE_WIRE_BUS 13
#define BUILTIN_LED 2

ESP8266HTTPUpdateServer httpUpdater;


long lastPub = 0;

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
ESP8266WebServer httpServer(80);

const char compile_date[] = __DATE__ " " __TIME__;



String lwt_topic;

void setup(void)
{
  
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.begin(115200);

  Serial.print("Connecting to ");
  Serial.println(wifi_SSID);
  
  WiFi.begin(wifi_SSID, wifi_password);

  // Let the led blink while connecting to wifi
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(BUILTIN_LED, LOW); 
    delay(200);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(200);
  }

  Serial.println(String(ESP.getChipId(), HEX));
  
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  WiFi.setAutoReconnect(true);

  httpUpdater.setup(&httpServer, "/update", update_username, update_password );
  
  sensor.begin();
  client.setServer(mqtt_server, 1883);

  httpServer.on("/", HTTP_GET, handleRoot);

  httpServer.begin();
  
  // setup the LWT as a global so it can be used for auto discovery and after MQTT connection
  lwt_topic = String(topic_root) + "" + String(ESP.getChipId(), HEX);
  lwt_topic.concat("/lwt");

  client.setBufferSize(512);
}

void loop(void)
{ 
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  long now = millis();
  // Check if the last pub is older than 30 secs
  if (now - lastPub > 60000 || lastPub == 0) {
    
    // Get the current temperature from the DS18b20 sensor
    sensor.requestTemperatures(); 

    Serial.println("Number of sensors found: " + sensor.getDeviceCount());

    for(int i = 0; i < sensor.getDeviceCount(); i++) {
      float temp;
      DeviceAddress tempDeviceAddress;
      sensor.getAddress(tempDeviceAddress, i);

      String sensorAddress = convertAddressToString(tempDeviceAddress);
      
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      Serial.print(sensorAddress);
      Serial.println();

      String topic = String(topic_root);
      topic.concat(sensorAddress);
      
      temp = sensor.getTempCByIndex(i);
      String temp_string = String(temp);
      
      client.publish(topic.c_str(), temp_string.c_str(), true);

      Serial.print("Sensor " + i);
      Serial.println(temp_string.c_str());
	  
	  // if this the first run, publish information for home assistant
	  if(lastPub == 0) {
		 DynamicJsonDocument doc(380);
		 doc["availability_topic"] = lwt_topic;
		 doc["state_topic"] = topic;
		 doc["unique_id"] = sensorAddress;
		 doc["device_class"] = "temperature";
		 doc["force_update"] = true;
		 doc["payload_available"] = "ONLINE";
		 doc["payload_not_available"] = "OFFLINE";
     doc["name"] = sensorAddress;
     doc["unit_of_measurement"] = "°C";
		 
		 char buffer[512];
		 serializeJson(doc, buffer);
		 
		 String autoDiscoveryTopic = String("homeassistant/sensor/");
		 autoDiscoveryTopic.concat(sensorAddress);
		 autoDiscoveryTopic.concat("/config");
		  
		  client.publish(autoDiscoveryTopic.c_str(), buffer, true);
	  }
    }
    lastPub = now;
  }

  httpServer.handleClient();
}

// Function for connecting to our MQTT-Broker
void connectMQTT() {
  Serial.println("Connecting to MQTT"); 
  // Loop until we're connected
  while (!client.connected()) {
    if (client.connect((String("ESPClient-") + String(ESP.getChipId(), HEX)).c_str(), mqtt_user, mqtt_password, lwt_topic.c_str(), 0, 1, "OFFLINE")) {
      
       // we've connected, update the LWT topic
       client.publish(lwt_topic.c_str(), "ONLINE", true);
       
       // Let the builtin LED flash for 3 seconds when we're connected
       Serial.println("MQTT connected");
       digitalWrite(BUILTIN_LED, LOW); 
       delay(3000);                   
       digitalWrite(BUILTIN_LED, HIGH);
       
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying

      delay(2000);
    }
  }
}

void handleRoot() {
  String body = String("<html><head><title>Tempy Status</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><body style=\"font-family: Arial !important;\"><h1>Tempy</h1>");
  body.concat("Chip ID: " + String(ESP.getChipId(), HEX) + "<br />");
  body.concat("Number of sensors found: " + String(sensor.getDeviceCount()) + "<br /><br />");

  for(int i = 0; i < sensor.getDeviceCount(); i++) {
      float temp;
      DeviceAddress tempDeviceAddress;
      
      sensor.getAddress(tempDeviceAddress, i);

      body.concat(convertAddressToString(tempDeviceAddress) + ": " + String(sensor.getTempCByIndex(i)) + "<br />");
  }
  body.concat("<br />Compiled on: ");
  body.concat(compile_date);
  body.concat("</body></html>");
  httpServer.send(200, "text/html", body);
}

// function to print a device address
String convertAddressToString(DeviceAddress deviceAddress) {
  String addrToReturn = "";
  for (uint8_t i = 0; i < 8; i++){
    if (deviceAddress[i] < 16) addrToReturn += "0";
    addrToReturn += String(deviceAddress[i], HEX);
  }
  return addrToReturn;
}
