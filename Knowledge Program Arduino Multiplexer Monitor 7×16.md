# Dokumentasi Program Multiplexer Monitor 7×16

## Deskripsi Umum

Program ini adalah sistem monitoring untuk **7 multiplexer dengan 16 channel per mux** (total 108 channel), yang dirancang untuk operasi 24/7 dengan fitur robust dan dual output serial plus LCD display.

---

## Fitur Utama

### 1. Hardware Support

- **7 Multiplexer** dengan 16 channel each (108 total channels)
- **LCD 1602A I2C** untuk display real-time
- **Dual Serial Output**: 
  - Serial0 (USB, 115200 baud)
  - Serial2 (TX2/RX2, pin 16/17 pada Arduino Mega)
- **Watchdog Timer** untuk auto-recovery
- **Timer1 ISR** untuk scanning kontinyu (interval 1ms)

### 2. Monitoring & Detection

- **Threshold Detection**:
  - HIGH: > 150 (aktivasi)
  - LOW: < 100 (deaktivasi)
- **Stable Reading**: 3x averaging untuk menghindari false trigger
- **Hysteresis**: Membutuhkan 2x consecutive readings untuk konfirmasi
- **Baseline Calibration**: Auto-recalibration setiap 24 jam

### 3. Output Protocol

**Format Data Serial:**

- `10[PIN]:[VALUE]` → Pin aktif (contoh: `10005:234`)
- `90[PIN]` → Pin non-aktif (contoh: `90005`)
- `99` → Heartbeat (setiap 5 detik)

**LCD Display:**

- Baris 1: Pin aktif dan jumlah (`Pin:5 Act:3`)
- Baris 2: Data serial terakhir (16 karakter)

### 4. Safety Features

- **Watchdog Timer** (1 detik timeout)
- **ISR Hang Detection** (500ms timeout)
- **FIFO Overflow Protection** (max 16 entries)
- **ADC Stuck Detection**
- **CRC Error Checking**
- **Error Counters** untuk debugging

### 5. Data Management

- **FIFO Buffer** (16 slots) untuk active pins
- **Broadcast System**: Mengirim ulang pin aktif setiap 2.5 detik
- **EEPROM Storage**: Tracking last calibration time

---

## Pin Configuration

### Control Pins

```
Mux Select Pins: 42, 43, 44, 45 (4-bit address)
Mux Signal Pins: A0, A1, A2, A3, A4, A5, A6 (7 analog inputs)
LED Status Pin:  13
LCD I2C:         SDA/SCL (default I2C pins)
Serial2:         TX2 (Pin 16), RX2 (Pin 17)
```

### Pin Mapping

| Multiplexer | Signal Pin | Enable Pin | Channels |
|-------------|------------|------------|----------|
| MUX 0       | A0         | -          | 1-16     |
| MUX 1       | A1         | -          | 17-32    |
| MUX 2       | A2         | -          | 33-48    |
| MUX 3       | A3         | -          | 49-64    |
| MUX 4       | A4         | -          | 65-80    |
| MUX 5       | A5         | -          | 81-96    |
| MUX 6       | A6         | -          | 97-108   |

---

## Alur Kerja Program

### 1. Setup Phase

1. Disable watchdog timer
2. Inisialisasi LED dan blink test
3. Inisialisasi Serial0 (USB) dan Serial2 (Hardware)
4. Inisialisasi LCD I2C
5. Setup control pins untuk multiplexer
6. Kalibrasi baseline noise
7. Reset FIFO buffer
8. Setup Timer1 untuk ISR (1ms interval)
9. Load EEPROM untuk cek last calibration
10. Enable watchdog timer (1 second)

### 2. ISR Timer1 (Interrupt Service Routine)

**Berjalan setiap 1ms:**

1. Set channel pada multiplexer
2. Baca nilai analog dengan stabilisasi
3. Averaging 3 kali pembacaan
4. Deteksi transisi HIGH/LOW dengan hysteresis
5. Update FIFO jika ada perubahan status
6. Kirim event ke serial output
7. Move ke channel berikutnya (round-robin)

### 3. Main Loop

**Continuous Operations:**

- **ISR Hang Detection** (setiap 100ms): Cek timeout ISR
- **LCD Update** (setiap 500ms): Refresh display
- **Serial Command** (continuous): Proses perintah dari user
- **Heartbeat** (setiap 5s): Kirim sinyal "99"
- **Broadcast FIFO** (setiap 2.5s): Re-transmit active pins
- **Baseline Recalibration** (setiap 24 jam): Auto-calibration
- **Watchdog Reset** (continuous): Prevent system reset

---

## Serial Command Interface

### Available Commands

| Command | Fungsi |
|---------|--------|
| `d` atau `D` | Toggle debug mode |

### Debug Mode Output

Ketika debug mode aktif, akan menampilkan:

```
Debug ON
FIFO cnt [jumlah]
Err ovf/timeout/stuck/crc/recal [counters]
```

Contoh output:
```
Debug ON
FIFO cnt 3
Err ovf/timeout/stuck/crc/recal 0/0/2/0/5
```

---

## Contoh Output Serial

### Normal Operation

