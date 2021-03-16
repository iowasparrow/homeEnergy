#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
//this sketch is working for a 50 amp ct
//const float FACTOR = 20; //20A/1V from the CT
const float FACTOR = 30;
const float multiplier = 0.0000633;

#ifndef STASSID
#define STASSID "makersmark"
#define STAPSK  "sieshell"
#endif

WiFiClient client;
PubSubClient mqttClient(client);

const char* ssid     = STASSID;
const char* password = STAPSK;
int i = 0;


void printMeasure(String prefix, float value, String postfix)
{
  Serial.print(prefix);
  Serial.print(value, 3);
  Serial.println(postfix);
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  ads.begin();

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  delay(1500);
  //mqttClient.setServer(server, 1883);
  mqttClient.setServer("192.168.1.97", 1883);
  mqttClient.setCallback(callback);

  if (mqttClient.connect("arduino-1")) {
    // connection succeeded
    Serial.println("Connected to mqtt ");
    //boolean r= mqttClient.subscribe("madison/apc/linevoltage");
    //Serial.println("subscribe ");
    //Serial.println(r);
  }
  else {
    // connection failed
    // mqttClient.state() will provide more information
    // on why it failed.
    Serial.println("Connection failed for mqtt ");
  }
}

void reconnect_mqtt()
{
  delay(1500);
  if (mqttClient.connect("arduino-1")) {
    // connection succeeded
    Serial.println("Connected to mqtt ");
  }
}

void reconnect_wifi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("reconnecting wifi");
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  //begin old function
  float voltageL1, voltageL2, voltageGenL1, voltageGenL2 = 0;  //maybe change this datatype to int16_t instead of float
  float currentL1, currentL2, currentGenL1, currentGenL2 = 0;
  int16_t raw_readingL1, raw_readingL2, raw_readingGenL1, raw_readingGenL2 = 0;
  float sumL1, sumL2, sumGenL1, sumGenL2 = 0;
  long time_check = millis();
  int counter = 0;

  while (millis() - time_check < 1000)
  {
    raw_readingGenL1 = ads.readADC_SingleEnded(0);
    raw_readingGenL2 = ads.readADC_SingleEnded(1);
    raw_readingL1 = ads.readADC_SingleEnded(2);
    raw_readingL2 = ads.readADC_SingleEnded(3);
    voltageL1 = raw_readingL1 * multiplier;
    voltageL2 = raw_readingL2 * multiplier;
    voltageGenL1 = raw_readingGenL1 * multiplier;
    voltageGenL2 = raw_readingGenL2 * multiplier;
    currentL1 = voltageL1 * FACTOR;
    currentL2 = voltageL2 * FACTOR;
    currentGenL1 = voltageGenL1 * FACTOR;
    currentGenL2 = voltageGenL2 * FACTOR;

    sumL1 += sq(currentL1);
    sumL2 += sq(currentL2);
    sumGenL1 += sq(currentGenL1);
    sumGenL2 += sq(currentGenL2);
    counter = counter + 1;
  }

  currentL1 = sqrt(sumL1 / counter);
  currentL2 = sqrt(sumL2 / counter);
  currentGenL1 = sqrt(sumGenL1 / counter);
  currentGenL2 = sqrt(sumGenL2 / counter);

  char resultL1[8]; //buffer for my number
  dtostrf(currentL1, 5, 3, resultL1);

  char resultL2[8]; //buffer for my number
  dtostrf(currentL2, 5, 3, resultL2);

  char resultGenL1[8]; //buffer for my number
  dtostrf(currentGenL1, 5, 3, resultGenL1);

  char resultGenL2[8]; //buffer for my number
  dtostrf(currentGenL2, 5, 3, resultGenL2);

  printMeasure("Irms: ", currentL1, "pin2");
  printMeasure("Irms2: ", currentL2, "pin3");
  printMeasure("GenL1: ", currentGenL1, "pin0");
  printMeasure("GenL2: ", currentGenL2, "pin1");



  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("not connected");
    reconnect_wifi();
  }

  if (mqttClient.state() != 0) {
    Serial.println(mqttClient.state());
    reconnect_mqtt();
  }

  Serial.println("publishing to mqtt broker");
  mqttClient.publish("madison/arduinocurrent", resultL1);
  mqttClient.publish("madison/arduinocurrent2", resultL2);
  mqttClient.publish("madison/generatorL1", resultGenL1);
  mqttClient.publish("madison/generatorL2", resultGenL2);

  //boolean rc = mqttClient.publish("test", "test message");
  //byte outmsg[]={0xff,0xfe};
  //Serial.println("publishing bytes");
  //rc = mqttClient.publish("testbyte", outmsg,2);
  delay(1000);

  //      Serial.println(WiFi.status()); // 3 is connected
  mqttClient.loop();

}  //end loop
