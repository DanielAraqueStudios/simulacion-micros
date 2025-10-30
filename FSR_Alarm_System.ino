/*
 * ESP32 FSR Weight Alarm System
 * Monitors 3 FSR sensors and transmits alarm codes via Serial
 * Target weight: 20 kg cement packages
 * 
 * HARDWARE CONNECTIONS:
 * - Sensor 1 (V1): 3.3V → FSR1 → GPIO39 → 1kΩ → GND
 * - Sensor 2 (V2): 3.3V → FSR2 → GPIO33 → 1kΩ → GND  
 * - Sensor 3 (V3): 3.3V → FSR3 → GPIO4  → 1kΩ → GND
 * - Serial: TX/RX at 115200 baud
 */

// Pin definitions - ADC inputs
#define SENSOR1_PIN 39  // ADC1_IN3 (VP)
#define SENSOR2_PIN 33  // ADC1_IN5
#define SENSOR3_PIN 4   // ADC2_IN2

// Circuit constants
#define ADC_MAX 4095    // 12-bit ADC resolution
#define ADC_VREF 3.3    // ADC reference voltage

// Calibration voltage points for each sensor (15 decimal precision)
// Sensor 1 voltage calibration: [W15kg, W18kg, W20kg, W22kg, W25kg]
const float S1_VOLTAGES[5] = {3.298270982997247, 3.298511517240511, 3.298635725901316, 3.298738853845236, 3.298866222065517};

// Sensor 2 voltage calibration: [W15kg, W18kg, W20kg, W22kg, W25kg]
const float S2_VOLTAGES[5] = {3.298044181581830, 3.298307319172045, 3.298441866933401, 3.298554961161512, 3.298694274698244};

// Sensor 3 voltage calibration: [W15kg, W18kg, W20kg, W22kg, W25kg]
const float S3_VOLTAGES[5] = {3.298618110841288, 3.298826722894415, 3.298933629749474, 3.299020928797040, 3.299128157988496};

// Corresponding weight values
const float WEIGHT_POINTS[5] = {15.0, 18.0, 20.0, 22.0, 25.0};

// Timing
#define READ_INTERVAL 500  // Read sensors every 500ms

// Global variables
float weight1, weight2, weight3;
float avgWeight;
unsigned long lastReadTime = 0;

void setup() {
  // Initialize Serial at 115200 baud
  Serial.begin(115200);
  
  // Configure ADC pins as input
  pinMode(SENSOR1_PIN, INPUT);
  pinMode(SENSOR2_PIN, INPUT);
  pinMode(SENSOR3_PIN, INPUT);
  
  // ADC configuration for better accuracy
  analogReadResolution(12);  // 12-bit resolution (0-4095)
  analogSetAttenuation(ADC_11db);  // Full 3.3V range
  
  delay(100);
  Serial.println("FSR Alarm System Started");
}

void loop() {
  // Read sensors at defined interval
  if (millis() - lastReadTime >= READ_INTERVAL) {
    lastReadTime = millis();
    
    // Read and calculate weight from each sensor
    weight1 = voltageToWeight(SENSOR1_PIN, S1_VOLTAGES);
    weight2 = voltageToWeight(SENSOR2_PIN, S2_VOLTAGES);
    weight3 = voltageToWeight(SENSOR3_PIN, S3_VOLTAGES);
    
    // Calculate average weight
    avgWeight = (weight1 + weight2 + weight3) / 3.0;
    
    // Evaluate all alarm conditions and transmit codes
    checkAlarms();
  }
}

// Convert ADC reading to weight using calibration voltage lookup
float voltageToWeight(int pin, const float* voltageCalibration) {
  // Read ADC value (average of 10 readings for stability)
  int adcSum = 0;
  for (int i = 0; i < 10; i++) {
    adcSum += analogRead(pin);
    delayMicroseconds(100);
  }
  int adcValue = adcSum / 10;
  
  // Convert ADC to voltage with high precision
  float voltage = (adcValue / (float)ADC_MAX) * ADC_VREF;
  
  // Find weight using linear interpolation between calibration points
  float weight = 0.0;
  
  // Check if voltage is below lowest calibration point
  if (voltage <= voltageCalibration[0]) {
    weight = WEIGHT_POINTS[0];
  }
  // Check if voltage is above highest calibration point
  else if (voltage >= voltageCalibration[4]) {
    weight = WEIGHT_POINTS[4];
  }
  // Interpolate between calibration points
  else {
    for (int i = 0; i < 4; i++) {
      if (voltage >= voltageCalibration[i] && voltage <= voltageCalibration[i + 1]) {
        // Linear interpolation formula
        float v1 = voltageCalibration[i];
        float v2 = voltageCalibration[i + 1];
        float w1 = WEIGHT_POINTS[i];
        float w2 = WEIGHT_POINTS[i + 1];
        
        weight = w1 + ((voltage - v1) * (w2 - w1)) / (v2 - v1);
        break;
      }
    }
  }
  
  return weight;
}

// Check all alarm conditions and transmit applicable codes
void checkAlarms() {
  String alarmCodes = "";
  
  // Count sensors above 20 kg
  int sensorsAbove20 = 0;
  if (weight1 > 20.0) sensorsAbove20++;
  if (weight2 > 20.0) sensorsAbove20++;
  if (weight3 > 20.0) sensorsAbove20++;
  
  // Condition J: All three sensors > 20 kg
  if (sensorsAbove20 == 3) {
    alarmCodes += "J";
  }
  
  // Condition H: At least two sensors > 20 kg
  if (sensorsAbove20 >= 2) {
    alarmCodes += "H";
  }
  
  // Condition G: At least one sensor > 20 kg
  if (sensorsAbove20 >= 1) {
    alarmCodes += "G";
  }
  
  // Condition K: All sensors in range [18, 20] kg
  if (weight1 >= 18.0 && weight1 <= 20.0 &&
      weight2 >= 18.0 && weight2 <= 20.0 &&
      weight3 >= 18.0 && weight3 <= 20.0) {
    alarmCodes += "K";
  }
  
  // Condition L: Weight difference between any pair exceeds ±2 kg
  if (abs(weight1 - weight2) > 2.0 ||
      abs(weight1 - weight3) > 2.0 ||
      abs(weight2 - weight3) > 2.0) {
    alarmCodes += "L";
  }
  
  // Condition P: Average > 25 kg or < 15 kg
  if (avgWeight > 25.0 || avgWeight < 15.0) {
    alarmCodes += "P";
  }
  
  // Transmit all applicable alarm codes
  if (alarmCodes.length() > 0) {
    Serial.print(alarmCodes);
  }
  
  // Debug output (optional - comment out for production)
  Serial.print(" | W1:");
  Serial.print(weight1, 2);
  Serial.print(" W2:");
  Serial.print(weight2, 2);
  Serial.print(" W3:");
  Serial.print(weight3, 2);
  Serial.print(" AVG:");
  Serial.println(avgWeight, 2);
}
