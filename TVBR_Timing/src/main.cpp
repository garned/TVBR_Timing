#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3D

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MIN_TRIGGER_TIME 15

int sw_mode_pin = 15; // High - Setup, Low - Race
int sw_reset_pin = 2;
int light_gate_pin = 34;

int led_pin = 19;

int i2c_sda_pin = 21;
int i2c_scl_pin = 22;

int rx_pin = 16;
int tx_pin = 17;

int state;
bool running = false;
/*
0 = setup
1 = ready/display
2 = running
*/

float times[20];
int times_i = 0;
int start_time = 0;
int falling_time = 0;
int rising_time = 0;

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

  for(int i = 0; i < times_i; i++){
    Serial.println(String(i+1) + ".  " + String(times[i], 3));
    display.println(String(i+1) + ".  " + String(times[i], 3));
  }

  display.display();
}

void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, rx_pin, tx_pin);

  pinMode(15, INPUT);
  pinMode(2, INPUT);
  pinMode(34, INPUT);

  pinMode(led_pin, OUTPUT);

  attachInterrupt(light_gate_pin, gate_change, CHANGE);

  Wire.begin(21, 22);
  //if(!display.begin(SSD1306_EXTERNALVCC, SCREEN_ADDRESS)) {
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display_ready();
}

void loop() {

  if(state != 2 && digitalRead(sw_mode_pin) == HIGH){
    state = 0;
    display_setup();
  }else if(state != 2){
    state = 1;
    //display_ready();
  }

  
  if(state == 0){ //Setup State
    if(digitalRead(light_gate_pin) == HIGH){
      digitalWrite(led_pin, LOW);
    }else{
      digitalWrite(led_pin, HIGH);
    }
  }else if(state == 1){
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
  }else if(state == 2){
    if(falling_time != 0 && rising_time != 0){

      Serial.println("Cross ditected!");
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
      state = 1;
      display_times();
      delay(1000);
    }
  }
}

