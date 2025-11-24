# Knowledge Program - Sistem AIPHONE 48 Line

## **Deskripsi Sistem**
Program Arduino untuk sistem **AIPHONE 48 Line** yang dirancang untuk operasi 24/7 dengan fitur robust monitoring dan watchdog timer.

## **Spesifikasi Teknis**

### **Konfigurasi Utama**
```cpp
const float LimitVal = 70;        // Tegangan minimum threshold
const int JmlLine = 48;           // Jumlah line yang dimonitor
const int TikTime = 10;           // Interval ticker idle
const int DispTime = 12;          // Waktu display aktif
const int MUX_SETTLE_TIME = 10;   // Stabilisasi multiplexer (ms)
const int ADC_SAMPLES = 3;        // Sample averaging untuk ADC
const unsigned long HEARTBEAT_INTERVAL = 60000;    // Heartbeat 1 menit
const unsigned long SYSTEM_CHECK_INTERVAL = 300000; // System check 5 menit
```

### **Hardware Configuration**
- **Microcontroller**: Arduino dengan 71+ pin
- **Serial2**: PIN 16 (TX), PIN 17 (RX)
- **Multiplexer**: 4x 16-channel (total 64 channel)
- **Analog Input**: A12, A13, A14, A15
- **LED Indicator**: PIN 13

## **Fungsi Utama**

### **1. Multiplexer Control**
```cpp
void setMuxChannel(int muxIndex, int channel)
```
- Mengontrol 4 multiplexer (16 channel each)
- Set pin S0-S15 untuk seleksi channel
- Settling time: 10ms untuk stabilisasi

### **2. Sensor Reading**
```cpp
int readMuxValue(int count)
```
- Membaca nilai analog dengan averaging (3 samples)
- Mapping: count 0-47 → mux 0-3, channel 0-15
- Noise reduction melalui multiple sampling

### **3. System Monitoring**
- **Watchdog Timer**: 8 detik timeout
- **Heartbeat**: Setiap 1 menit
- **Health Check**: Setiap 5 menit
- **Error Counting**: Tracking system errors

## **Protokol Komunikasi**

### **Serial Output Format**
- **Line Active**: `"10[number]: [value]"`
- **Line Inactive**: `"90[number]:"`
- **Idle State**: `"99:"`

### **Serial2 Commands**
- **Reset/Normal**: `<N>`
- **Room Calls**: `<RR[room]-[unit]>`, `<KKM-[room]>`

## **Struktur Data**

### **Sensor Arrays**
```cpp
float mySensVals[71];        // Nilai sensor (0-70)
int mySensValsStat[71];      // Status sensor (0=active, 1=inactive)
```

### **System Health**
```cpp
struct SystemHealth {
  unsigned long totalLoops;
  unsigned long errors;
  unsigned long lastReset;
  bool serialOK;
  bool serial2OK;
}
```

## **Room Mapping (KirimDisply)**
Array berisi 71 entri untuk mapping nomor kamar:
- Format: `"<RR101-01>"` (Regular Room)
- Format: `"<KKM-101>"` (Khusus KKM)
- Format: `"<CCB-NOL>"` (Central Control)

**Pattern Mapping:**
- PIN 1-48: Rooms 101-124 + KKM
- PIN 49-70: Special rooms & control units

## **Alur Kerja Program**

### **Setup Sequence**
1. Disable watchdog sebelumnya
2. Initialize serial communication
3. Setup multiplexer pins
4. Clear sensor arrays
5. Enable watchdog (8s timeout)
6. Startup LED blink sequence

### **Main Loop Sequence**
1. **Watchdog Reset**
2. **Health Monitoring**
   - System check setiap 5 menit
   - Heartbeat setiap 1 menit
3. **Line Scanning** (48 lines)
   - Baca nilai sensor dengan averaging
   - Validasi range (0-1023)
   - Bandingkan dengan LimitVal
4. **State Management**
   - Active: Kirim room number via Serial2
   - Inactive: Kirim `<N>` reset
   - Idle: Periodic `<N>` setiap TikTime

## **Error Handling & Robustness**

### **Safety Features**
- **Watchdog Timer**: Auto-reset jika system hang
- **Boundary Checking**: Validasi array index
- **Serial Health Check**: Monitor connection status
- **Error Counting**: Track system stability
- **Memory Monitoring**: Free RAM tracking

### **Recovery Mechanisms**
- Automatic Serial2 reinitialization
- Counter overflow protection (~49 days)
- Invalid value skipping
- Graceful degradation

## **Diagnostic Features**

### **Health Reporting**
```cpp
void printSystemHealth()
```
Menampilkan:
- Uptime (seconds)
- Total loops executed
- Error count
- Serial/Serial2 status
- Free RAM available

### **Visual Indicators**
- LED blink patterns:
  - 3x blink: Startup complete
  - 2x blink: Heartbeat
  - 1x blink: Loop completion

## **Optimization Features**

### **Performance**
- ADC averaging untuk noise reduction
- Multiplexer settling time
- Efficient memory usage dengan PROGMEM
- Non-blocking delays dengan millis()

### **Reliability**
- Watchdog protection
- Automatic recovery
- Comprehensive error handling
- System health monitoring

## **Usage Notes**

### **Calibration**
- `LimitVal` disesuaikan berdasarkan perangkat:
  - iPhone: 150
  - Commax: 50
  - Default: 70

### **Maintenance**
- Monitor health reports secara berkala
- Check error counts untuk early detection
- Verify free RAM untuk memory leaks

Sistem ini dirancang untuk operasi continuous 24/7 dengan minimal maintenance requirement dan comprehensive monitoring capabilities.
