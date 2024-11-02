#define BLYNK_TEMPLATE_ID "YourTemplateId" // change thissssss
#define BLYNK_TEMPLATE_NAME "RestaurantGazLeakDetector"
#define BLYNK_AUTH_TOKEN "YourAuthToken" // also thissssss

#include <BlynkSimpleEsp32.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <WiFi.h>  
#include <Arduino.h>

// Wifi credentials
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define SDA and SCL pins
#define SDA 13
#define SCL 14

// Pin configurations
const int ledPin = 25;
const int buzzerPin = 26;
const int mq2Pin = 34;
const int mq9Pin = 35;
const int stepPin1 = 32;  // Motor 1 (Window)
const int dirPin1 = 27;
const int stepPin2 = 33;  // Motor 2 (Ventilator)
const int dirPin2 = 12;

// Motor objects
AccelStepper Wstepper(AccelStepper::DRIVER, stepPin1, dirPin1);
AccelStepper Vstepper(AccelStepper::DRIVER, stepPin2, dirPin2);

// State tracking
bool gasDetected = false;

void setup() {
  Serial.begin(115200);

  // initialize Wi-Fi
  connectToWiFi();

  // initialize Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Initialize I2C and LCD
  Wire.begin(SDA, SCL);
  if (!i2CAddrTest(0x27)) {
    lcd = LiquidCrystal_I2C(0x3F, 20, 4);
  }

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Hello, measuring...");

  // Set pin modes
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(mq2Pin, INPUT);
  pinMode(mq9Pin, INPUT);

  // Configure motors
  Wstepper.setMaxSpeed(1000);
  Wstepper.setAcceleration(500);
  Vstepper.setMaxSpeed(1000);
  Vstepper.setAcceleration(500);
}

void loop() {
  Blynk.run();

  // Read gas sensor values
  int mq2Value = analogRead(mq2Pin);
  int mq9Value = analogRead(mq9Pin);

  // Send values to Blynk
  // make sure to create 2 data flow in your device template on Blynk: V0 and V1
  Blynk.virtualWrite(V0, mq2Value); // Update virtual pin V0 with MQ2 value
  Blynk.virtualWrite(V1, mq9Value); // Update virtual pin V1 with MQ9 value

  // Update LCD with sensor readings
  updateRow(1, "MQ2: ", mq2Value);
  updateRow(2, "MQ9: ", mq9Value);

  // Check for gas detection
  if (mq2Value > 300 || mq9Value > 300) {
    if (!gasDetected) {
      gasDetected = true;
      handleGasDetected();
    }
  } else {
    if (gasDetected) {
      gasDetected = false;
      handleGasCleared();
    }
  }

  Wstepper.run();
  Vstepper.run();
  delay(100);
}

void connectToWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void handleGasDetected() {
  digitalWrite(ledPin, HIGH);
  digitalWrite(buzzerPin, HIGH);
  updateRow(3, "Opening window & vent", 0);
  Wstepper.moveTo(200);
  Vstepper.moveTo(200);

  
  Blynk.logEvent("notification", "Gas detected! Taking safety measures."); 
}

void handleGasCleared() {
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  updateRow(3, "Closing window & vent", 0);
  Wstepper.moveTo(0);
  Vstepper.moveTo(0);

  
  Blynk.logEvent("notification", "Gas levels are back to normal.");
}

void updateRow(int row, const char* message, int value) {
  lcd.setCursor(0, row);
  char buffer[21];
  snprintf(buffer, sizeof(buffer), "%-20s", message);
  lcd.print(buffer);

  if (value >= 0) {
    lcd.setCursor(strlen(message), row);
    lcd.print(value);
  }
}

bool i2CAddrTest(uint8_t addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}
