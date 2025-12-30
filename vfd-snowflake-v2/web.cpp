#include <map>
#include "Arduino.h"
#include <WebServer.h>
#include "TemplateTango.h"
#include "global.h"

String main_html { R"(
  <!DOCTYPE html>
<head>
   <title>VFD Small Snowflake</title>
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">
<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js" integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI" crossorigin="anonymous"></script>
</head>

<body>
   <h1>VFD Small Snowflake</h1>

<div class="p-3">
<form method="post">
<h2>Power</h2>
<div class="form-check form-switch">
  <input class="form-check-input" type="checkbox" role="switch" id="enable" name="enable" value="1"
  {{enable_chk}}
  >
  <label class="form-check-label" for="enable">Power On</label>
</div>

<h2>Pattern</h2>
<div class="form-check">
  <input class="form-check-input" type="checkbox" id="cirOut1" name="cirOut1" value="1"
{{cirOut1_chk}}
   >
   <label class="form-check-label" for="cirOut1">Circle Outward</label>
</div>
<div class="form-check">
  <input class="form-check-input" type="checkbox" id="cirOut0" name="cirOut0" value="1"
{{cirOut0_chk}}
   >
   <label class="form-check-label" for="cirOut0">Circle Outward (Invert)</label>
</div>

<div class="form-check">
  <input class="form-check-input" type="checkbox" id="test1" name="test1" value="1"
{{test1_chk}}
   >
   <label class="form-check-label" for="test1">Test1</label>
</div>

<div class="form-check">
  <input class="form-check-input" type="checkbox" id="rotate0" name="rotate0" value="1"
{{rotate0_chk}}
   >
   <label class="form-check-label" for="rotate0">Rotate</label>
</div>
<div class="form-check">
  <input class="form-check-input" type="checkbox" id="rotate1" name="rotate1" value="1"
{{rotate1_chk}}
   >
   <label class="form-check-label" for="rotate1">Rotate (Invert)</label>
</div>

<div class="form-check">
  <input class="form-check-input" type="checkbox" id="spiral0" name="spiral0" value="1"
{{spiral0_chk}}
   >
   <label class="form-check-label" for="spiral0">Spiral</label>
</div>

<div class="form-check">
  <input class="form-check-input" type="checkbox" id="spiral1" name="spiral1" value="1"
{{spiral1_chk}}
   >
   <label class="form-check-label" for="spiral1">Spiral (Invert)</label>
</div>


<h2>Timer</h2>
<div class="form-check form-switch">
  <input class="form-check-input" type="checkbox" name="timerEnable" id="timerEnable" value="1"
{{timerEnable_chk}}
  >
  <label class="firm-check-label" for="timerEnable">Enable Timer</label>
</div>

<label>On Time</label>
<div class="input-group mb-3">
      <input type="text" class="form-control" name="onTimeH" value={{onTimeH}} aria-label="H">
      <span class="input-group-text">:</span>
      <input type="text" class="form-control" name="onTimeM" value={{onTimeM}} aria-label="M">
</div>
<label>Off Time</label>
<div class="input-group mb-3">
      <input type="text" class="form-control" name="offTimeH" value={{offTimeH}} aria-label="H">     
      <span class="input-group-text">:</span>                           
      <input type="text" class="form-control" name="offTimeM" value={{offTimeM}} aria-label="M">     
</div>

<button type="submit" class="btm btn-primary">Submit</button>
</form>
</div>
</body>
)" };

WebServer server(80);

const int NUM_PATTERNS = 7;
String patterns[NUM_PATTERNS] = {
  "cirOut1", "cirOut0", "test1", "rotate0", "rotate1", "spiral0", "spiral1"
};
  
void handleRoot()
{
  // on POST, update state
  if (server.method() == HTTP_POST) {
    // switches and checkboxes are only sent if "1", so default to "0".
    uint16_t pattern = 0;
    bool enable = false;
    bool timerEnable = false;
    
    for (int i = 0; i < server.args(); i ++) {
      unsigned int val = strtol(server.arg(i).c_str(), 0, NULL);
      if (server.argName(i) == "enable") { enable = val; }
      else if (server.argName(i) == "timerEnable") { timerEnable = val; }
      else if (server.argName(i) == "onTimeH") { g_state.onTimeH = val; }
      else if (server.argName(i) == "onTimeM") { g_state.onTimeM = val; }
      else if (server.argName(i) == "offTimeH") { g_state.offTimeH = val; }
      else if (server.argName(i) == "offTimeM") { g_state.offTimeM = val; }
      else {
	for (int j = 0; j < NUM_PATTERNS; j ++) {
	  if (server.argName(i) == patterns[j] && val != 0) {
	    pattern |= (1 << j);
	  }
	}
      }
    }
    g_state.enable = enable;
    g_state.timerEnable = timerEnable;
    g_state.pattern = pattern;

    update_prefs();
  }

  // on either GET or POST, report current state by updating HTML template
  std::map<String, String> vars{
    { "enable_chk", g_state.enable ? "checked" : "" },
    { "timerEnable_chk", g_state.timerEnable ? "checked" : "" },
    { "onTimeH", String(g_state.onTimeH) },
    { "onTimeM", String(g_state.onTimeM) },
    { "offTimeH", String(g_state.offTimeH) },
    { "offTimeM", String(g_state.offTimeM) },
  };
  for (int i = 0; i < NUM_PATTERNS; i++) {
    vars[patterns[i] + "_chk"] = String((g_state.pattern & (1 << i)) ? "checked" : "");
  }
  String response = TemplateTango::render(main_html, vars);
  server.send(200, "text/html", response.c_str());
}

void init_webserver()
{
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void check_webserver()
{
  server.handleClient();
}