```
=== 7-MUX 108-CH 24/7 ROBUST ===
=== Created by: WahyuCF, RAffi, Orang Pintar ===
=== September 2025 ===
=== Dual Serial Output Active ===

Calibrating baseline...
Calibration done.
=== MONITOR ACTIVE ===

99
10005:234
10012:456
90005
99
10005:245
99
```

### LCD Display Examples

**Saat ada pin aktif:**
```
┌────────────────┐
│Pin:5 Act:3     │
│10005:234       │
└────────────────┘
```

**Saat idle:**
```
┌────────────────┐
│No Active Pin   │
│Waiting data... │
└────────────────┘
```

---

## Error Handling & Recovery

### Error Counters

Program melacak berbagai jenis error:

- **fifoOverflow**: FIFO buffer penuh (>16 entries)
- **isrTimeout**: ISR hang/timeout
- **adcStuck**: ADC membaca nilai ekstrim (0 atau 1023)
- **crcError**: CRC checksum error
- **recalDone**: Jumlah recalibration yang dilakukan

### Recovery Mechanisms

1. **Watchdog Timer**: Auto-reset jika system hang (1s timeout)
2. **ISR Timeout Detection**: Deteksi dan log ISR hang
3. **FIFO Overflow Protection**: Reject new entries jika penuh
4. **Baseline Auto-Recalibration**: Setiap 24 jam

---

## Konfigurasi & Tuning

### Threshold Values

```cpp
const int THRESHOLD_HIGH = 150;  // Nilai aktivasi
const int THRESHOLD_LOW = 100;   // Nilai deaktivasi
```

### Timing Intervals

```cpp
BROADCAST_INTERVAL = 2500;      // Broadcast FIFO (ms)
HEARTBEAT_INTERVAL = 5000;      // Heartbeat signal (ms)
LCD_UPDATE_INTERVAL = 500;      // LCD refresh (ms)
BASELINE_RECAL_HOURS = 24;      // Auto-calibration (hours)
ISR_HANG_TIMEOUT = 500;         // ISR timeout detection (ms)
```

### LCD I2C Address

Jika LCD tidak terdeteksi, coba ubah alamat:

```cpp
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Default
// atau
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Alternatif
```

---

## Keunggulan Design

✅ **Robust**: Watchdog, error recovery, ISR timeout detection  
✅ **Dual Output**: Simultaneous USB + hardware serial  
✅ **Visual Feedback**: LCD real-time display  
✅ **Auto-Calibration**: 24-hour baseline adjustment  
✅ **Stable Detection**: Multi-level filtering (averaging + hysteresis)  
✅ **Low False Triggers**: Stable count mechanism  
✅ **Scalable**: Mudah disesuaikan untuk jumlah mux berbeda  
✅ **Error Tracking**: Comprehensive error monitoring  

---

## Use Case & Aplikasi

Program ini cocok untuk:

- **Industrial Monitoring**: Sensor array untuk pabrik
- **Multi-Input Data Acquisition**: Sistem logging multi-channel
- **Quality Control Systems**: Monitoring produksi real-time
- **Building Automation**: Multi-zone monitoring
- **Laboratory Equipment**: Multi-sensor data collection
- **Security Systems**: Multi-point detection

### Kebutuhan Sistem

- Operasi 24/7 tanpa henti
- Multiple simultaneous outputs
- Visual monitoring capability
- Long-term stability
- Reliable error recovery

---

## Troubleshooting

### LCD Tidak Muncul

1. Cek koneksi I2C (SDA/SCL)
2. Coba alamat 0x3F
3. Scan I2C address dengan I2C scanner
4. Cek power supply LCD

### Serial Tidak Output

1. Pastikan baud rate 115200
2. Cek koneksi USB/TX2-RX2
3. Restart Arduino
4. Cek Serial Monitor settings

### False Triggers

1. Adjust THRESHOLD_HIGH/LOW
2. Tingkatkan stable count requirement
3. Lakukan manual calibration
4. Cek kualitas sinyal analog

### ISR Hang

1. Cek error counters dengan debug mode
2. Reduce ISR load
3. Optimize analog reading
4. Check for hardware issues

---

## Dependencies & Libraries

```cpp
#include <avr/wdt.h>              // Watchdog Timer
#include <EEPROM.h>                // EEPROM Storage
#include <Wire.h>                  // I2C Communication
#include <LiquidCrystal_I2C.h>     // LCD I2C Driver
```

### Required Hardware

- Arduino Mega 2560 (atau compatible)
- 7× CD74HC4067 Multiplexer (atau similar 16-channel mux)
- LCD 1602A dengan I2C adapter
- Power supply yang stabil
- Pull-down resistors (optional, untuk stabilitas)

---

## Credits

**Created by:**
- WahyuCF
- Raffi
- Orang Pintar

**Date:** September 2025

**Version:** 24/7 Robust Edition with LCD 1602A

---

## License & Support

Untuk support dan pertanyaan, hubungi tim developer.

Program ini dibuat untuk keperluan monitoring industrial dengan fokus pada reliability dan robustness untuk operasi jangka panjang.

---

**Last Updated:** January 2026
