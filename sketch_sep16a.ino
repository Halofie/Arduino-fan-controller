// Required for Arduino functions and math
#include <Arduino.h>
#include <math.h>
// Arduino Uno: NTC 10k on A0, Fan (LED) on D13
#define NTC_PIN A0
#define FAN_PIN 13

// NTC parameters
const float SERIES_RESISTOR = 10000.0; // 10k series resistor
const float NOMINAL_RESISTANCE = 10000.0; // 10k at 25C
const float NOMINAL_TEMPERATURE = 25.0; // 25C
const float B_COEFFICIENT = 3950.0; // Typical for 10k NTC
const int ADC_MAX = 1023;
const float ADC_VCC = 5.0;

// PWM simulation
unsigned long lastPwmTime = 0;
int pwmState = LOW;
int pwmDuty = 0; // 0-100
const int pwmPeriodMs = 20; // 50Hz (20ms period)

void setup() {
  Serial.begin(9600);
  pinMode(NTC_PIN, INPUT);
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

float readTemperatureC() {
  int adc = analogRead(NTC_PIN);
  float resistance = SERIES_RESISTOR * ((float)ADC_MAX / adc - 1.0);
  float steinhart;
  steinhart = resistance / NOMINAL_RESISTANCE;     // (R/Ro)
  steinhart = log(steinhart);                      // ln(R/Ro)
  steinhart /= B_COEFFICIENT;                      // 1/B * ln(R/Ro)
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                     // Invert
  steinhart -= 273.15;                             // convert to C
  return steinhart;
}

void setFanPwm(int duty) {
  pwmDuty = constrain(duty, 0, 100);
}

void handleFanPwm() {
  // Software PWM for D13
  unsigned long now = millis();
  int onTime = pwmPeriodMs * pwmDuty / 100;
  int offTime = pwmPeriodMs - onTime;
  static unsigned long pwmTimer = 0;
  static bool isOn = false;

  if (pwmDuty == 0) {
    digitalWrite(FAN_PIN, LOW);
    isOn = false;
    pwmTimer = now;
    return;
  }
  if (pwmDuty == 100) {
    digitalWrite(FAN_PIN, HIGH);
    isOn = true;
    pwmTimer = now;
    return;
  }
  if (!isOn && (now - pwmTimer >= offTime)) {
    digitalWrite(FAN_PIN, HIGH);
    isOn = true;
    pwmTimer = now;
  } else if (isOn && (now - pwmTimer >= onTime)) {
    digitalWrite(FAN_PIN, LOW);
    isOn = false;
    pwmTimer = now;
  }
}

void loop() {
  float tempC = readTemperatureC();
  int fanPwm = 0;
  String fanState = "OFF";
  if (tempC < 25.0) {
    fanPwm = 0;
    fanState = "OFF";
  } else if (tempC < 30.0) {
    fanPwm = 50;
    fanState = "50%";
  } else {
    fanPwm = 100;
    fanState = "100%";
  }
  setFanPwm(fanPwm);
  handleFanPwm();

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    Serial.print("Temp: ");
    Serial.print(tempC, 1);
    Serial.print(" C | Fan: ");
    Serial.println(fanState);
    lastPrint = millis();
  }
}
