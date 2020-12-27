#include <ArduinoLowPower.h>
#include <MKRGSM.h>
#include <RTCZero.h>

const char PINNUMBER[]     = "";           // replace this with your sim PIN number
const char GPRS_APN[]      = "fp.com.attz";// replace your GPRS APN
const char GPRS_LOGIN[]    = "";           // replace with your GPRS login
const char GPRS_PASSWORD[] = "";           // replace with your GPRS password

GSMLocation location;
GPRS gprs;
GSM gsmAccess;
RTCZero rtc;

bool connected, locData, timeSync;
float lat, lon;

void GSMconnect() {
  while (!connected) {
    Serial.println(F("Trying to connect.."));
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
      updateTime();
    } else {
      Serial.println(F("Not connected"));
      delay(1000);
    }
  }
  Serial.println(F("GPRS/3G Connected!\n"));
}

void GSMloc() {
  if (!connected)
    GSMconnect();
  location.begin();
  while (!location.available());
  Serial.println(F("Waiting for better location accuracy"));
  while (location.accuracy() > 1000) //Unfortunately this can take a very long time! dammit SARA-U201!
    if (location.available())
      Serial.print('.');
  locData = true;
  lon = location.longitude();
  lat = location.latitude();
  Serial.print(F("\nlocation longitude: "));
  Serial.println(lon);
  Serial.print(F("location latitude: "));
  Serial.println(lat);
  Serial.println();
}

String request(String server, String path, bool secure) {
  if (!connected)
    GSMconnect();
  char serChar[server.length() + 1];
  strcpy(serChar, server.c_str());
  if (gprs.ping(F("google.com")) < 0) {
    restModem();
    GSMconnect();
  }
  String req;
  if (secure) {
    GSMSSLClient client;
    int port = 443;
    if (client.connect(serChar, port)) {
      client.print(F("GET "));
      client.print(path);
      client.println(F(" HTTP/1.1"));
      client.print(F("Host: "));
      client.println(server);
      client.println(F("Connection: close"));
      client.println();
      while (!client.available());
      while (client.available())
        req += (char)client.read();
      client.stop();
    }
    else {
      Serial.println(F("Request failed"));
      restModem();
    }
  }
  else {
    GSMClient client;
    int port = 80;
    if (client.connect(serChar, port)) {
      client.print(F("GET "));
      client.print(path);
      client.println(F(" HTTP/1.1"));
      client.print(F("Host: "));
      client.println(server);
      client.println(F("Connection: close"));
      client.println();
      while (!client.available());
      while (client.available())
        req += (char)client.read();
      client.stop();
    }
    else {
      Serial.println(F("Request failed"));
      restModem();
    }
  }
  return req;
}

void weatherInfo() {
  if (!connected)
    GSMconnect();
  if (!locData)
    GSMloc();
  String server = F("api.openweathermap.org");
  String path = F("/data/2.5/weather?lon=");
  path += String(lon);
  path += F("&lat=");
  path += String(lat);
  path += F("&units=metric&appid=0a3456488cb52d167293ee9ca1f00539"); // Please use your own AppID instead of using mine :)
  Serial.println(F("\nConnecting to the weather info server..."));
  String str = request(server, path , false);
  char split = '"';
  int pos = str.indexOf(F("description\":\"")) + 14;
  String weather = str.substring(pos, str.indexOf(split, pos));
  if (weather != NULL) {
    Serial.println(F("Here is the weather information for today:"));
    weather[0] = toupper(weather[0]);
    Serial.println(weather);
    split = ',';
    pos = str.indexOf(F("temp\":")) + 6;
    Serial.print(F("Temperature:"));
    Serial.print(str.substring(pos, str.indexOf(split, pos)));
    Serial.println(F(" C"));
    pos = str.indexOf(F("p_min\":")) + 7;
    Serial.print(F("Minimum temperature:"));
    Serial.print(str.substring(pos, str.indexOf(split, pos)));
    Serial.println(F(" C"));
    pos = str.indexOf(F("p_max\":")) + 7;
    Serial.print(F("Maximum temperature:"));
    Serial.print(str.substring(pos, str.indexOf(split, pos)));
    Serial.println(F(" C"));
    split = '}';
    pos = str.indexOf(F("humidity\":")) + 10;
    Serial.print(F("Humidity level:"));
    Serial.print(str.substring(pos, str.indexOf(split, pos)));
    Serial.println(F("%"));
    split = ',';
    pos = str.indexOf(F("speed\":")) + 7;
    Serial.print(F("Wind speed: "));
    Serial.print(str.substring(pos, str.indexOf(split, pos)));
    Serial.println(F(" m/s"));
    split = '\"';
    pos = str.indexOf(F("name\":")) + 7;
    Serial.print(F("Area name: "));
    Serial.println(str.substring(pos, str.indexOf(split, pos)) + '\n');
  }
  else
    Serial.println(F("Failed to get weather info!\n"));
}

