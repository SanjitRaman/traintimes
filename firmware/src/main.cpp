#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "DepartureBoardDisplay.h"
#include "secrets.h"

const char* stationCode = "HDM";
const char* apiURL =
    "https://api1.raildata.org.uk/1010-live-departure-board-dep1_2/LDBWS/api/"
    "20220120/GetDepBoardWithDetails/HDM";

const int maxServices = 6;

DynamicJsonDocument doc(8192);

String callingAt[maxServices];
String stds[maxServices];
String etds[maxServices];
String destinations[maxServices];
bool cancelled[maxServices];
bool delayed[maxServices];

int viewIndex = 0, scrollOffset = 0, blinkCounter = 0;
int scrollMax =
    callingAt[viewIndex].length();  // number of characters to scroll

bool delayPhase = true;
unsigned long lastFetch = 0;
const unsigned long fetchInterval = 60000;

DepartureBoardDisplay board;

void fetchData()
{
    HTTPClient http;
    http.begin(apiURL);
    http.addHeader("x-apikey", apiKey);
    http.addHeader("User-Agent", "TrainTimesESP32/1.0");

    int httpCode = http.GET();
    if (httpCode == 200)
    {
        String payload = http.getString();
        deserializeJson(doc, payload);

        JsonArray services = doc["trainServices"];
        int count = 0;
        for (JsonObject s : services)
        {
            String crs = s["destination"][0]["crs"].as<String>();
            if (crs != "MYB")
                continue;
            stds[count] = s["std"].as<String>();
            etds[count] = s["etd"].as<String>();
            destinations[count] =
                s["destination"][0]["locationName"].as<String>();
            cancelled[count] = s["isCancelled"] | false;
            delayed[count] = (etds[count] != "On time" && !cancelled[count]);
            callingAt[count] = "";
            for (JsonObject point :
                 s["subsequentCallingPoints"][0]["callingPoint"]
                     .as<JsonArray>())
            {
                callingAt[count] +=
                    point["locationName"].as<String>() + ",";  // potential bug
            }
            count++;
            if (count >= maxServices)
                break;
        }
    }
    http.end();
}

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    configTime(0, 0, "pool.ntp.org");
    board.begin();
    fetchData();  // your fetch function as before
}

void loop()
{
    if (millis() - lastFetch >= fetchInterval)
    {
        fetchData();
        viewIndex = 0;
        scrollOffset = 0;
        blinkCounter = 0;
        delayPhase = true;
        lastFetch = millis();
    }

    board.showBoard(stds, destinations, etds, callingAt, cancelled, delayed,
                    viewIndex, scrollOffset, delayPhase);

    if (delayPhase)
    {
        if (blinkCounter++ >= 6)
            delayPhase = false;
    }
    else
    {
        scrollOffset++;
    }

    if (!delayPhase && scrollOffset >= scrollMax)
    {
        viewIndex++;
        scrollMax =
            callingAt[viewIndex].length();  // number of characters to scroll

        if (viewIndex > maxServices - 2)
            viewIndex = 0;
        scrollOffset = 0;
        delayPhase = true;
    }

    delay(500);
}
