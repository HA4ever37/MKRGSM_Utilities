#include <MKRGSM.h>

const char PINNUMBER[]     = "";           // replace this with your sim PIN number
const char GPRS_APN[]      = "fp.com.attz";// replace your GPRS APN
const char GPRS_LOGIN[]    = "";           // replace with your GPRS login
const char GPRS_PASSWORD[] = "";           // replace with your GPRS password

// initialize the library instance
GSMLocation location;
GSMClient client;
GPRS gprs;
GSM gsmAccess;

const char server[] = "api.openweathermap.org";
const int port = 80; // port 80 is the default for HTTP

void setup() {
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only

  Serial.println("Starting Arduino web client.");
  // connection state
  bool connected = false;

  // After starting the modem with GSM.begin()
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
    } else {
      Serial.println(F("Not connected"));
      delay(1000);
      Serial.println(F("Trying again.."));
    }
  }

  String path = "/data/2.5/weather?lon=";
  Serial.println(F("Connected!"));
  location.begin();
  while (!location.available());
  Serial.println(F("Waiting to get better location accuracy"));
  while (location.accuracy() > 1000) //Unfortunately this can take a very long time! dammit SARA-U201!
    if (location.available())
      Serial.print('.');
  path += location.longitude();
  path += F("&lat=");
  path += location.latitude();
  path += F("&units=metric&appid=0a3456488cb52d167293ee9ca1f00539"); // Please use your own AppID instead of using mine :)
  //Serial.println(server + path);
  Serial.println(F("\nconnecting..."));

  // if you get a connection, report back via serial:
  if (client.connect(server, port)) {
    Serial.println(F("connected\n"));
    // Make a HTTP request:
    client.print(F("GET "));
    client.print(path);
    client.println(F(" HTTP/1.1"));
    client.print(F("Host: "));
    client.println(server);
    client.println(F("Connection: close"));
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println(F("connection failed"));
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available() && client.connected()) {
    String str;
    while (client.available())
      str += (char) client.read();
    //Serial.println(F(str + "\n\n\n"));
    char split = '"';
    int pos = str.indexOf(F("description\":\"")) + 14;
    String weather = str.substring(pos, str.indexOf(split, pos));
    Serial.println(F("Here is the weather for today:"));
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
    Serial.print(F("For the location: "));
    Serial.println(str.substring(pos, str.indexOf(split, pos)));
  }
  // if the server's disconnected, stop the client and blick the bulitin Led forever
  if (!client.available() && !client.connected()) {
    Serial.println();
    Serial.println(F("disconnecting."));
    client.stop();
    while (1) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(500);
    }
  }
}