void serialGSM() {
  Serial.println(F("Preparing GSM Serial Passthrough session.."));
  restModem();
  SerialGSM.begin(115200);
  delay(5000);
  SerialGSM.readString();
  Serial.println(F("Ready!\n"));
  while (true) {
    if (Serial.available())
      SerialGSM.write(Serial.read());
    if (SerialGSM.available())
      Serial.write(SerialGSM.read());
  }
}

void sleepMode(byte minutes) {
  updateTime();
  rtc.setAlarmTime(rtc.getHours(), rtc.getMinutes() + minutes, 00);
  rtc.enableAlarm(rtc.MATCH_HHMMSS);
  rtc.attachInterrupt(alarmMatch);
  Serial.println(F("Entering sleep mode.."));
  gsmAccess.shutdown();
  rtc.standbyMode();
  LowPower.deepSleep();
}

void alarmMatch() {
  restModem();
  NVIC_SystemReset();
}

void restModem() {
  pinMode(GSM_RESETN, OUTPUT);
  digitalWrite(GSM_RESETN, HIGH);
  delay(100);
  digitalWrite(GSM_RESETN, LOW);
  connected = false;
}

void updateTime() {
  if (!timeSync)
    if (!connected)
      GSMconnect();
    else {
      rtc.begin();
      rtc.setEpoch(gsmAccess.getTime());
      timeSync = true;
    }
}

void showTime() {
  updateTime();
  print2digits(rtc.getDay());
  Serial.print("/");
  print2digits(rtc.getMonth());
  Serial.print("/");
  print2digits(rtc.getYear());
  Serial.print("  ");
  print2digits(rtc.getHours());
  Serial.print(":");
  print2digits(rtc.getMinutes());
  Serial.print(":");
  print2digits(rtc.getSeconds());
  Serial.println('\n');
}

void print2digits(int number) {
  if (number < 10) {
    Serial.print(F('0')); // print a 0 before if the number is < than 10
  }
  Serial.print(number);
}

void setup() {
  Serial.begin(115200);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only
}

void loop() {
  Serial.println(F("Choose one of the following:\n'C' Connect to GPRS/3G\n'L' for location information\n'U' for a URL request\n'R' to restart the modem\n'W' for weather info\n'T' to display the date & time\n'S' for GSM Serial Passthrough (Must restart to quit)\n'X' to sleep for a number of minutes"));
  while (!Serial.available());
  char c = (char)Serial.read();
  while (Serial.available())
    Serial.read();

  if (c == 'c' || c == 'C')
    GSMconnect();
  else if (c == 'l' || c == 'L')
    GSMloc();
  else if (c == 'w' || c == 'W')
    weatherInfo();
  else if (c == 's' || c == 'S')
    serialGSM();
  else if (c == 't' || c == 'T')
    showTime();
  else if (c == 'r' || c == 'R') {
    restModem();
    Serial.println(F("Modem is restarted\n"));
  }
  else if (c == 'u' || c == 'U') {
    Serial.println(F("Enter the URL (without http://www.): "));
    while (!Serial.available());
    String s = Serial.readString();
    Serial.println(F("Is it HTTPS/SSL URL? (y/n) "));
    while (!Serial.available());
    c = (char)Serial.read();
    while (Serial.available())
      Serial.read();
    String server = s.substring(0, s.indexOf('/'));
    String path = s.substring(server.length(), s.length());
    server.trim();
    path.trim();
    Serial.println(F("Building request.."));
    if (c == 'y' || c == 'Y')
      Serial.println(request(server, path, true));
    else
      Serial.println(request(server, path, false));
  }
  else if (c == 'x' || c == 'X') {
    Serial.println(F("How many minutes ?"));
    while (!Serial.available());
    byte b = (byte)Serial.readString().toInt();
    if (b == 0)
      Serial.println(F("Wrong entry!"));
    else {
      Serial.print(F("The device will sleep for : "));
      Serial.print(b);
      Serial.println(F(" minute(s)"));
      sleepMode(b);
    }
  }
  else
    Serial.println(F("Wrong entry! Please try again\n"));
}
