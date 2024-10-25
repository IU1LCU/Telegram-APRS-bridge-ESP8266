/*******************************************************************/
//APRS

#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
ESP8266WiFiMulti WiFiMulti;
/////// User Data APRS-IS///////
const char* callsign = "YOURCALL";  //Callsign
const char* aprsid = "12";        //APRS-ID
const char* icon = "Z";           // APRS Icon (see on google "APRS synbol")
const char* aprspass = "APRS-IS PASS";   // APRS-IS pass generated from https://www.iz3mez.it/aprs-passcode/
const char* vers = "TestESP";     //Device, not important
const char* comment = "Comment";
////// Server Data APRS-IS ///////
const uint16_t port = 14580;            //APRS-IS Default port
const char* host = "france.aprs2.net";  //APRS-IS Server
// Wifi network station credentials
#define WIFI_SSID "WIFINAME"
#define WIFI_PASSWORD "WIFIPASS"
// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "token from botfather"

/*******************************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

const unsigned long BOT_MTBS = 1000;  // mean time between scan messages

unsigned long bot_lasttime;  // last time messages' scan has been done
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

void aprs(float latitudine, float longitudine) {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    HTTPClient http;
    WiFiClient client;  // Wifi client mode
    delay(5000);

    if (!client.connect(host, port)) {
      return;  // If connection fail, quit the loop
    }
    String userpass = "user " + String(callsign) + "-" + String(aprsid) + " pass " + String(aprspass) + " vers " + String(vers);
    Serial.println(userpass);
    client.println(userpass);

    delay(300);
    String packet = String(callsign) + "-" + String(aprsid) + ">APRSWX,TCPIP*,qAC,WIDE1-1:=" + String(convertDecimalToNMEA(latitudine)) + "/" + String(convertLongitudeToNMEA(longitudine)) + String(icon);  //Basic aprs
    client.println(packet);
    Serial.println(packet);
  }
  //delay(300000); the delay doesnt work for live sharing, cause it dont refresh the new position!!!! dont use it
}

//CONVERSIONE

String convertDecimalToNMEA(float decimalLatitude) {
  int degrees = int(decimalLatitude);                                       // Ottieni i gradi come parte intera
  float minutesDecimal = (decimalLatitude - degrees) * 60;                  // Calcola i minuti decimali
  String nmeaLatitude = String(degrees) + String(minutesDecimal, 2) + "N";  // Formatta come stringa con due cifre decimali per i minuti
  return nmeaLatitude;
}
String convertLongitudeToNMEA(float decimalLongitude) {
  int degrees = int(decimalLongitude);                                                                    // Ottieni i gradi come parte intera
  float minutesDecimal = (decimalLongitude - degrees) * 60;                                               // Calcola i minuti decimali
  String nmeaLongitude = (degrees < 100 ? "00" : "") + String(degrees) + String(minutesDecimal, 2) + "E";  // Formatta come stringa con tre cifre per i gradi (aggiunge zeri iniziali)

  return nmeaLongitude;
}
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    Serial.print(bot.messages[i].from_name);
    Serial.print(" ID: ");
    Serial.print(chat_id);
    Serial.print(" Ha scritto: ");
    Serial.println(text);

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (bot.messages[i].longitude != 0 || bot.messages[i].latitude != 0) {
      float latitudine = (bot.messages[i].latitude);
      float longitudine = (bot.messages[i].longitude);
      Serial.print("LAT NMEA ");
      Serial.println(convertDecimalToNMEA(latitudine));  // Stampa 4410.36N
      Serial.print("LONG NMEA ");
      Serial.println(convertLongitudeToNMEA(longitudine));  // Stampa 00820.47E
      Serial.print("Long: ");
      Serial.println(String(bot.messages[i].longitude, 6));
      Serial.print("Lat: ");
      Serial.println(String(bot.messages[i].latitude, 6));


      String message = "Long: " + String(bot.messages[i].longitude, 6) + "\n";
      message += "Lat: " + String(bot.messages[i].latitude, 6) + "\n";
      message += "Long NMEA: " + convertLongitudeToNMEA(longitudine) + "\n";
      message += "Lat NMEA: " + convertDecimalToNMEA(latitudine) + "\n";
      bot.sendMessage(chat_id, message, "Markdown");
      bot.sendMessage(chat_id, "Sending APRS", "");

      aprs(latitudine, longitudine);
    } else if (text == "/start") {
      String welcome = "Welcome to Telegram-APRS Bridge " + from_name + ".\n";
      welcome += "Share a location or a live location to send your position to APRS.fi\n";
      welcome += "Pay attention that live location refresh every 30 second, it mean that you send aprs packet every 30 second!!\n";

      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  // attempt to connect to Wifi network:
  configTime(0, 0, "pool.ntp.org");       // get UTC time via NTP
  secured_client.setTrustAnchors(&cert);  // Add root certificate for api.telegram.org
  Serial.print("Connecting to Wifi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // Check NTP/Time, usually it is instantaneous and you can delete the code below.
  Serial.print("Retrieving time: ");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(100);
    now = time(nullptr);
  }
  Serial.println(now);
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}
