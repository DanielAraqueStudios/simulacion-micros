# ESP32 FSR Weight Alarm System

Embedded system for monitoring cement package weights using three Force Sensing Resistor (FSR) sensors with real-time alarm transmission.

---

## 📋 Project Overview

This system measures the weight of cement packages using three calibrated FSR sensors and transmits alarm codes via serial communication based on weight conditions. The target weight for packages is **20 kg**.

### Key Features
- ✅ Triple FSR sensor measurement with individual calibration
- ✅ Real-time weight calculation using characteristic equations
- ✅ Continuous alarm monitoring with multiple condition checks
- ✅ Serial communication at 115,200 baud/s
- ✅ Averaged readings for measurement stability

---

## 🔌 Hardware Components

| Component | Specification | Quantity |
|-----------|--------------|----------|
| ESP32 Dev Board | 3.3V logic | 1 |
| FSR Sensors | 2 kΩ nominal resistance | 3 |
| Resistors | 1 kΩ (series conditioning) | 3 |
| Power Supply | 3.3V | 1 |

---

## 🔧 Circuit Connections

### Voltage Divider Configuration

Each sensor uses a simple voltage divider circuit:

```
3.3V ──┬── FSR_SENSOR ──┬── Vx (to ADC) ──┬── 1kΩ ──┬── GND
       │                │                  │        │
     Power          Sensor Output      Measurement  Series R
```

### Pin Mapping

| Sensor | FSR Output | ADC Channel | ESP32 GPIO | Series Resistor |
|--------|------------|-------------|------------|-----------------|
| Sensor 1 | V1 | ADC1_IN3 | GPIO 39 (VP) | 1 kΩ to GND |
| Sensor 2 | V2 | ADC1_IN5 | GPIO 33 | 1 kΩ to GND |
| Sensor 3 | V3 | ADC2_IN2 | GPIO 4 | 1 kΩ to GND |

### Wiring Diagram

```
ESP32                    FSR SENSORS
                         
GPIO39 ──────────┬─── Sensor 1 ─── 3.3V
                 └─── 1kΩ ─── GND

GPIO33 ──────────┬─── Sensor 2 ─── 3.3V
                 └─── 1kΩ ─── GND

GPIO4  ──────────┬─── Sensor 3 ─── 3.3V
                 └─── 1kΩ ─── GND

TX/RX ────────── Serial Monitor (115200 baud)
```

---

## 📐 Sensor Calibration

### Characteristic Equations

Each sensor has a unique power-law relationship between resistance (R_out in kΩ) and weight (W in kg):

| Sensor | Equation | K Value | Exponent |
|--------|----------|---------|----------|
| Sensor 1 | R_out = 4.91 × W₁⁻⁰·⁸²⁶ | 4.91 | -0.826 |
| Sensor 2 | R_out = 5.05 × W₂⁻⁰·⁷⁹¹ | 5.05 | -0.791 |
| Sensor 3 | R_out = 4.82 × W₃⁻⁰·⁹⁰² | 4.82 | -0.902 |

### Resistance Calculation Table

Calculated FSR resistance (R_out in kΩ) for different weights:

| Sensor | W=15 kg | W=18 kg | W=20 kg | W=22 kg | W=25 kg |
|--------|---------|---------|---------|---------|---------|
| **Sensor 1** | 0.473 | 0.405 | 0.375 | 0.350 | 0.317 |
| **Sensor 2** | 0.514 | 0.442 | 0.410 | 0.383 | 0.348 |
| **Sensor 3** | 0.394 | 0.329 | 0.301 | 0.278 | 0.248 |

**Formula:** `R_out = K × W^(-exponent)`

---

### Voltage Calculation Table

Measured voltage (Vx in Volts) at ADC pins for different weights:

| Sensor | W=15 kg | W=18 kg | W=20 kg | W=22 kg | W=25 kg |
|--------|---------|---------|---------|---------|---------|
| **Sensor 1** | 2.239 | 2.342 | 2.396 | 2.440 | 2.500 |
| **Sensor 2** | 2.186 | 2.301 | 2.360 | 2.409 | 2.473 |
| **Sensor 3** | 2.328 | 2.447 | 2.507 | 2.556 | 2.620 |

**Formula:** `Vx = Vcc × Rs / (Rs + R_out)` where Rs = 1 kΩ, Vcc = 3.3V

---

## 🚨 Alarm System Logic

The system evaluates **all conditions simultaneously** and transmits **all applicable alarm codes** continuously.

### Alarm Code Table

| Code | Condition Description | Priority |
|------|----------------------|----------|
| **G** | At least ONE sensor detects > 20 kg | Low |
| **H** | At least TWO sensors detect > 20 kg | Medium |
| **J** | ALL THREE sensors detect > 20 kg | High |
| **K** | All sensors within 18-20 kg range (optimal) | Normal |
| **L** | Weight difference between any pair > ±2 kg | Imbalance |
| **P** | Average weight > 25 kg OR < 15 kg | Critical |

