#include <SoftwareSerial.h>

SoftwareSerial espSerial(11, 12); // RX = Pin 11, TX = Pin 12

const int PIN_LDR         = 7;
const int PIN_BUZZER      = 8;
const int PIN_RELAY       = 9;  // Lampu 1 (Sensor LDR - Tanpa Timer)
const int PIN_PIR         = 10;
const int PIN_SUARA       = A1;
const int PIN_LAMPU_PIR   = 5;  // Lampu 2 (Sensor PIR - Timer 15 detik)
const int PIN_LAMPU_SUARA = 6;  // Lampu 3 (Sensor Suara - Timer 15 detik)

const int BATAS_SUARA_DB = 65;
const unsigned long DURASI_LAMPU = 15000; // 15 detik untuk PIR & Suara
const unsigned long JEDA_LOG = 1000;

const bool GELAP_SAAT_HIGH = true;
const bool RELAY_ACTIVE_LOW = true;

unsigned long waktuGerakTerakhir = 0;
unsigned long waktuSuaraTerakhir = 0;
unsigned long waktuLogTerakhir   = 0;

bool pernahAdaGerakan = false;
bool pernahAdaSuara   = false;

// Kontrol manual terpisah untuk masing-masing lampu & buzzer
bool manualL1Aktif = false, manualL1State = false;
bool manualL2Aktif = false, manualL2State = false;
bool manualL3Aktif = false, manualL3State = false;
bool manualBuzzerAktif = false, manualBuzzerState = false;

bool statusBuzzerSebelumnya = false;

void setLampu1(bool nyala) {
  digitalWrite(PIN_RELAY, RELAY_ACTIVE_LOW ? (nyala ? LOW : HIGH) : (nyala ? HIGH : LOW));
}

void setLampu2(bool nyala) {
  digitalWrite(PIN_LAMPU_PIR, nyala ? HIGH : LOW);
}

void setLampu3(bool nyala) {
  digitalWrite(PIN_LAMPU_SUARA, nyala ? HIGH : LOW);
}

void setBuzzer(bool bunyi) {
  if (bunyi == statusBuzzerSebelumnya) return; // Cegah reset tone() berulang
  statusBuzzerSebelumnya = bunyi;

  if (bunyi) {
    tone(PIN_BUZZER, 1000);
  } else {
    noTone(PIN_BUZZER);
  }
}

int konversiKeDesibel(int adcVal) {
  int selisih = abs(adcVal - 618); // 618 adalah nilai tengah saat ruangan sepi
  int db = map(selisih, 0, 80, 35, 95);
  return constrain(db, 30, 100);
}

void cekPerintahESP() {
  while (espSerial.available() > 0) {
    String cmd = espSerial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) continue;

    Serial.println("PERINTAH MASUK: " + cmd);

    if (cmd == "CMD:AUTO") {
      manualL1Aktif = manualL2Aktif = manualL3Aktif = false;
      manualBuzzerAktif = false;
    } else if (cmd == "CMD:LAMPU_ON") {
      manualL1Aktif = manualL2Aktif = manualL3Aktif = true;
      manualL1State = manualL2State = manualL3State = true;
    } else if (cmd == "CMD:LAMPU_OFF") {
      manualL1Aktif = manualL2Aktif = manualL3Aktif = true;
      manualL1State = manualL2State = manualL3State = false;
    } else if (cmd == "CMD:L1_ON") {
      manualL1Aktif = true; manualL1State = true;
    } else if (cmd == "CMD:L1_OFF") {
      manualL1Aktif = true; manualL1State = false;
    } else if (cmd == "CMD:L2_ON") {
      manualL2Aktif = true; manualL2State = true;
    } else if (cmd == "CMD:L2_OFF") {
      manualL2Aktif = true; manualL2State = false;
    } else if (cmd == "CMD:L3_ON") {
      manualL3Aktif = true; manualL3State = true;
    } else if (cmd == "CMD:L3_OFF") {
      manualL3Aktif = true; manualL3State = false;
    } else if (cmd == "CMD:BUZZER_ON") {
      manualBuzzerAktif = true; manualBuzzerState = true;
    } else if (cmd == "CMD:BUZZER_OFF") {
      manualBuzzerAktif = true; manualBuzzerState = false;
    }
  }
}

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  pinMode(PIN_LDR, INPUT);
  pinMode(PIN_SUARA, INPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LAMPU_PIR, OUTPUT);
  pinMode(PIN_LAMPU_SUARA, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  setLampu1(false);
  setLampu2(false);
  setLampu3(false);
  noTone(PIN_BUZZER);

  Serial.println("=== SMART STREET LIGHTING UNO SIAP ===");
}

