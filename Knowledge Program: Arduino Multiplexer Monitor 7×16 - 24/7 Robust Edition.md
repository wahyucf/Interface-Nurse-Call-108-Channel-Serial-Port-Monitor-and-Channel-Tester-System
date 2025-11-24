# Knowledge Program: Arduino Multiplexer Monitor 7×16 - 24/7 Robust Edition

## 📋 Gambaran Umum
Program ini merupakan sistem monitoring 108 channel menggunakan 7 multiplexer CD4051 yang dirancang untuk operasi 24/7 dengan fitur robust seperti watchdog timer, CRC checksum, dan automatic baseline recalibration.

## 🎯 Spesifikasi Teknis

### Konfigurasi Hardware
- **Jumlah Multiplexer**: 7 unit
- **Channel per MUX**: 16 channel
- **Total Channel**: 108 channel
- **MUX Control Pins**: 42, 43, 44, 45
- **MUX Signal Pins**: A0, A1, A2, A3, A4, A5, A6
- **LED Status**: Pin 13

### Parameter Operasi
- **Threshold High**: 150
- **Threshold Low**: 100
- **Broadcast Interval**: 2500 ms
- **Heartbeat Interval**: 5000 ms
- **Baseline Recalibration**: Setiap 24 jam
- **ISR Timeout**: 500 ms

## 🔧 Arsitektur Sistem

### Struktur Data Utama

#### ChannelStatus
```cpp
struct ChannelStatus {
  bool active;           // Status aktif/tidak
  uint8_t stableHigh;    // Counter bacaan high stabil
  uint8_t stableLow;     // Counter bacaan low stabil
  uint8_t readCount;     // Counter pembacaan
  uint16_t sumReadings;  // Sum untuk averaging
  int lastAnalogValue;   // Nilai analog terakhir
};
```

#### SafeFIFO
```cpp
struct SafeFIFO {
  int buffer[FIFO_SIZE]; // Buffer FIFO
  int count;             // Jumlah item
  int index;             // Index saat ini
  // Methods: reset(), add(), remove(), getNext()
};
```

### Sistem Monitoring

#### Timer ISR (Interrupt Service Routine)
- **Frekuensi**: 1 ms
- **Fungsi**: Scanning channel berkelanjutan
- **Proses**:
  1. Set channel multiplexer
  2. Baca nilai analog
  3. Averaging 3 sample
  4. Deteksi transisi status
  5. Update FIFO untuk event

#### Deteksi Status Channel
- **Aktif**: `avg > THRESHOLD_HIGH` selama 2 cycle
- **Non-aktif**: `avg < THRESHOLD_LOW` selama 2 cycle
- **Nilai Analog**: Disimpan di `lastAnalogValue`

## 📊 Protokol Komunikasi Serial

### Format Pesan
```
## Format Aktifasi:
10<pin>:<nilai_analog>
Contoh: 1015:245

## Format Deaktifasi:
90<pin>
Contoh: 9015

## Heartbeat:
990
```

### Fitur Keamanan
- **CRC-8 Checksum**: Polinomial 0x8C (Maxim)
- **Watchdog Timer**: 1 second timeout
- **ISR Hang Detection**: Monitor stuck ISR

## 🔄 Kalibrasi & Baseline

### Automatic Baseline Calibration
```cpp
void calibrateBaseline(bool silent = false)
```
- **Trigger**: Setiap 24 jam atau manual
- **Proses**: Rata-rata semua channel dalam keadaan idle
- **Penyimpanan**: EEPROM address 0-3

### Error Handling
```cpp
struct {
  uint32_t fifoOverflow;  // Buffer penuh
  uint32_t isrTimeout;    // ISR stuck
  uint32_t adcStuck;      // ADC nilai 0/1023
  uint32_t crcError;      // Error checksum
  uint32_t recalDone;     // Kalibrasi selesai
} errors;
```

## 🎛️ Perintah Debug

### Mode Debug
Kirim karakter 'D' atau 'd' melalui Serial untuk:
- Status FIFO count
- Error counters
- Toggle debug mode

### LED Indikator
- **1 blink**: Heartbeat normal
- **3 blink**: ISR hang detected
- **Custom**: Sesuai kebutuhan debug

## ⚙️ Konfigurasi Lanjutan

### Timing Parameters
```cpp
const unsigned long BROADCAST_INTERVAL = 2500;    // 2.5 detik
const unsigned long HEARTBEAT_INTERVAL = 5000;    // 5 detik  
const unsigned long BASELINE_RECAL_HOURS = 24;    // 24 jam
const unsigned long ISR_HANG_TIMEOUT = 500;       // 500 ms
```

### Threshold Adjustment
```cpp
#define THRESHOLD_HIGH 150  // Nilai untuk deteksi HIGH
#define THRESHOLD_LOW 100   // Nilai untuk deteksi LOW
```

## 🛠️ Troubleshooting

### Common Issues
1. **FIFO Overflow**: Periksa kecepatan processing event
2. **ISR Timeout**: Optimalkan kode dalam ISR
3. **ADC Stuck**: Periksa koneksi hardware dan referensi voltage

### Maintenance Routine
- Monitor error counters melalui debug mode
- Verifikasi baseline calibration secara berkala
- Pastikan komunikasi serial stabil

## 📈 Optimisasi Kinerja

### Pembacaan Analog Stabil
```cpp
int readAnalogStable(int pin) {
  analogRead(pin);  // Discard first reading
  delayMicroseconds(50);
  // Average 3 readings dengan delay
}
```

### Manajemen Memori
- Fixed-size array untuk menghindari heap fragmentation
- Global variables dengan size predetermined
- EEPROM usage minimized

Program ini dirancang untuk reliability tinggi dengan comprehensive error handling dan monitoring capabilities untuk aplikasi industri 24/7.
