#include "Arduino.h"
#include <TimeLib.h>
#include "time.h"
#include "WiFi.h"

const char* ntpServer = "pool.ntp.org";

void printLocalTime();

void init_time()
{
  // Init and get the time
  configTzTime("PST8PDT", ntpServer);
  printLocalTime();

}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  setTime(mktime(&timeinfo));
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
