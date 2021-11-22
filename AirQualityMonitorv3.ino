
#include "20font.h"
#include "icons.h"

// The font names are arrays references, thus must NOT be in quotes ""
#define AA_FONT_SMALL Bold20

#include <SPI.h>
#include <Wire.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <DHT.h>
#include "Adafruit_CCS811.h"
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define DHTPIN 4     // what pin DHT22 is connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor
Adafruit_CCS811 ccs; //co2&voc sensor

//base
float hum[10];  //Stores humidity value
float temp[10]; //Stores temperature value
int eCO2[10];
int TVOC[10];
unsigned int bkcolor = 0xF800; //initial background
unsigned int frcolor = 0xFFFF; //text and icons
unsigned int tempcolor, humcolor, cocolor, voccolor;

//variables for color calculations
float tempave, humave, coave, vocave;
float sum = 0;
int loopcount = 0;
const int totalloops = 10;
byte yellow = 0, magenta = 0, blue = 0;
unsigned int colour = yellow << 11;

//constants for color changes
const int templow = 16, tempok = 21, temphot = 27, tempcray = 32;
const int humlow = 25, humgoodlow = 40, humgoodhigh = 60, humhigh = 80;
const int co2mid = 1500, co2high = 3000;
const int vocmid = 1000, vochigh = 2000;

//wifi constants
const char *SSID = "Sindar";
const char *PWD = "melin369";

//mqtt constants
const char* mqttServer = "192.168.1.105";
WiFiClient espClient;
PubSubClient client(espClient);

TFT_eSPI tft = TFT_eSPI();

bool screenon = true;

void setup(void) {
  tempcolor = humcolor = cocolor = voccolor = bkcolor;

  //all the network stuff
  Serial.begin(9600);  //for testing values
  initWiFi(); //connect to WiFi
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  
  //all the physical stuff
  tft.begin();
  tft.invertDisplay(0);
  tft.fillScreen(bkcolor);
  tft.setRotation(3); //screen rotation
  dht.begin();
  if(!ccs.begin()){
    while(1);
  }
  // Wait for the sensor to be ready
  while(!ccs.available()); 
  
  DrawBase(); //draws base
  tft.loadFont(AA_FONT_SMALL); //loads the custom font
  pinMode(27, INPUT_PULLUP); //prepare the button for turning the screen off
}


