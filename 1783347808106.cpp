// Auto Lighting System Firmware - Edge AI Simulation
// Pin Configurations
const int PIR_PIN = 19;  // Digital input for PIR sensor
const int LDR_PIN = 4;   // Analog input for Photoresistor (LDR)[cite: 2]
const int PWM_PIN = 18;  // PWM output to control LED brightness[cite: 2]

// PWM properties
const int pwmChannel = 0;
const int freq = 5000;
const int resolution = 8; // 8-bit resolution (0-255 values)

// Moving Average Filter Parameters
const int numReadings = 10;
int readings[numReadings];      
int readIndex = 0;              
long total = 0;                  
int averageLDR = 0;              

void setup() {
  Serial.begin(115200);
  
  pinMode(PIR_PIN, INPUT);
  
  // Configure LED PWM functionalitites
  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(PWM_PIN, pwmChannel);
  
  // Initialize all moving average filter readings to 0
  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
  
  Serial.println("--- Auto Lighting System Initialized ---");
}

void loop() {
  // Read occupancy status
  int motionDetected = digitalRead(PIR_PIN);
  
  // Apply Moving Average Smoothing Filter to LDR readings to prevent flickering
  total = total - readings[readIndex];
  readings[readIndex] = analogRead(LDR_PIN);
  total = total + readings[readIndex];
  readIndex = readIndex + 1;

  if (readIndex >= numReadings) {
    readIndex = 0;
  }

  averageLDR = total / numReadings;

  int targetPWM = 0;
  float illuminationPercentage = 0.0;

  if (motionDetected == HIGH) {
    // Space is occupied: Map LDR analog range (0-4095) inversely to PWM output (255-0)
    // Darker environment -> Higher duty cycle intensity
    targetPWM = map(averageLDR, 0, 4095, 255, 0);
    
    // Constrain within 8-bit bounds
    targetPWM = constrain(targetPWM, 3, 255); 
    illuminationPercentage = (targetPWM / 255.0) * 100.0;
  } else {
    // Space is completely empty: Force override to 0% duty cycle to maximize savings[cite: 2]
    targetPWM = 0;
    illuminationPercentage = 0.0;
  }

  // Execute the PWM dimming action[cite: 2]
  ledcWrite(pwmChannel, targetPWM);

  // Send system telemetry logs directly to the Serial Dashboard
  Serial.print("Occupancy: ");
  Serial.print(motionDetected == HIGH ? "OCCUPIED " : "VACANT   ");
  Serial.print(" | Raw Ambient Light (Lux proxy): ");
  Serial.print(averageLDR);
  Serial.print(" | Output Illumination Power: ");
  Serial.print(illuminationPercentage, 1);
  Serial.println("%");

  delay(100); // 10Hz sampling intervals
}