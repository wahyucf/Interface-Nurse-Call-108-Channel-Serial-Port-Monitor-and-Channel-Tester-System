# Knowledge Program: Arduino Mega 2560 - LCD 12864 ST7920 Serial Terminal

## 📋 Gambaran Umum
Program ini mengimplementasikan terminal serial dengan tampilan LCD 12864 ST7920 yang memiliki fitur auto-scroll, penomoran baris, dan kemampuan mengubah baud rate secara dinamis.

## 🎯 Fitur Utama

### ✅ Fungsi Dasar
- **Display Serial Data**: Menampilkan data dari Serial Monitor ke LCD
- **Auto-scroll**: Text bergulir otomatis saat mencapai batas layar
- **Line Numbering**: Setiap baris memiliki nomor urut (001, 002, dst.)
- **Clear & Reset**: Membersihkan layar dan mereset nomor baris

### ⚡ Kontrol
- **Tombol Fisik**: 
  - PIN 40: Clear LCD
  - PIN 41: Reset nomor baris
- **Perintah Serial**:
  - `CLEAR` - Membersihkan layar
  - `RESET` - Mereset nomor baris ke 1
  - `MENU` - Menampilkan menu baud rate
  - `BAUD:xxxxx` - Mengubah baud rate langsung

### 🔧 Baud Rate Management
- **Penyimpanan EEPROM**: Baud rate tersimpan permanen
- **Menu Interaktif**: Pemilihan baud rate saat startup
- **Multiple Options**: 11 pilihan baud rate standar

## 🔌 Konfigurasi Pin

### 📺 LCD ST7920 (Parallel 8-bit)
| Pin LCD | Pin Arduino Mega | Keterangan |
|---------|------------------|------------|
| RS      | 22               | Register Select |
| CS      | 23               | Chip Select |
| E       | 24               | Enable |
| D0-D7   | 25-32            | Data Bus 8-bit |

### 🔘 Tombol Kontrol
| Tombol | Pin Arduino | Fungsi |
|--------|-------------|---------|
| CLEAR  | 40          | Clear LCD (Active LOW) |
| RESET  | 41          | Reset line number (Active LOW) |

## 📊 Struktur Data

### Buffer Display
```cpp
String lineBuffer[4];        // Menyimpan 4 baris text
const int LCD_COLS = 16;     // Lebar LCD 16 karakter
unsigned long lineNumber = 1; // Counter nomor baris
```

### Baud Rate Configuration
```cpp
const unsigned long baudRates[] = {
  300, 1200, 2400, 4800, 9600, 14400, 
  19200, 28800, 38400, 57600, 115200
};
```

## 🔄 Alur Program

### 1. **Setup Awal**
- Load baud rate dari EEPROM
- Inisialisasi LCD dan pin
- Tampilkan splash screen
- Tawarkan menu baud rate (5 detik pertama)

### 2. **Operasi Normal**
- Monitor serial input
- Monitor tombol fisik
- Process commands
- Update display dengan auto-scroll

### 3. **Menu Baud Rate**
- Tampilkan pilihan via Serial Monitor
- Validasi input user
- Simpan ke EEPROM
- Restart komunikasi serial

## 💾 EEPROM Mapping

| Alamat | Ukuran | Keterangan |
|--------|--------|------------|
| 0-3    | 4 byte | Baud rate (unsigned long) |
| 4      | 1 byte | Validasi data (0xAA = valid) |

## 🛠️ Fungsi Penting

### Core LCD Functions
- `lcdWriteCommand()` - Kirim command ke LCD
- `lcdWriteData()` - Kirim data ke LCD
- `lcdSetXY()` - Set kursor position
- `lcdPrint()` - Print text ke LCD
- `lcdClear()` - Clear layar

### Baud Rate Management
- `saveBaudRateToEEPROM()` - Simpan baud rate
- `loadBaudRateFromEEPROM()` - Load baud rate
- `isValidBaudRate()` - Validasi baud rate
- `showBaudRateMenu()` - Tampilkan menu
- `changeBaudRate()` - Proses perubahan baud rate

## 📝 Cara Penggunaan

### 1. **Startup**
- Program menampilkan splash screen 2 detik
- Tawarkan menu baud rate selama 5 detik
- Kirim `MENU` via Serial untuk akses menu

### 2. **Operasi Normal**
```
Kirim text → Tampil di LCD dengan nomor baris
Contoh: "Hello World" → "001:Hello World"
```

### 3. **Perintah Khusus**
```
CLEAR     → Clear layar
RESET     → Reset nomor baris ke 1
MENU      → Tampilkan menu baud rate
BAUD:9600 → Ubah baud rate ke 9600
```

### 4. **Tombol Fisik**
- Tekan tombol CLEAR (PIN 40) untuk clear layar
- Tekan tombol RESET (PIN 41) untuk reset nomor baris

## ⚠️ Catatan Penting

### Baud Rate Change
- Setelah perubahan baud rate, **Serial Monitor harus ditutup dan dibuka kembali** dengan baud rate baru
- Data tersimpan permanen di EEPROM

### LCD Compatibility
- Program dikhususkan untuk LCD 12864 dengan controller ST7920
- Mode komunikasi: Parallel 8-bit

### Memory Usage
- Menggunakan Arduino Mega 2560 karena kebutuhan pin yang banyak
- Buffer text disimpan dalam String array

## 🔧 Troubleshooting

### LCD Tidak Menampilkan
- Periksa koneksi pin RS, CS, E
- Pastikan semua pin data D0-D7 terhubung
- Check power supply LCD

### Serial Tidak Responsif
- Pastikan baud rate Serial Monitor sesuai
- Cek kabel USB connection
- Restart Arduino jika diperlukan

### EEPROM Issues
- Data EEPROM bisa direset dengan mengubah alamat 4 ke nilai selain 0xAA
- Atau upload program blank untuk clear EEPROM

## 📈 Enhancement Potential

### Fitur Tambahan
- Backlight control
- Custom character support
- Scroll horizontal untuk text panjang
- Save/Restore display content
- Timestamp feature
- Multiple display modes

### Optimisasi
- Replace String dengan char array untuk stability
- Implement circular buffer
- Add LCD contrast control
- Power management features

Program ini memberikan dasar yang solid untuk pengembangan terminal serial dengan display LCD, dengan fitur management baud rate yang user-friendly dan penyimpanan setting yang persisten.
