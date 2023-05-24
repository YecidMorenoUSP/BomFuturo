
#include <PubSubClient.h>

#include <TFT_eSPI.h>
#include "FreeFonts.h"
#include <SPI.h>



#include <WiFi.h>
const char* ssid = "YecidAP";
const char* password = "autocad2013";

WiFiClient espClient;
//const char* mqtt_server = "10.41.67.206";
const char* mqtt_server = "192.168.43.1";
PubSubClient client(espClient);

using namespace std;

TFT_eSPI tft = TFT_eSPI(); 

#define X TFT_HEIGHT
#define Y TFT_WIDTH

void callbackMQTT(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
    }
    else if(messageTemp == "off"){
      Serial.println("off");
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
   
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi conectada.");
  Serial.println("Endereço de IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callbackMQTT);

  tft.init();
  tft.setRotation(3);
  tft.setTextDatum(MC_DATUM);

  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);

  tft.setFreeFont(FSSBO24);  
  tft.setTextSize(2);

  // tft.printf("%2d %%",7);

}

void setHumidity(int H){
  static char txt_humidity[10];

  if(H>= 00 && H< 20) tft.setTextColor(TFT_WHITE,TFT_BLACK);
  if(H>= 20 && H< 50) tft.setTextColor(TFT_GREEN,TFT_BLACK);
  if(H>= 50 && H< 80) tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  if(H>= 80 && H<=99) tft.setTextColor(TFT_RED,TFT_BLACK);

  sprintf(txt_humidity," %2d %%  ",H);
  tft.drawString(txt_humidity, X/2, Y/2);
}

int getHumidity(){
  return (analogRead(36)/4095.0f)*99;
}

int H[2] = {0,-1};
void loop() {
        
    if (!client.connected()) {
      tft.fillScreen(TFT_BLACK);
    reconnect();
  }
  client.loop();

    if((H[0] = getHumidity()) != H[1]){
      setHumidity(H[0]);

      static char txt_mqtt[10];
      sprintf(txt_mqtt,"%2d %%",H[0]);
      client.publish("esp32/temperature", txt_mqtt );
      H[1] = H[0];
    }    

    delay(100);
}
