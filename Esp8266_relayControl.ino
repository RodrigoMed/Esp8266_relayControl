#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WebServer.h>

// WiFi credentials
const char* ssid = "";
const char* password = "";

// NTP configuration
const long gmtOffset_sec = -10800;
;                                     // GMT -3 (EST)
const int daylightOffset_sec = 3600;  // Daylight Saving Time: +1 hour
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, daylightOffset_sec);

// Relay pin
const int relayPin = 2;

// EEPROM addresses
const int startHourAddr = 0;
const int startMinAddr = 2;
const int endHourAddr = 4;
const int endMinAddr = 6;

ESP8266WebServer server(80);

// Default time range
int startHour = 0;
int startMin = 0;
int endHour = 0;
int endMin = 0;


void setup() {
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.begin(115200);

  // Set relay pin as output
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Synchronize time with NTP server
  timeClient.begin();
  while (!timeClient.update()) {
    delay(1000);
    Serial.println("Synchronizing time...");
  }
  Serial.println("Time synchronized");

  // Read time range from EEPROM
  EEPROM.begin(512);
  startHour = EEPROM.read(startHourAddr);
  startMin = EEPROM.read(startMinAddr);
  endHour = EEPROM.read(endHourAddr);
  endMin = EEPROM.read(endMinAddr);
  EEPROM.end();

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();
  Serial.println("HTTP server started");
}

void handleSet() {

  // Update time range and save to EEPROM
  startHour = server.arg("startHour").toInt();
  startMin = server.arg("startMin").toInt();
  endHour = server.arg("endHour").toInt();
  endMin = server.arg("endMin").toInt();

  EEPROM.begin(512);
  EEPROM.write(startHourAddr, startHour);
  EEPROM.write(startMinAddr, startMin);
  EEPROM.write(endHourAddr, endHour);
  EEPROM.write(endMinAddr, endMin);
  EEPROM.commit();
  EEPROM.end();

  // Redirect to the root URL
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// Function to handle the root page
void handleRoot() {
  // Read the state of the relay
  int relayState = digitalRead(relayPin);

  // Create the HTML page
  String html = "<html><body>";
  html += "<h1>ESP8266 Relay Control</h1>";
  html += "<p>Current time: " + timeClient.getFormattedTime() + "</p>";
  html += "<p>Current Range: " + String(startHour) + String(":") + String(startMin) + String(" a ") + String(endHour) + String(":") + String(endMin) + "</p>";
  html += "<p>Relay state: ";
  if (relayState == HIGH) {
    html += "OFF";
  } else {
    html += "ON";
  }
  html += "</p>";

  String form = "<h1>Set time range:</h1>";
  form += "<form action=\"/set\">";
  form += "<label for=\"startHour\">Start hour:</label>";
  form += "<input type=\"number\" id=\"startHour\" name=\"startHour\" min=\"0\" max=\"23\" value=\"" + String(startHour) + "\">";
  form += "<br>";
  form += "<label for=\"startMin\">Start minute:</label>";
  form += "<input type=\"number\" id=\"startMin\" name=\"startMin\" min=\"0\" max=\"59\" value=\"" + String(startMin) + "\">";
  form += "<br>";
  form += "<label for=\"endHour\">End hour:</label>";
  form += "<input type=\"number\" id=\"endHour\" name=\"endHour\" min=\"0\" max=\"23\" value=\"" + String(endHour) + "\">";
  form += "<br>";
  form += "<label for=\"endMin\">End minute:</label>";
  form += "<input type=\"number\" id=\"endMin\" name=\"endMin\" min=\"0\" max=\"59\" value=\"" + String(endMin) + "\">";
  form += "<br>";
  form += "<input type=\"submit\" value=\"Set time range\">";
  form += "</form>";

  html += form + "</body></html>";

  // Send the response to the client
  server.send(200, "text/html", html);
}



void loop() {
  // Update time from NTP server
  server.handleClient();
  timeClient.update();
  int hour = timeClient.getHours();
  int minute = timeClient.getMinutes();

  Serial.println(String("Hora actual: ") + String(hour) + String(":") + String(minute));
  Serial.println(String("Rango: ") + String(startHour) + String(":") + String(startMin) + String(" a ") + String(endHour) + String(":") + String(endMin));

  // Check if current time is within time range
  if (hour > startHour || (hour == startHour && minute >= startMin)) {
    if (hour < endHour || (hour == endHour && minute < endMin)) {
      Serial.println("Rele: Encendido");
      digitalWrite(relayPin, LOW);
    } else {
      Serial.println("Rele: Apagado");
      digitalWrite(relayPin, HIGH);
    }
  } else {
    Serial.println("Rele: Apagado");
    digitalWrite(relayPin, HIGH);
  }



  delay(1000);
}