void loop() {
  cekPerintahESP();

  unsigned long sekarang = millis();

  // 1. Sensor LDR (Tanpa Timer)
  int rawLDR = digitalRead(PIN_LDR);
  bool isGelap = GELAP_SAAT_HIGH ? (rawLDR == HIGH) : (rawLDR == LOW);

  // 2. Sensor Suara + Timer 15 detik
  int adcSuara = analogRead(PIN_SUARA);
  int suaraDB = konversiKeDesibel(adcSuara);
  bool suaraKeras = (suaraDB >= BATAS_SUARA_DB);
  if (suaraKeras) {
    pernahAdaSuara = true;
    waktuSuaraTerakhir = sekarang;
  }
  bool timerSuaraAktif = pernahAdaSuara && ((sekarang - waktuSuaraTerakhir) < DURASI_LAMPU);

  // 3. Sensor PIR + Timer 15 detik
  bool gerakTerdeteksi = (digitalRead(PIN_PIR) == HIGH);
  if (gerakTerdeteksi) {
    pernahAdaGerakan = true;
    waktuGerakTerakhir = sekarang;
  }
  bool timerPIRAktif = pernahAdaGerakan && ((sekarang - waktuGerakTerakhir) < DURASI_LAMPU);

  // Logika Otomatis 1 Sensor = 1 Lampu (atau Override Manual)
  bool lampuLDR   = manualL1Aktif ? manualL1State : isGelap;                           // Tanpa timer
  bool lampuPIR   = manualL2Aktif ? manualL2State : (gerakTerdeteksi || timerPIRAktif); // Timer 15 dtk
  bool lampuSuara = manualL3Aktif ? manualL3State : (suaraKeras || timerSuaraAktif);    // Timer 15 dtk

  bool adaManualLampu = (manualL1Aktif || manualL2Aktif || manualL3Aktif);

  // Status Mode
  String statusMode = "SIANG";
  if (adaManualLampu) {
    statusMode = "MANUAL_WEB";
  } else if (gerakTerdeteksi) {
    statusMode = "GERAK_AKTIF";
  } else if (timerPIRAktif || timerSuaraAktif) {
    statusMode = "TUNDA_15S";
  } else if (isGelap) {
    statusMode = "GELAP_SEPI";
  }

  // Logika Buzzer
  bool bunyikanBuzzer = manualBuzzerAktif ? manualBuzzerState : suaraKeras;
  if (manualBuzzerAktif && !adaManualLampu) {
    statusMode = "BUZZER_MANUAL";
  }

  setLampu1(lampuLDR);
  setLampu2(lampuPIR);
  setLampu3(lampuSuara);
  setBuzzer(bunyikanBuzzer);

  if (sekarang - waktuLogTerakhir >= JEDA_LOG) {
    waktuLogTerakhir = sekarang;

    unsigned long sisaDetik = 0;
    if (!manualL2Aktif && !gerakTerdeteksi && timerPIRAktif) {
      unsigned long s = (DURASI_LAMPU - (sekarang - waktuGerakTerakhir) + 999) / 1000;
      if (s > sisaDetik) sisaDetik = s;
    }
    if (!manualL3Aktif && !suaraKeras && timerSuaraAktif) {
      unsigned long s = (DURASI_LAMPU - (sekarang - waktuSuaraTerakhir) + 999) / 1000;
      if (s > sisaDetik) sisaDetik = s;
    }

    // Tampilkan data debug di Serial Monitor (9600 baud)
    Serial.print("LDR: "); Serial.print(isGelap);
    Serial.print(" | PIR: "); Serial.print(gerakTerdeteksi);
    Serial.print(" | ADC Suara: "); Serial.print(adcSuara);
    Serial.print(" | dB: "); Serial.print(suaraDB);
    Serial.print(" | L1/L2/L3: ");
    Serial.print(lampuLDR); Serial.print("/");
    Serial.print(lampuPIR); Serial.print("/");
    Serial.print(lampuSuara);
    Serial.print(" | BUZ: "); Serial.println(bunyikanBuzzer);

    // Kirim paket data ke ESP-12E
    String paket = statusMode + "|" +
                   String(isGelap ? 1 : 0) + "|" +
                   String(gerakTerdeteksi ? 1 : 0) + "|" +
                   String(sisaDetik) + "|" +
                   String(suaraDB) + "|" +
                   String(lampuLDR ? 1 : 0) + "|" +
                   String(lampuPIR ? 1 : 0) + "|" +
                   String(lampuSuara ? 1 : 0) + "|" +
                   String(bunyikanBuzzer ? 1 : 0);

    espSerial.println(paket);
  }
}