# Knowledge Program: Sistem Display Nurse Call Berbasis Arduino dengan DMD

## Gambaran Umum
Program ini merupakan sistem display nurse call yang robust dan handal untuk operasi 24/7 menggunakan Dot Matrix Display (DMD) dengan berbagai fitur proteksi dan pemulihan error.

## Spesifikasi Teknis

### Hardware yang Digunakan
- **Display**: DMD (Dot Matrix Display) 2x2 panel
- **Microcontroller**: Arduino dengan watchdog timer
- **Output**: Buzzer (pin 4), LED indicator (pin 10, 23)
- **Komunikasi**: Serial 9600 bps

### Proteksi Sistem
1. **Watchdog Timer** - Reset otomatis jika sistem hang
2. **Memory Leak Prevention** - Manajemen memori yang ketat
3. **Overflow Protection** - Batasan buffer dan pengecekan panjang string
4. **Error Recovery** - Reset komunikasi dan reinitialisasi display

## Struktur Kode

### Konfigurasi Display
```cpp
#define DISPLAYS_ACROSS 2
#define DISPLAYS_DOWN 2
#define SERIAL_TIMEOUT 5000
#define MAX_MESSAGE_LENGTH 20
#define HEARTBEAT_INTERVAL 30000
```

### Variabel Global dengan Proteksi
```cpp
int id = 100;                    // ID perintah
unsigned long millisB = 0;       // Timer millis backup
unsigned long lastHeartbeat = 0; // Timer heartbeat
unsigned long lastSerialActivity = 0; // Monitor aktivitas serial
char myArray[MAX_MESSAGE_LENGTH] = ""; // Buffer pesan dengan batasan
```

## Protokol Komunikasi

### Format Pesan
```
<[Tipe][Pesan]>
```
- **Tipe**: Karakter tunggal menentukan jenis notifikasi
- **Pesan**: Konten yang akan ditampilkan (maks 15 karakter)

### Kode Perintah
| Tipe | ID | Keterangan | Pola Buzzer |
|------|----|------------|-------------|
| N | 0 | Status Connection | - |
| R | 1 | Regular Bed Call | Pendek (100ms) |
| K | 2 | KM Call | Panjang (300ms) |
| C | 3 | Code Blue | Double short beep |
| S | 4 | Staff Assistance | Double long beep |
| P | 5 | Priority Call | Pendek (100ms) |

## Fungsi Utama

### 1. Manajemen Display
```cpp
void drawText(String dispString)
```
- Membersihkan layar dan menampilkan teks
- Membatasi panjang string maksimal 15 karakter
- Reset watchdog selama operasi

### 2. Sistem Audio (Buzzer)
- `beepCB()`: Code Blue - double short beep
- `beepSA()`: Staff Assistance - double long beep  
- `beeppendek()`: Single short beep (100ms)
- `beeppanjang()`: Single long beep (300ms)

### 3. Mode Tampilan
- `DispBed()`: Notifikasi bed call regular
- `DispKM()`: Notifikasi KM call
- `DispCBlue()`: Notifikasi Code Blue dengan LED biru
- `DispSA()`: Notifikasi Staff Assistance
- `StatCon()`: Status koneksi

## Sistem Proteksi

### Watchdog Timer
```cpp
void resetWatchdog()
wdt_enable(WDTO_8S)  // Timeout 8 detik
```
- Reset periodik di setiap loop
- Reset selama operasi panjang

### Overflow Protection
- Batasan buffer: `MAX_MESSAGE_LENGTH = 20`
- Pengecekan panjang string sebelum pemrosesan
- Reset buffer setelah overflow terdeteksi

### Timeout Handling
```cpp
#define SERIAL_TIMEOUT 5000
```
- Reset state komunikasi jika timeout
- Clear buffer serial yang tertinggal

### Error Recovery
```cpp
void reinitDisplay()
void resetCommState()
```
- Reinitialisasi display jika error
- Reset state komunikasi ke kondisi awal

## Monitoring Sistem

### Heartbeat Indicator
- LED pada pin 23 berkedip setiap 30 detik
- Indikator visual bahwa sistem berjalan normal

### Serial Activity Monitor
- Track waktu sejak aktivitas serial terakhir
- Reset state jika tidak ada komunikasi dalam 5 detik

## Sequence Startup
1. Disable watchdog sementara
2. Inisialisasi hardware dan display
3. Test sequence LED dan buzzer
4. Enable watchdog timer
5. Tampilkan "Nurse" pada display
6. Kirim "Sub Start" via serial

## Best Practices yang Diimplementasi

### 1. Memory Safety
- Penggunaan `memset()` untuk clear buffer
- Batasan eksplisit pada array dan string
- Avoid String objects yang berlebihan

### 2. Robust Communication
- Timeout handling
- Buffer overflow protection
- State machine yang jelas

### 3. System Stability
- Watchdog timer protection
- Error recovery mechanisms
- Periodic system monitoring

### 4. Performance
- Penggunaan `millis()` untuk non-blocking delay
- Switch statement untuk efisiensi
- Minimal delay blocking

## Troubleshooting

### Gejala Umum dan Solusi
1. **Display blank**: Call `reinitDisplay()`
2. **Tidak respon serial**: Reset komunikasi state
3. **Buzzer tidak berhenti**: Reset hardware
4. **System hang**: Watchdog akan reset otomatis

### Monitoring Health
- LED heartbeat harus berkedip setiap 30 detik
- Serial responsive dalam 5 detik
- Display clear setelah notifikasi selesai

Sistem ini dirancang untuk operasi continuous 24/7 dengan minimal downtime dan automatic recovery dari berbagai kondisi error.