void loop() {
  if(loopcount == totalloops) { 
    loopcount=0; //reset loopcount

    //calculate temperature color
    //my screen somehow has ranges of 0-31 on blue and yellow and 0-63 on magenta...
    sum = 0;
    for (int i = 0; i < totalloops; i++) { sum += temp[i]; }
    tempave = sum / totalloops;
    if (tempave < templow){
      magenta = 0; //magenta
      blue = 31; //light blue
      yellow = 0;
    }
    else if (tempave < tempok){
      magenta = 0; //magenta
      blue = 31; //light blue
      yellow = map(tempave, templow, tempok, 0, 31); //lowest temperature 15C when color will become blue, closer to 21, it will be more green
    }
    else if (tempave < temphot){
      yellow = 31; //yellow
      magenta = 0; //magenta
      blue = map(tempave, tempok, temphot, 31, 0); //21C-27C when color will become yellow
    }
    else if (tempave < tempcray){
      yellow = 31; //yellow
      magenta = map(tempave, temphot, tempcray, 0, 63); //27C-32C when color will become red
      blue = 0;
    }
    else { //else just red
      yellow = 31; 
      magenta = 63; 
      blue = 0; 
    }
    colour = yellow << 11 | magenta << 5 | blue; //assign the actual color
    tempcolor = colour;
    //Serial.println("Temperature colors, yellow, magenta, blue");
    //Serial.println(yellow);
    //Serial.println(magenta);
    //Serial.println(blue);
    
    //calculate humidity color
    sum = 0;
    for (int i = 0; i < totalloops; i++) {sum += hum[i];}
    humave = sum / totalloops;
    if (humave < humlow){
      yellow = 31; //yellow
      magenta = map(humave, 0, humlow, 63, 0); //get more red if lower number
      blue = 0;
    }
    else if (humave < humgoodlow){
      yellow = 31; //yellow
      magenta = 0;
      blue = map(humave, humlow, humgoodlow, 0, 31); //get more green if closer to 50
    }
    else if (humave < humgoodhigh){ //40 to 60 is ideal humidity so green
      yellow = 31;
      magenta = 0;
      blue = 31; 
    }
    else if (humave < humhigh){
      yellow = map(humave, humgoodhigh, humhigh, 31, 0);
      magenta = 0;
      blue = 31; //get more blue if closer to 80
    }
    else {
      yellow = 0;
      magenta = map(humave, humhigh, 100, 0, 63);
      blue = 31; //get more dark blue if closer to 100
    }
    colour = yellow << 11 | magenta << 5 | blue; //assign the actual color
    humcolor = colour;
    //Serial.println("Humidity colors, yellow, magenta, blue");
    //Serial.println(yellow);
    //Serial.println(magenta);
    //Serial.println(blue);

    //calculate voc color
    sum = 0;
    for (int i = 0; i < totalloops; i++) {sum += TVOC[i];}
    vocave = sum / totalloops;
    if (vocave < vocmid){
      yellow = 31; 
      magenta = 0;
      blue = map(vocave, 0, vocmid, 31, 0); //get more green if closer to 0
    }
    else if (vocave < vochigh){
      yellow = 31;
      magenta = map(vocave, vocmid, vochigh, 0, 63); //get more red if closer to 2000
      blue = 0;
    }
    else { //if > vochigh then just red
      yellow = 31;
      magenta = 63;
      blue = 0; 
    }
    colour = yellow << 11 | magenta << 5 | blue; //assign the actual color
    voccolor = colour;
    //Serial.println("VOC colors, yellow, magenta, blue");
    //Serial.println(yellow);
    //Serial.println(magenta);
    //Serial.println(blue);
    
    //calculate co2 color
    sum = 0;
    for (int i = 0; i < totalloops; i++) {sum += eCO2[i];}
    coave = sum / totalloops;
    if (coave < co2mid){
      yellow = 31; //get more green if closer to 400
      magenta = 0;
      blue = map(coave, 400, co2mid, 31, 0); 
    }
    else if (coave < co2high){
      yellow = 31; 
      magenta = map(coave, co2mid, co2high, 0, 63); //get more red if closer to 2400
      blue = 0;
    }
    else { //if > co2high then just red
      yellow = 31; 
      magenta = 63;
      blue = 0; 
    }    
    colour = yellow << 11 | magenta << 5 | blue; //assign the actual color
    cocolor = colour;
    //Serial.println("CO2 colors, yellow, magenta, blue");
    //Serial.println(yellow);
    //Serial.println(magenta);
    //Serial.println(blue);
    
    //draw all the colors
    tft.fillRect (0, 0, 80, 40, tempcolor);
    tft.fillRect (81, 0, 79, 40, humcolor);
    tft.fillRect (0, 41, 80, 39, cocolor);
    tft.fillRect (81, 41, 79, 39, voccolor);
    DrawBase(); //icons again
    //Serial.println(tempave);
    //Serial.println(humave);
    //Serial.println(coave);
    //Serial.println(vocave);

    //check WiFi connection and reconnect if needed
    if (WiFi.status() != WL_CONNECTED) { 
        //Serial.println("Reconnecting to WiFi...");
        WiFi.reconnect();
    }
    while (WiFi.status() != WL_CONNECTED) {
      //Serial.print('.');
      delay(1000);
    }

    //sending mqtt message
    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    initMQTT();
    JSONencoder["temp"] = tempave;
    JSONencoder["hum"] = humave;
    JSONencoder["co2"] = coave;
    JSONencoder["voc"] = vocave;
    char JSONmessageBuffer[100];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    //Serial.println("Sending message to MQTT topic..");
    //Serial.println(JSONmessageBuffer);
    if (client.publish("bedroom", JSONmessageBuffer) == true) {
      //Serial.println("Success sending message");
    } else {
      //Serial.println("Error sending message");
    }
 
    client.loop();
    //Serial.println("-------------");
  }
    
  hum[loopcount] = dht.readHumidity();
  temp[loopcount]= dht.readTemperature();
  ccs.setEnvironmentalData(temp[loopcount], hum[loopcount]); //temperature and humidity info for co2&voc compensation
  ccs.readData();
  eCO2[loopcount] = ccs.geteCO2();
  TVOC[loopcount] = ccs.getTVOC();
  tft.setTextColor(frcolor, bkcolor); // Set the font colour AND the background colour

  ShowData(); //draw data on screen  
  
  int sensorVal = digitalRead(27);
  if (sensorVal == LOW) {
    if (screenon == true){
    tft.writecommand(0x10); //turn off the screen if button is pressed
    screenon = false;
    }
    else {
    tft.writecommand(0x11); //turn on the screen
    screenon = true;
    }
  }
  
  loopcount++;
  //Serial.print(screenon);
  delay(5000);
}

void DrawBase(){ //draw the lines, icons and static text
  //tft.fillScreen(bk);
  tft.drawLine(80,0,80,80,frcolor);
  tft.drawLine(0,40,160,40,frcolor);
  tft.drawBitmap(0, 5, tempicon, 32, 32, frcolor);
  tft.drawBitmap(80, 5, humicon, 32, 32, frcolor);
  tft.drawBitmap(0, 45, co2icon, 32, 32, frcolor);
  tft.drawBitmap(80, 50, vocicon, 32, 21, frcolor);
}


void ShowData() { //displays the data on screen
  tft.fillRect (32, 0, 46, 38, tempcolor); //clears the previous value
  tft.setTextColor(frcolor, tempcolor);
  tft.setCursor(32, 10); //sets cursor to write new value
  tft.print(temp[loopcount]); // temp
  tft.fillRect (112, 0, 48, 38, humcolor);
  tft.setTextColor(frcolor, humcolor);
  tft.setCursor(112, 10);
  tft.print(hum[loopcount]); //humidity
  tft.fillRect (35, 42, 42, 38, cocolor);
  tft.setTextColor(frcolor, cocolor);
  tft.setCursor(35, 50);
  tft.print(eCO2[loopcount]); //co2
  tft.fillRect (115, 42, 45, 38, voccolor);
  tft.setTextColor(frcolor, voccolor);
  tft.setCursor(115, 50);  
  tft.print(TVOC[loopcount]); //voc
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PWD);
  //Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print('.');
    delay(1000);
  }
  //Serial.println(WiFi.localIP());
}

void initMQTT() {
  client.setServer(mqttServer, 1883);
  while (!client.connected()) {
    //Serial.println("Connecting to MQTT...");
 
    if (client.connect("ESP32Client")) {
 
      //Serial.println("connected");
 
    } else {
 
      //Serial.print("failed with state ");
      //Serial.print(client.state());
      delay(2000);
 
    }
  }
}
