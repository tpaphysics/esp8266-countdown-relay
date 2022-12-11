
/*
  Author: Thiago Pacheco Andrade
  Date:   Dezembro 03, 2022
*/

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "env.h"

// For AP Mode Config
const int WIFI_CHANEL = 1;
const bool WIFI_HIDDEN = false;
// const int WIFI_MAX_CONNECTIONS = 1;

// IPAddress local_IP(192, 168, 4, 22);
// IPAddress gateway(192, 168, 4, 9);
// IPAddress subnet(255, 255, 255, 0);

const int ledPin = 2;
const int detonator = 5; // D1

AsyncWebServer server(80);

String ledState;

String state(int pin)
{
  if (digitalRead(pin))
  {
    return "OFF";
  }
  else
  {
    return "ON";
  }
}

void notFound(AsyncWebServerRequest *request)
{
  request->redirect("/");
}

void setup()
{

  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(detonator, OUTPUT);

  digitalWrite(ledPin, HIGH);
  digitalWrite(detonator, LOW);

  // Begin LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // WIFI AP Mode

  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(ssid, password) ? "AP id ready!" : "AP id failed!");
  WiFi.softAP(ssid);
  WiFi.softAP(ssid, password, WIFI_CHANEL, WIFI_HIDDEN);

  Serial.print("Soft-AP IP address = ");
  Serial.println(WiFi.softAPIP());

  // Connect to WIFI STA
  // WiFi.mode(WIFI_STA);
  // WiFi.begin(ssid, password);
  // if (WiFi.waitForConnectResult() != WL_CONNECTED)
  //{
  //  Serial.printf("WiFi Failed!\n");
  //  return;
  //}

  // Print ESP32 Local IP Address
  Serial.println(WiFi.localIP());
  if (MDNS.begin("esp"))
  {
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("DNS:");
    Serial.println("esp.local");
  }

  // Web React Page

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });

  server.on("/assets/index.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/assets/index.css", "text/css"); });

  server.on("/assets/index.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/assets/index.js", "text/javascript"); });

  server.on("/connected", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "connected ok!"); });

  // Controller Test
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request)
            { digitalWrite(ledPin, !digitalRead(ledPin));
   
    StaticJsonDocument<200> doc;
    doc["led"] = state(ledPin);
    String payload;
    serializeJson(doc, payload);
    request->send(200, "application/json", payload); });

  // Trigger Post Route Relay
  AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/trigger", [](AsyncWebServerRequest *request, JsonVariant &json)
                                                                         {
    StaticJsonDocument<200> data;
    if (json.is<JsonArray>())
    {
      data = json.as<JsonArray>();
    }
    else if (json.is<JsonObject>())
    {
      data = json.as<JsonObject>();
    }

    String response;
    String msg = data["gpio"];

    if (msg == "ON")
    {
      digitalWrite(detonator, HIGH);
      serializeJson(data, response);
      request->send(200, "application/json", response);
    }
    if (msg == "OFF")
    {
      digitalWrite(detonator, LOW);
      serializeJson(data, response);
      request->send(200, "application/json", response);
    }

    StaticJsonDocument<200> dataResponse;
    dataResponse["status"] = "Error";
    dataResponse["message"] = "Invalid request body!";
    serializeJson(dataResponse, response);
    request->send(404, "application/json", response); });

  server.addHandler(handler);

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{
  MDNS.update();
}