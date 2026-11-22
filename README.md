# 💡 Smart Street Lighting IoT (Arduino Uno + ESP-12E)

Prototipe **Sistem Penerangan Jalan Pintar (Smart Street Lighting)** berbasis *Internet of Things* (IoT) untuk Ujian Akhir Semester (UAS) Mata Kuliah Internet of Things, Program Studi Teknologi Informasi, Universitas Teuku Umar.

Sistem ini mengintegrasikan **3 sensor lingkungan** (cahaya, gerak, dan suara) dengan **3 lampu terpisah** serta **buzzer alarm**, dilengkapi *web dashboard* lokal bertema *Dark Mode* untuk kontrol manual (*override*) secara *real-time* dan pencatatan data grafik *cloud* menggunakan **ThingSpeak**.

---
## 👥 Identitas Proyek (Kelompok 6)

* **Mata Kuliah:** Internet of Things (UAS)
* **Program Studi:** Teknologi Informasi (Semester 5)
* **Institusi:** Universitas Teuku Umar
* **Anggota Kelompok 6:**
  1. **Puja Syerla Virona** *(NIM: 2505903040071)*
  2. **T. Ryan** *(NIM: 2505903040105)*
  3. **Nur Majid** *(NIM: 2405903040064)*
  4. **Rislam Febriansah Putra** *(NIM: 2505903040018)*

---

## ✨ Fitur Utama

1. **Otomatisasi 1:1 (1 Sensor = 1 Lampu):**
   * **Sensor Cahaya (LDR) $\rightarrow$ Lampu 1:** Menyala otomatis saat kondisi gelap dan langsung padam saat terang (tanpa timer).
   * **Sensor Gerak (PIR) $\rightarrow$ Lampu 2:** Menyala saat mendeteksi pergerakan dan tetap aktif selama **15 detik** setelah gerakan berhenti.
   * **Sensor Suara $\rightarrow$ Lampu 3 & Buzzer:** Saat kebisingan mencapai **$\ge 65\text{ dB}$**, Lampu 3 menyala dengan timer **15 detik** dan Buzzer berbunyi sebagai alarm peringatan selama suara keras berlangsung.
2. **Kontrol Manual Web (*Override*):**
   * *Dashboard* web responsif (*Dark Mode*) yang di-*hosting* langsung oleh NodeMCU ESP-12E.
   * Mendukung kontrol **Mode AUTO**, **Semua Lampu ON/OFF**, serta kontrol terpisah untuk **Lampu 1, Lampu 2, Lampu 3, dan Buzzer**.
3. **Monitoring *Real-Time* & *Cloud Logging*:**
   * Pembaruan status sensor, hitung mundur timer, dan *progress bar* intensitas suara (dB) setiap **1 detik** di *dashboard* lokal.
   * Pengiriman data otomatis ke **ThingSpeak** (7 *field*) setiap **20 detik** via HTTP POST JSON.

---

## 🛠️ Daftar Komponen (Hardware)

| No | Komponen | Jumlah | Fungsi |
| :--- | :--- | :---: | :--- |
| 1 | **Arduino Uno** | 1 | Mikrokontroler utama pembacaan sensor, logika timer, dan aktuator |
| 2 | **NodeMCU ESP-12E (ESP8266)** | 1 | Modul Wi-Fi, *Web Server* lokal, dan pengirim data ke ThingSpeak |
| 3 | **Modul Sensor LDR (Digital)** | 1 | Mendeteksi kondisi terang/gelap |
| 4 | **Sensor PIR (HC-SR501)** | 1 | Mendeteksi keberadaan/pergerakan objek |
| 5 | **Modul Sensor Suara (Analog)** | 1 | Mengukur intensitas kebisingan (dikonversi ke dB) |
| 6 | **Modul Relay 1-Channel (*Active-Low*)** | 1 | Saklar elektronik Lampu 1 (sumber daya 4× AA) |
| 7 | **LED + Resistor 220 $\Omega$** | 3 | Simulasi Lampu Jalan 1, 2, dan 3 |
| 8 | **Buzzer** | 1 | Alarm peringatan suara keras / sirine manual (`tone()`) |
| 9 | **Baterai 9V + Klip** | 1 | Catu daya mandiri untuk Arduino Uno |
| 10 | **Baterai 4× AA (6V) + Holder** | 1 | Catu daya mandiri untuk NodeMCU ESP-12E dan Lampu 1 |
| 11 | **Breadboard & Kabel Jumper** | 1 set | Media perakitan dan jalur *Common GND* |

---

## 🔌 Konfigurasi Pin (*Wiring*)

### 1. Arduino Uno (`uno/uno.ino`)

| Pin Arduino Uno | Komponen / Tujuan | Keterangan |
| :---: | :--- | :--- |
| `5` | Lampu 2 (LED PIR) | Melalui Resistor 220 $\Omega$ ke GND |
| `6` | Lampu 3 (LED Suara) | Melalui Resistor 220 $\Omega$ ke GND |
| `7` | Sensor LDR (`DO`) | Logika `GELAP_SAAT_HIGH = true` |
| `8` | Buzzer (`+`) | Menggunakan `tone(8, 1000)` / `noTone(8)` |
| `9` | Relay (`IN`) | *Active-Low*, memutus-sambung daya 4× AA ke Lampu 1 |
| `10` | Sensor PIR (`OUT`) | Deteksi gerak (`HIGH`) |
| `A1` | Sensor Suara (`AO`) | Pembacaan ADC (titik tenang kalibrasi: `618`) |
| `11` | NodeMCU Pin `D6` (TX) | `SoftwareSerial` RX (9600 baud) |
| `12` | NodeMCU Pin `D5` (RX) | `SoftwareSerial` TX (9600 baud) |
| `GND` | Jalur `GND` Bersama | Wajib terhubung dengan GND NodeMCU & baterai 4× AA |

### 2. NodeMCU ESP-12E (`esp12e/esp12e.ino`)

| Pin NodeMCU | Tujuan | Keterangan |
| :---: | :--- | :--- |
| `D5` (GPIO14) | Arduino Uno Pin `12` | Menerima paket data sensor dari Uno |
| `D6` (GPIO12) | Arduino Uno Pin `11` | Mengirim perintah tombol web ke Uno |
| `VIN` & `GND` | Baterai 4× AA (`+` / `-`) | Cabut saat proses *upload* kode via USB |

---

## 📡 Protokol Komunikasi & *Cloud*

### 1. Komunikasi Serial (Uno $\leftrightarrow$ ESP-12E)
* **Uno $\rightarrow$ ESP (setiap 1 detik):**
  ```text
  mode|gelap|gerak|sisaDetik|suaraDB|lampuLDR|lampuPIR|lampuSuara|buzzer