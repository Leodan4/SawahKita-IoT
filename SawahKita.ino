// SawahKita by ZakyLeo
#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include <AdvKeyPad.h>
#include <Wire.h>
#include <EEPROM.h>
#include <HCSR04.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

char ssid[] = "SSID";
char pass[] = "Password ";
int status = WL_IDLE_STATUS;

/*
D1 5  = I2C
D2 4  = I2C
D3 0  = Relay
D4 2  = Relay
D5 14 = Temperatur
D6 12 = Ketinggian Air T
D7 13 = Ketinggian Air R
D8 15 = (only out)
*/

// Data Fuzzy daily disimpan ke SD card
// Tambahin RTC DS3231

OneWire oneWire(14);
Adafruit_ADS1115 ads;
AdvKeyPad keyPad(0x38);
DallasTemperature tempsensor(&oneWire);
UltraSonicDistanceSensor distanceSensor(12, 13);

float temp;
int kelembapan;
String keystring;
float kali;
String fuzzy;
String nextion;

unsigned long before = 0, beforeget = 0, beforepre = 0;

float TAir, ATAir, BTAir;
float volt, ampere, avgampere;
int arsample;
String sumur, sharga, sluas, spupuk, dis;
int umur, harga, luas, pupuk, hasil, total, con, page, predict;

String send_data, send_data2, send_data3, pompauser, pompa, pompanex, nexid,
    link = "Sawahkitafirebasedb/PaTani.json",
    lin1 = "Sawahkitafirebasedb/Sawah2/pompauser.json",
    lin2 = "Sawahkitafirebasedb/power.json",
    lin3 = "Sawahkitafirebasedb/prediksi.json";

void setup()
{
  Serial.begin(57600);
  EEPROM.begin(512);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("wp0.pic=");
    Serial.print(4);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("wifi.txt=\"");
    Serial.print("menghubungkan");
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print(".");
    delay(500);
  }
  Serial.print("wp0.pic=");
  Serial.print(5);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);

  Serial.println(" ");

  Wire.begin();
  Wire.setClock(400000);
  tempsensor.begin();
  if (keyPad.begin()!)
  {
    Serial.println("\nERROR: cannot communicate to keypad.\nPlease reboot.\n");
    while (1)
      ;
  }
  if (!ads.begin())
  {
    Serial.println("Failed to initialize ADS.");
    while (1)
      ;
  }

  umur = 0;
  harga = 0;
  luas = 0;
  pupuk = 0;
  /*umur = EEPROM.read(1);
  harga = EEPROM.read(2);
  luas = EEPROM.read(3);
  pupuk = EEPROM.read(4);
  sumur = umur;
  sharga = harga;
  sluas = luas;
  spupuk = pupuk;*/

  con = 0;
  page = 0;
  pompauser = "false";
  printWiFiStatus();
}

