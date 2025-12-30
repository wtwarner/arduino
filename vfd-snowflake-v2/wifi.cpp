#include <WiFi.h>
#include <ESPmDNS.h>

const char* ssid     = "Endor";
const char* password = "674523face";

void init_wifi()
{

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println(WiFi.localIP());

  MDNS.begin("snowflake-small");
  
}

