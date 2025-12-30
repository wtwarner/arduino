#include <map>
#include "Arduino.h"
#include <WebServer.h>
#include "TemplateTango.h"

String main_html { R"(
  <!DOCTYPE html>
<head>
   <title>VFD Snowflake Web Server</title>
<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-sRIl4kxILFvY47J16cr9ZwB07vP4J8+LH7qKQnuqkuIAvNWLzeN8tE5YBujZqJLB" crossorigin="anonymous">
<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.8/dist/js/bootstrap.bundle.min.js" integrity="sha384-FKyoEForCGlyvwx9Hj09JcYn3nv7wiPVlz7YYwJrWVcXK/BmnVDxM+D2scQbITxI" crossorigin="anonymous"></script>
</head>

<body>
   <h1>VFD Snowflake</h1>

<div class="p-3">
<form method="post">
<h2>Power</h2>
<div class="form-check form-switch">
  <input class="form-check-input" type="checkbox" role="switch" id="power" name="power" value="1"
  {{power_chk}}
  >
  <label class="form-check-label" for="power">Power On</label>
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

void handleRoot() {
  std::map<String, String> vars{
   { "power_chk", "checked" },
   { "cirOut1_chk", "checked" },
   { "cirOut0_chk", "checked" },
   { "test1_chk", "" },
   { "rotate0_chk", "checked" },
   { "rotate1_chk", "checked" },
   { "spiral0_chk", "checked" },
   { "spiral1_chk", "checked" },
   { "onTimeH", "4" },
   { "onTimeM", "30" },
   { "offTimeH", "22" },
   { "offTimeM", "30" },
  };
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