void printWiFiStatus()
{
  if (page == 0)
  {
    Serial.print("wifi.txt=\"");
    Serial.print(ssid);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  Serial.print("wifi.txt=\"");
  Serial.print(ssid);
  Serial.print("\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);

  long rssi = WiFi.RSSI();
  Serial.print("speed.txt=\"");
  Serial.print(rssi);
  Serial.print("\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);

  if (page == 1)
  {
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print("wp0.pic=");
      Serial.print(4);
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);

      Serial.print(".");
      delay(500);
    }
    Serial.print("wp0.pic=");
    Serial.print(5);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.println();
  }
}

String update_data(String send_data)
{
  WiFiClientSecure client;
  client.setInsecure(); // Ignore SSL certificate validation
  HTTPClient https;
  String response = "";
  // bool response=false;

  Serial.printf("[Firebase] UPDATE with data : %s\n", send_data.c_str());
  if (https.begin(client, link))
  { // HTTPS
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpCode = https.PUT(send_data);
    if (httpCode > 0)
    {
      response = https.getString();
      // response=true;
      Serial.printf("[Firebase] UPDATE response code: %d\n", httpCode);
      // Serial.println(response); // debug
    }
    else
    {
      Serial.printf("[Firebase] UPDATE error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else
  {
    Serial.printf("[Firebase] Unable to connect\n");
  }
  return response;
}

String get_data()
{
  WiFiClientSecure client;
  client.setInsecure(); // Ignore SSL certificate validation
  HTTPClient https;
  String response = "";
  // bool response=false;

  Serial.println("Firebase] GET data");
  if (https.begin(client, lin1))
  { // HTTPS
    int httpCode = https.GET();
    if (httpCode > 0)
    {
      response = https.getString();
      // response=true;
      Serial.printf("[Firebase] GET response code: %d\n", httpCode);
      // Serial.println(response); // debug
    }
    else
    {
      Serial.printf("[Firebase] GET error: %s\n", https.errorToString(httpCode).c_str());
    }
    https.end();
  }
  else
  {
    Serial.printf("[Firebase] Unable to connect\n");
  }
  return response;
}

void pump()
{
  if (pompauser == "true" || TAir > ATAir)
  {
    digitalWrite(2, LOW);
    digitalWrite(0, LOW);
    pompa = "true";
    if (page == 0)
    {
      /*Serial.print("onoff.txt=\"on\"");Serial.write(0xff);Serial.write(0xff);Serial.write(0xff);
      Serial.print("onoff.pco=3562");Serial.write(0xff);Serial.write(0xff);Serial.write(0xff);*/
      Serial.print("bt0.val=1");
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }
  }
  else
  {
    digitalWrite(2, HIGH);
    digitalWrite(0, HIGH);
    pompa = "false";
    if (page == 0)
    {
      /*Serial.print("onoff.txt=\"off\"");Serial.write(0xff);Serial.write(0xff);Serial.write(0xff);
      Serial.print("onoff.pco=63813");Serial.write(0xff);Serial.write(0xff);Serial.write(0xff);*/
      Serial.print("bt0.val=0");
      Serial.write(0xff);
      Serial.write(0xff);
      Serial.write(0xff);
    }
  }

  if (TAir > ATAir && page == 0)
  {
    Serial.print("t17.txt=\"Tinggi\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t17.pco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (BTAir <= TAir <= ATAir && page == 0)
  {
    Serial.print("t17.txt=\"Normal\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t17.pco=3562");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (BTAir > TAir && page == 0)
  {
    Serial.print("t17.txt=\"Rendah\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t17.pco=22078");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
}

void loop()
{
  nextion = "";
  unsigned long nowmillis = millis();
  if (page == 0 && nowmillis - beforeget > 7500)
  {
    pompauser = get_data();
    beforeget = nowmillis;
  }
  printWiFiStatus();
  // volarus();
  pump();
  if (nowmillis - before > 60000 && page == 0)
  {
    before = nowmillis;

    TempTinggi();
    AutoTAir();
    LembapTanah();
    Fuzzy();
    prediksi();

    send_data = (String("{") +
                 "\"kelembapan\":" + kelembapan + "," +
                 "\"suhu\":" + temp + "," +
                 "\"tinggiair\":" + TAir + "," +
                 "\"pompa\":" + pompa + "" +
                 "}");
    update_data(send_data);
    /*send_data2 = (String("{") +
      "\"Tegangan\":"+volt+","+
      "\"Arus\":"+ampere+""+
    "}");
    update_data2();*/
  }
  if (Serial.available())
  {
    while (Serial.available())
    {
      nextion += Serial.read();
    }
    delay(50);
  }
  Serial.println(nextion);
  if (page == 0 || page == 2)
  {
    nextion = nextion.substring(10, 40);
  }
  nexid = nextion.substring(4, 6);
  String nexus = nextion;
  Serial.println(nexus);

  if (nexid == "37")
  {
    // page2
    page = 2;
    Serial.print("page 2");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("page 2");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (nexid == "16")
  {
    // page1
    page = 1;
    Serial.print("page 1");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("page 1");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (nexid == "15")
  {
    // simpan
    /*EEPROM.put(1, umur);
    EEPROM.put(2, harga);
    EEPROM.put(3, luas);
    EEPROM.put(4, pupuk);
    EEPROM.commit();*/
    send_data3 = (String("{") +
                  "\"umur\":" + umur + "," +
                  "\"harga\":" + harga + "," +
                  "\"luas\":" + luas + "," +
                  "\"predict\":" + predict + "," +
                  "\"pupuk\":" + pupuk + "," +
                  "\"hasil\":" + hasil + "" +
                  "}");
    update_data(send_data3);
    Serial.print("t8.txt=\"Data Tersimpan!\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t8.txt=\"Data Tersimpan!\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    delay(1000);
    Serial.print("t8.txt=\" \"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (nexid == "20")
  {
    // page0
    page = 0;
    Serial.print("page 0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("page 0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (nexid == "60")
  {
    page = 0;
    Serial.print("page 0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("page 0");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }

  // pump();
  // 80=umur, 10=harga, 12=luas, 15=pupuk+, 14=pupuk-, 18=simpan, 37=page2, 20=page0
  if (page == 2)
  {
    Serial.print("wifi.txt=\"");
    Serial.print(ssid);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("umur.txt=\"");
    Serial.print(sumur);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("harga.txt=\"");
    Serial.print(sharga);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("luas.txt=\"");
    Serial.print(sluas);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("pupuk.txt=\"");
    Serial.print(spupuk);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

    if (con == 0)
    {
      input();
    }
    else if (con == 1)
    {
      keypadumur();
    }
    else if (con == 2)
    {
      keypadharga();
    }
    else if (con == 3)
    {
      keypadluas();
    }
    else if (con == 4)
    {
      keypadpupuk();
    }
  }

  if (page == 0 && nowmillis - beforepre > 30000)
  {
    send_data3 = (String("{") +
                  "\"umur\":" + umur + "," +
                  "\"harga\":" + harga + "," +
                  "\"luas\":" + luas + "," +
                  "\"predict\":" + predict + "," +
                  "\"pupuk\":" + pupuk + "," +
                  "\"hasil\":" + hasil + "" +
                  "}");
    update_data(send_data3);
    beforepre = nowmillis;
  }
}

void TempTinggi()
{
  tempsensor.requestTemperatures();
  temp = tempsensor.getTempCByIndex(0);
  Serial.print("suhu.txt=\"");
  Serial.print(temp);
  Serial.print("°C\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  float RTAir = distanceSensor.measureDistanceCm(temp); // TAir[0]
  TAir = 11.5 - RTAir;
  int airValue = TAir / 8 * 100;
  Serial.print("air.txt=\"");
  Serial.print(TAir);
  Serial.print(" cm\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("airValue.val=");
  Serial.print(airValue);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if (temp < 18)
  {
    Serial.print("t16.txt=\"Dingin\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t16.pco=22078");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (18 <= temp < 25)
  {
    Serial.print("t16.txt=\"Sejuk\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t16.pco=3562");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (25 <= temp < 33)
  {
    Serial.print("t16.txt=\"Hangat\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t16.pco=64512");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (33 <= temp)
  {
    Serial.print("t16.txt=\"Panas\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t16.pco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
}

void LembapTanah()
{
  kelembapan = 100 - (ads.readADC_SingleEnded(0) / (32768 * 5 / 6.144)) * 100;
  Serial.print("kelembapan.txt=\"");
  Serial.print(kelembapan);
  Serial.print("%\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  if (kelembapan < 65)
  {
    Serial.print("t15.txt=\"Kering\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t15.pco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (65 <= kelembapan < 93)
  {
    Serial.print("t15.txt=\"Lembap\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t15.pco=3562");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (93 <= kelembapan)
  {
    Serial.print("t15.txt=\"Basah\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t15.pco=22078");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
}

void volarus()
{
  arsample = 0;
  unsigned int y = 0;
  for (int y = 0; y < 20; y++)
  {
    arsample = arsample + ads.readADC_SingleEnded(1);
    delay(4);
  }
  avgampere = arsample / 20.0;
  ampere = (2.5 - (arsample * 5.0 / 32768.0)) / 0.066;
  volt = ads.readADC_SingleEnded(2) * 18.432 / 32768.0;
  if (page == 1)
  {
    Serial.print("Tegangan.txt\"");
    Serial.print(volt);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("Arus.txt\"");
    Serial.print(ampere);
    Serial.print("\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  delay(50);
}

void input()
{
  Serial.print("umur.bco=65535");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("harga.bco=65535");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("luas.bco=65535");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("pupuk.bco=65535");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  uint8_t key;
  key = keyPad.getKey();
  if (key != KP__NOKEY)
  {
    char keys = keyPad.getLastChar();
    Serial.println(keys);
    if (keys == 'A')
    {
      con = 1;
    }
    else if (keys == 'B')
    {
      con = 2;
    }
    else if (keys == 'C')
    {
      con = 3;
    }
    else if (keys == 'D')
    {
      con = 4;
    }
    else
    {
      con = 0;
    }
  }
}

void keypadumur()
{
  Serial.print("umur.bco=57085");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  uint8_t key;
  key = keyPad.getKey();
  if (key != KP__NOKEY)
  {
    char keys = keyPad.getLastChar();
    Serial.println(keys);
    if (keys >= '0' && keys <= '9')
    {
      dis += keys;
      sumur = dis;
      Serial.println(sumur);
    }
    else if (keys == '#')
    {
      sumur = dis;
      umur = sumur.toInt();
      Serial.println(umur);
      dis = "";
      con = 0;
    }
    else if (keys == '*')
    {
      dis = "";
      sumur = umur;
      con = 0;
    }
  }
}

void keypadharga()
{
  Serial.print("harga.bco=57085");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  uint8_t key;
  key = keyPad.getKey();
  if (key != KP__NOKEY)
  {
    char keys = keyPad.getLastChar();
    Serial.println(keys);
    if (keys >= '0' && keys <= '9')
    {
      dis += keys;
      sharga = dis;
      Serial.println(sharga);
    }
    else if (keys == '#')
    {
      sharga = dis;
      harga = sharga.toInt();
      Serial.println(harga);
      dis = "";
      con = 0;
    }
    else if (keys == '*')
    {
      dis = "";
      sharga = harga;
      con = 0;
    }
  }
}

void keypadluas()
{
  Serial.print("luas.bco=57085");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  uint8_t key;
  key = keyPad.getKey();
  if (key != KP__NOKEY)
  {
    char keys = keyPad.getLastChar();
    Serial.println(keys);
    if (keys >= '0' && keys <= '9')
    {
      dis += keys;
      sluas = dis;
      Serial.println(sluas);
    }
    else if (keys == '#')
    {
      sluas = dis;
      luas = sluas.toInt();
      Serial.println(luas);
      dis = "";
      con = 0;
    }
    else if (keys == '*')
    {
      dis = "";
      sluas = luas;
      con = 0;
    }
  }
}

void keypadpupuk()
{
  Serial.print("pupuk.bco=57085");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  uint8_t key;
  key = keyPad.getKey();
  if (key != KP__NOKEY)
  {
    char keys = keyPad.getLastChar();
    Serial.println(keys);
    if (keys >= '0' && keys <= '9')
    {
      dis += keys;
      spupuk = dis;
      Serial.println(spupuk);
    }
    else if (keys == '#')
    {
      spupuk = dis;
      pupuk = spupuk.toInt();
      Serial.println(pupuk);
      dis = "";
      con = 0;
    }
    else if (keys == '*')
    {
      dis = "";
      spupuk = pupuk;
      con = 0;
    }
  }
}

/*
dingin < 18 <= sejuk < 25 <= hangat < 33 <= panas
kering < 65 <= lembap < 93 <= basah
gagal = 0 , kurang = 40 , sedang = 70 , berhasil = 100 , melimpah = 130
berhasil = 1 ton = 1000m2
*/
void Fuzzy()
{
  if (temp < 18 && kelembapan < 65)
  { // dingin kering
    fuzzy = "gagal";
    kali = 0;
    predict = 0;
    Serial.print("t12.txt=\"Gagal\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (temp < 18 && 65 <= kelembapan < 93)
  { // dingin lembap
    fuzzy = "sedang";
    kali = 0.7;
    predict = 2;
    Serial.print("t12.txt=\"Sedang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=64512");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (temp < 18 && 93 <= kelembapan)
  { // dingin basah
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (18 <= temp < 25 && kelembapan < 65)
  { // sejuk kering
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (18 <= temp < 25 && 65 <= kelembapan < 93)
  { // sejuk lembap
    fuzzy = "melimpah";
    kali = 1.246;
    predict = 4;
    Serial.print("t12.txt=\"Melimpah\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=22078");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (18 <= temp < 25 && 93 <= kelembapan)
  { // sejuk basah
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (25 <= temp < 33 && kelembapan < 65)
  { // hangat kering
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (25 <= temp < 33 && 65 <= kelembapan < 93)
  { // hangat lembap
    fuzzy = "berhasil";
    kali = 1;
    predict = 3;
    Serial.print("t12.txt=\"Berhasil\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=3562");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (25 <= temp < 33 && 93 <= kelembapan)
  { // hangat basah
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (33 <= temp && kelembapan < 65)
  { // panas kering
    fuzzy = "gagal";
    kali = 0;
    predict = 0;
    Serial.print("t12.txt=\"Gagal\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (33 <= temp && 65 <= kelembapan < 93)
  { // panas lembap
    fuzzy = "sedang";
    kali = 0.7;
    predict = 2;
    Serial.print("t12.txt=\"Sedang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=64512");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else if (33 <= temp && 93 <= kelembapan)
  { // panas basah
    fuzzy = "kurang";
    kali = 0.4;
    predict = 1;
    Serial.print("t12.txt=\"Kurang\"");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.print("t12.bco=63813");
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
  }
}

void AutoTAir()
{
  if (0 <= umur <= 8 || 70 <= umur <= 71 || 91 <= umur <= 92)
  {
    ATAir = 1;
    BTAir = 0.2;
  }
  else if (9 <= umur <= 12 || 25 <= umur <= 28 || 45 <= umur <= 49)
  {
    ATAir = 2;
    BTAir = 0.5;
  }
  else if (13 <= umur <= 20 || 29 <= umur <= 30 || 43 <= umur <= 44 || 72 <= umur <= 76 || 89 <= umur <= 90)
  {
    ATAir = 4;
    BTAir = 1;
  }
  else if (21 <= umur <= 24 || 31 <= umur <= 36 || 61 <= umur <= 69)
  {
    ATAir = 5;
    BTAir = 1;
  }
  else if (37 <= umur <= 42 || 50 <= umur <= 60 || 77 <= umur <= 88)
  {
    ATAir = 8;
    BTAir = 3;
  }
  else
  {
    ATAir = 5;
    BTAir = 0;
  }
}

void prediksi()
{
  Fuzzy();
  hasil = luas * kali;
  total = hasil * harga;
  Serial.print("t13.txt=\"");
  Serial.print(hasil);
  Serial.print("Kg\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("t14.txt=\"Rp");
  Serial.print(harga);
  Serial.print("\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("t19.txt=\"Rp");
  Serial.print(total);
  Serial.print("\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("LTanah.txt=\"");
  Serial.print(luas);
  Serial.print(" m2\"");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}