### Example Outputs

```
K | W1:19.20 W2:18.95 W3:19.50 AVG:19.22
   ↑ All sensors in optimal range

GHL | W1:21.50 W2:18.20 W3:19.00 AVG:19.57
 ↑↑↑ One sensor > 20kg, Two sensors condition, Imbalanced

JHP | W1:26.10 W2:25.80 W3:27.20 AVG:26.37
 ↑↑↑ All > 20kg, Two+ > 20kg, Average critical
```

---

## 💻 Software Implementation

### Algorithm Flow

```
┌─────────────────────────────────────┐
│  Read ADC values from 3 sensors     │
│  (averaged over 10 samples)         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Convert ADC to Voltage (Vx)        │
│  Vx = (ADC / 4095) × 3.3V          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Calculate FSR Resistance           │
│  R_out = Rs × (Vcc/Vx - 1)         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Calculate Weight from Equation     │
│  W = (R_out / K)^(-1/exponent)     │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Evaluate ALL alarm conditions      │
│  - Count sensors > 20 kg            │
│  - Check range [18-20] kg           │
│  - Check differences > 2 kg         │
│  - Check average vs thresholds      │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Transmit all applicable codes      │
│  via Serial (115200 baud)           │
└─────────────────────────────────────┘
```

### Key Functions

1. **`readWeight(pin, K, exponent)`**
   - Reads ADC value with 10-sample averaging
   - Calculates FSR resistance from voltage divider
   - Applies sensor-specific calibration equation
   - Returns weight in kilograms

2. **`checkAlarms()`**
   - Evaluates all 6 alarm conditions
   - Builds string with applicable alarm codes
   - Transmits codes via Serial
   - Outputs debug information (weights and average)

---

## 🚀 Installation & Usage

### Requirements
- Arduino IDE 1.8.x or 2.x
- ESP32 Board Support Package
- USB cable for programming

### Upload Steps

1. **Install ESP32 Board Support**
   - Arduino IDE → Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Tools → Board → Boards Manager → Search "ESP32" → Install

2. **Configure Board Settings**
   - Board: "ESP32 Dev Module"
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Port: Select your COM port

3. **Upload Code**
   - Open `FSR_Alarm_System.ino`
   - Click Upload button
   - Wait for "Done uploading" message

4. **Monitor Output**
   - Tools → Serial Monitor
   - Set baud rate to **115200**
   - Observe alarm codes and weight readings

---

## 📊 Testing & Calibration

### Verification Checklist

- [ ] All three sensors connected correctly
- [ ] 1 kΩ resistors installed for each sensor
- [ ] 3.3V power supply stable
- [ ] Serial monitor displays readings at 115200 baud
- [ ] Weight values reasonable (0-100 kg range)
- [ ] Alarm codes trigger correctly for test conditions

### Calibration Notes

If readings seem inaccurate:
1. Verify resistor values (should be exactly 1.0 kΩ)
2. Check power supply voltage (should be 3.3V)
3. Ensure FSR sensors are properly seated
4. Adjust calibration constants (K1, EXP1, etc.) if needed
5. Use known weights to verify accuracy

---

## 🔍 Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| No Serial output | Wrong COM port | Check Tools → Port |
| Weight always 0 | Sensor not connected | Verify wiring |
| Erratic readings | Loose connections | Check breadboard contacts |
| Voltage > 3.3V | Wrong ADC attenuation | Code uses ADC_11db (correct) |
| Compilation error | Missing ESP32 board | Install ESP32 board package |

---

## 📁 Project Structure

```
simulacion-micros/
├── FSR_Alarm_System.ino    # Main Arduino sketch
└── README.md               # This documentation
```

---

## 📝 Technical Specifications

| Parameter | Value |
|-----------|-------|
| Microcontroller | ESP32 (Xtensa dual-core) |
| Operating Voltage | 3.3V |
| ADC Resolution | 12-bit (0-4095) |
| ADC Reference | 3.3V |
| Serial Baud Rate | 115,200 baud/s |
| Sampling Rate | 500 ms (2 Hz) |
| Samples per Reading | 10 (averaged) |
| Weight Range | 0-100 kg |
| Target Weight | 20 kg |

---

## 👨‍💻 Author

**Daniel Araque**  
Universidad Militar Nueva Granada  
Mechatronics Engineering - 6th Semester  
Microcontrollers Course

---

## 📅 Project Information

- **Course:** Microcontrollers (MICROS)
- **Semester:** Sixth (6th)
- **Date:** October 2025
- **Application:** Industrial weight monitoring for cement packages

---

## 📜 License

Educational project for academic purposes.

---

## 🔗 References

- ESP32 Arduino Core Documentation
- Force Sensing Resistor Application Notes
- Voltage Divider Circuit Theory
- Power-Law Calibration Methods

---

**Status:** ✅ Functional and tested  
**Version:** 1.0  
**Last Updated:** October 30, 2025
