/*
 * Author: Patrick Aung
 * Cloud-Based Smart Safety and Security System
 */

#include <DallasTemperature.h>
#include <ArduinoJson.h>
#include <WiFi101.h>
#include <OneWire.h>
#include <Servo.h>
#include <SPI.h>

#define photoResistorPin A3
#define analogSensorPin1 A1
#define analogSensorPin2 A2
#define waterSensorPin A0
#define tempSensorPin 5
#define redLedPin 10
#define buzzerPin 6
#define servoPin 9
#define potPin A5
#define ledPin 6

Servo myServo;
OneWire oneWire(tempSensorPin);
DallasTemperature sensors(&oneWire);

float Celsius;
// Sending to Cloud
int popToCloud = 0;
float tempToCloud;
int waterToCloud;
int potToCloud;
int lightToCloud;

// Response Triggers and Computations From Lambda
int popTrigger = 0;
int tempTrigger = 0;
int waterTrigger = 0;
int potTrigger = 0;
int lightTrigger = 0;
int antTrigger = 0;
int avg_pop = 0;
float avg_temp = 0.0;
int avg_water = 0;
int avg_light = 0;
int max_pop = 0;
int max_temp = 0;
int max_water = 0;
int max_light = 0;
float globalTime = 0.0;
float globalTemp = 0.0;
int bus_num = 0;
int bike_num = 0;

// Initial city status
String popStatus = "Safe";
String tempStatus = "Safe";
String waterStatus = "Safe";
String potStatus = "LOCKED";
String lightStatus = "OFF";
String turtleStatus = "We Cool";

//********************************* WiFi Connection settings ********************************
#define SSID_WORK
#if defined(SSID_HOME)
char ssid[] = "";           //  your network home SSID (name)
char pass[] = "";           // your network password (use for WPA, or use as key for WEP)
#elif defined(SSID_WORK)
char ssid[] = "";
char pass[] = "";           // Work network password (use for WPA, or use as key for WEP)
#else
#error ssid and pass undefined
#endif

//********************************* Server settings ********************************
char server[] = ""; // name address for Amazon (using DNS)
char path[] =  "";
char api_key[] = "";

unsigned long lastConnectionTime = 0;               // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 1000L;  // delay between updates, in milliseconds
int keyIndex = 0;                                   // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
// Initialize Ethernet client library with IP address & port of server you
// want to connect to (port 80 is default for HTTP):
WiFiSSLClient client;

//********************************* Print WiFi Status  ********************************
void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();          // print your WiFi shield's IP address:
  Serial.print("IP Address: ");
  Serial.println(ip);
  long rssi = WiFi.RSSI();                // print the received signal strength:
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

//********************************** httpRequest  *********************************
void httpRequest(int population, float temperature, int water, int pot, int light) {    // make a HTTP connection to the server:
  client.stop();
  if (client.connect(server, 443)) {    // if there's a successful connection:
    Serial.println("\nconnecting...");
    char req[120];
    sprintf(req, "{\"population\":%d, \"temperature\":%f, \"water\":%d, \"pot\":%d, \"light\":%d}", population, temperature, water, pot, light);
    client.print("POST ");              // url path + protocol definition
    client.print(path);
    client.println(" HTTP/1.1");
    client.print("Host: ");             // Host with API key added to request header
    client.println(server);             // assigned above
    client.print("x-api-key: ");
    client.println(api_key);            // assigned above
    client.print("Content-Length: ");   // Transmit request body
    client.print(strlen(req));
    client.print("\n\n");
    client.print(req);
    lastConnectionTime = millis();       // time connection was made:
  }
  else {
    Serial.println("connection failed");  // if you can''t make a connection:
  }
}

//******************* Parse JSON Object To Get Response ***************************
void parseJSON() {
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
  }
  else {
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.parseObject(client);
    if (!root.success()) {
      Serial.println(F("Parsing failed!"));
    }
    else {
      popTrigger = root["popTrigger"];
      tempTrigger = root["tempTrigger"];
      globalTime = root["globalTime"];
      waterTrigger = root["waterTrigger"];
      potTrigger = root["potTrigger"];
      lightTrigger = root["lightTrigger"];
      antTrigger = root["antTrigger"];
      avg_pop = root["avg_pop"];
      avg_temp = float(root["avg_temp"]);
      avg_water = root["avg_water"];
      avg_light = root["avg_light"];
      max_pop = root["max_pop"];
      max_temp = root["max_temp"];
      max_water = root["max_water"];
      max_light = root["max_light"];
      bus_num = int(root["bus_num"]);
      bike_num = int(root["bike_num"]);
    }
  }
}

//********************** Check if Server is Disconnected ***************************
void ifServerDisconnected() {
  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
    while (true);   // do nothing forevermore:
  }
}

