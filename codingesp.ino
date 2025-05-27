const int FLEX_PIN = 2;  // GPIO 2
const int SAMPLES = 100;  // Number of samples for calibration
const int CALIBRATION_DELAY = 5000;  // 5 seconds delay

// Variables for calibration
float minValue = 4095;  // Maximum ADC value for ESP32
float maxValue = 0;
float voltage, angle;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // ESP32 has 12-bit ADC
  
  Serial.println("Flex Sensor Calibration");
  Serial.println("1. Keep the sensor straight");
  Serial.println("2. Wait for 5 seconds");
  Serial.println("3. Bend the sensor to maximum angle");
  Serial.println("4. Wait for 5 seconds");
  
  // Calibration for straight position
  delay(CALIBRATION_DELAY);
  Serial.println("Calibrating straight position...");
  for(int i = 0; i < SAMPLES; i++) {
    float reading = analogRead(FLEX_PIN);
    minValue = min(minValue, reading);
    delay(10);
  }
  
  Serial.println("Now bend the sensor to maximum position");
  delay(CALIBRATION_DELAY);
  Serial.println("Calibrating bent position...");
  for(int i = 0; i < SAMPLES; i++) {
    float reading = analogRead(FLEX_PIN);
    maxValue = max(maxValue, reading);
    delay(10);
  }
  
  Serial.println("\nCalibration Complete!");
  Serial.println("Min Value (Straight): " + String(minValue));
  Serial.println("Max Value (Bent): " + String(maxValue));
}

void loop() {
  // Read sensor and map to angle
  float reading = analogRead(FLEX_PIN);
  angle = map(reading, minValue, maxValue, 0, 90);  // Assuming 90 degree max bend
  
  // Print values
  Serial.print("Raw: ");
  Serial.print(reading);
  Serial.print(" | Angle: ");
  Serial.println(angle);
  
  delay(100);
}