#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Arduino.h>

#include <esp_now.h>
#include <WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3D

#define MIN_TRIGGER_TIME 15

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

typedef struct struct_message {
  char m;
} struct_message;

struct_message startMessage;

int sw_mode_pin = 15; // High - Setup, Low - Race
int sw_reset_pin = 32;
int light_gate_pin = 34;

int led_pin = 19;

int i2c_sda_pin = 21;
int i2c_scl_pin = 22;

int state;
bool running = false;
/*
1 = ready
2 = running
3 = display
*/

float times[20];
int times_i = 0;
int start_time = 0;
int falling_time = 0;
int rising_time = 0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&startMessage, incomingData, sizeof(startMessage));
  if(state == 1 && startMessage.m == 'S'){
    start_time = millis();

    for(int i = 0; i < 8; i++){
      times[i] = 0.0;
    }

    times_i = 0;
    rising_time = 0;
    falling_time = 0;
    state = 2;
  }
}

void gate_change(){
  if(state == 2){
    if(digitalRead(light_gate_pin)){
      rising_time = millis();

      Serial.println("Rising");
      Serial.println(rising_time);
    }else{
      falling_time = millis();

      Serial.println("Falling");
      Serial.println(falling_time);
    }
  }
}

void display_ready(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("READY");

  display.display();
}

void display_error(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("ERROR");

  display.display();
}

void display_running(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("MEASURING");
  display.setTextSize(1);
  display.println("---------------------");
  display.display();
}

void display_time(){
  float delta_t = (float)(millis() - start_time) / 1000;
  
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);

  display.println("MEASURING");
  display.setTextSize(1);
  display.println("---------------------");
  display.setTextSize(2);
  display.println(String(delta_t, 3));

  display.display();
}

void display_setup(){
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);
  display.println("SETUP");
  display.setTextSize(1);
  display.println("---------------------");
  display.println("The LED shows whether");
  display.println("the laser gate is");
  display.println("alligned.");
  display.display();
}

void display_times(){
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0);
  display.cp437(true);

  Serial.println(times_i);

  display.println("TIMES");

  for(int i = 0; i < times_i; i++){
    Serial.println(String(i+1) + ".  " + String(times[i], 3));
    display.println(String(i+1) + ".  " + String(times[i], 3));
  }

  display.display();
}

void setup() {
  Serial.begin(9600);

  pinMode(sw_reset_pin, INPUT);
  pinMode(light_gate_pin, INPUT);
  attachInterrupt(light_gate_pin, gate_change, CHANGE);

  Wire.begin(i2c_sda_pin, i2c_scl_pin);
  //if(!display.begin(SSD1306_EXTERNALVCC, SCREEN_ADDRESS)) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display_ready();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }else{
    if(esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv)) != ESP_OK){
      Serial.println("Error registering ESP-NOW");
    }
  }
 
  state = 1;
  Serial.println("Setup done!...");
}

void loop() {
  if(state == 1){ //ready
    display_ready();

    if(digitalRead(sw_reset_pin) == HIGH){
      start_time = millis();
      
      for(int i = 0; i < 8; i++){
        times[i] = 0.0;
      }

      times_i = 0;
      rising_time = 0;
      falling_time = 0;
      state = 2;
      Serial.println("START");
    }
  }else if(state == 2){ //running

    if(falling_time != 0 && rising_time != 0){

      Serial.println("Cross detected!");
      Serial.println(falling_time - rising_time);
      if(falling_time - rising_time  > MIN_TRIGGER_TIME){
        Serial.println("Valid!");
        times[times_i++] = (float)(rising_time - start_time) / 1000;
        rising_time = 0;
        falling_time = 0;
      }else{
        Serial.println("Invalid!");
        rising_time = 0;
        falling_time = 0;
      }
    }else{
      display_time();
    }

    if(digitalRead(sw_reset_pin) == HIGH && millis() - start_time > 500){
      state = 3;
      delay(500);
    }
  }else if(state == 3){ //display
    display_times();
    
    if(digitalRead(sw_reset_pin) == HIGH){
      state = 1;
      delay(500);
    }
  }
}