//******************************* Buzz and Blink **********************************
void buzzAndBlink() {
  for (int i = 0; i < 4; i++) {
    tone(buzzerPin, 1000, 100);
    digitalWrite(ledPin, HIGH);
    delay(100);
    noTone(buzzerPin);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

//************************************ Blink ***************************************
void blink() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

//********************************** lightsOn **************************************
void lightsOn() {
  digitalWrite(ledPin, HIGH);
}

//********************************** lightsOff **************************************
void lightsOff() {
  digitalWrite(ledPin, LOW);
}

//********************************* Open Gate **************************************
void openGate() {
  myServo.write(0);
  delay(0);
}

//********************************* Close Gate **************************************
void closeGate() {
  myServo.write(75);
  delay(150);
}

//********************************* Get Temp **************************************
float getTemp() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}

//********************************* Get Population **************************************
int getPop() {
  if (analogRead(analogSensorPin1) > 450) {
    closeGate();
    delay(1000);
    return popToCloud += 1;
  }
  if (analogRead(analogSensorPin2) > 450 and popToCloud > 0) {
    delay(1000);
    return popToCloud -= 1;
  }
  else {
    return popToCloud;
  }
}

//********************************** printStats *********************************
void printStats(String popStatus, String tempStatus, String waterStatus, String potStatus, String lightStatus, int avg_pop, float avg_temp, int avg_water, String turtleStatus, int bus_num, int bike_num) {
  Serial.println("--------------------------------------");
  Serial.println("       City Block Safety Status       ");
  Serial.println("--------------------------------------");
  Serial.print("City Gate Status             => ");
  Serial.println(potStatus);
  Serial.print("City Light Status            => ");
  Serial.println(lightStatus);
  Serial.print("Population Status            => ");
  Serial.println(popStatus);
  Serial.print("Temperature Status           => ");
  Serial.println(tempStatus);
  Serial.print("Water Level Status           => ");
  Serial.println(waterStatus);
  Serial.println("--------------------------------------");
  Serial.println("           City Block Stats           ");
  Serial.println("--------------------------------------");
  Serial.print("Current Population           => ");
  Serial.println(popToCloud);
  Serial.print("Local Temperature            => ");
  Serial.println(avg_temp);
  Serial.print("Global Time                  => ");
  Serial.println(globalTime);
  Serial.print("Global Temp                  => ");
  Serial.println(globalTemp);
  Serial.println("--------------------------------------");
  Serial.println("         Green Turtle City Stats        ");
  Serial.println("--------------------------------------");
  Serial.print("People on Bus                => ");
  Serial.println(bus_num);
  Serial.print("People on Bike               => ");
  Serial.println(bike_num);
  Serial.println("--------------------------------------");
  Serial.println("             Average Stats            ");
  Serial.println("--------------------------------------");
  Serial.print("Average Population           => ");
  Serial.println(avg_pop);
  Serial.print("Average Temperature          => ");
  Serial.println(avg_temp);
  Serial.print("Average Water Level          => ");
  Serial.println(avg_water);
  Serial.print("Average Light Level          => ");
  Serial.println(avg_light);
  Serial.println("\n \n \n");
}

//********************************* runSmartCity() ********************************
void runSmartCity() {
  popToCloud = getPop();
  tempToCloud = getTemp();
  waterToCloud = analogRead(waterSensorPin);
  potToCloud = analogRead(potPin);
  lightToCloud = analogRead(photoResistorPin);

  if (lightTrigger == 1) lightStatus = "ON", lightsOn();
  if (lightTrigger == 0) lightStatus = "OFF", lightsOff();
  if (potTrigger == 1) openGate(), potStatus = "UNLOCKED", digitalWrite(redLedPin, LOW);
  if (analogRead(potPin) <= 500) potStatus = "LOCKED", digitalWrite(redLedPin, HIGH);
  if (popTrigger == 0) popStatus = "Safe";
  if (tempTrigger == 0) tempStatus = "Safe";
  if (waterTrigger == 0) waterStatus = "Safe";
  if (antTrigger == 0) turtleStatus = "We Cool";

  if (popTrigger == 1 or tempTrigger == 1 or waterTrigger == 1) {
    lightStatus = "BLINKING";
    buzzAndBlink();
    if (popTrigger == 1) popStatus = "Unsafe";
    if (tempTrigger == 1) tempStatus = "Unsafe";
    if (waterTrigger == 1) waterStatus = "Unsafe";
    if (antTrigger == 1) turtleStatus = "We No Cool";
  }
  printStats(popStatus, tempStatus, waterStatus, potStatus, lightStatus, avg_pop, avg_temp, avg_water, turtleStatus, bus_num, bike_num);
}

//************************************ My Setup ***********************************
void mySetup() {
  sensors.begin();
  pinMode(potPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(tempSensorPin, INPUT);
  pinMode(waterSensorPin, INPUT);
  pinMode(photoResistorPin, INPUT);
  pinMode(analogSensorPin1, INPUT);
  pinMode(analogSensorPin2, INPUT);
  sensors.begin();
  myServo.attach(servoPin);
  myServo.write(75);
  httpRequest(-1, -1. - 1, -1, -1, -1);
}

//*********************************** Main Setup **********************************
void setup() {
  // Set these pins or you get "WiFi shield not present" error on Feather MO
  WiFi.setPins(8, 7, 4, 2);
  Serial.begin(9600);
  while (!Serial) {   // wait for serial port to connect. For native USB port only
  }
  if (WiFi.status() == WL_NO_SHIELD) {  // check for the presence of the shield:
    Serial.println("ERROR: WiFi shield not present");
    while (true);     // don't continue:
  }
  while (status != WL_CONNECTED) {  // attempt to connect to Wifi network:
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    delay(2000);
  }
  Serial.println("Connected to wifi");
  printWifiStatus();
  mySetup();
}

//************************************ Main Loop **********************************
void loop() {
  runSmartCity();
  if (millis() - lastConnectionTime > postingInterval) {    //checked if 10secs has passed
    httpRequest(popToCloud, tempToCloud, waterToCloud, potToCloud, lightToCloud);
  }
  ifServerDisconnected();
  parseJSON();
}
