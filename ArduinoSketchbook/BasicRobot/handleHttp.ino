/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  //handleFileRead("/index.html");
  // code borrowed from https://automatedhome.party/2017/07/15/wifi-controlled-car-with-a-self-hosted-htmljs-joystick-using-a-wemos-d1-miniesp8266/

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html>"
    "<head>"
    "<meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0'>"
    "<link rel='stylesheet' type='text/css' href='style.css'>"
    "</head>"
  );
  server.sendContent(
    "<body class='bodybg'>"
    "<div id='container'></div>"
    "<div id='info'>"
    "Touch the screen and move<br />Robot SSID: "
  );
  server.sendContent(String(BookWorm.SSID));
  server.sendContent("</div>");
  if (BookWorm.nvm.advanced)
  {
    server.sendContent("<div id='topbar' width='100%'><table width='98%'>");
    server.sendContent(
      "<tr><td width='50%'><input type='button' value='Weapon Safe' onclick='weapsetpossafe()' /></td><td width='50%'><input type='button' value='Pos A' onclick='weapsetposa()' /></td></tr>"
    );
    server.sendContent(
      "<tr><td width='50%'><input type='button' value='Flip' onclick='flip()' /><input type='checkbox' id='flip1' onclick='flip()' /></td><td width='50%'><input type='button' value='Pos B' onclick='weapsetposb()' /></td></tr>"
    );
    server.sendContent("</table></div>");
    server.sendContent("<div id='sidebar' ");
    if (BookWorm.nvm.leftHanded) {
      server.sendContent(" class='rightside' ");
    }
    server.sendContent("><table>");
    server.sendContent(
      "<tr><td><input type='button' value='Weapon Safe' onclick='weapsetpossafe()' /></td></tr><tr><td><input type='button' value='Pos A' onclick='weapsetposa()' /></td></tr>"
    );
    server.sendContent(
      "<tr><td><input type='button' value='Pos B' onclick='weapsetposb()' /></td></tr><tr><td><input type='button' value='Flip' onclick='flip()' /><input type='checkbox' id='flip2' onclick='flip()' /></td></tr>"
    );
    server.sendContent("</table></div>");
  }
  server.sendContent("<script>\n");
  server.sendContent("var desiredStickRadius = "); server.sendContent(String(BookWorm.nvm.stickRadius)); server.sendContent(";\n");
  server.sendContent("var weapPosSafe = ");
  if (BookWorm.nvm.advanced) {
    server.sendContent(String(BookWorm.nvm.weapPosSafe));
  }
  else {
    server.sendContent("0");
  }
  server.sendContent(";\n");
  server.sendContent("var weapPosA = "); server.sendContent(String(BookWorm.nvm.weapPosA)); server.sendContent(";\n");
  server.sendContent("var weapPosB = "); server.sendContent(String(BookWorm.nvm.weapPosB)); server.sendContent(";\n");
  server.sendContent(
    "</script>"
    "<script src='joy.js'></script>"
    "</body>"
    "</html>"
  );
  
  server.client().stop(); // stop() is needed if we sent no content length
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // stop() is needed if we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}

void handleMove() {
  int i;
  bool gotLeft = false;
  bool gotRight = false;
  bool gotX = false;
  bool gotY = false;
  bool gotWeap = false;
  bool gotFlip = false;
  for (i = 0; i < server.args(); i++)
  {
    String argName = server.argName(i);
    signed int v;
    if (argName.length() <= 0) {
      continue;
    }

#define IF_HANDLE_MOVE_ARG(__argName, __varName, __typeCast, __flagName)   if (argName.equalsIgnoreCase(String(__argName))) { if (readServerArg(i, &v)) { (__varName) = (__typeCast)v; (__flagName) = true; } }

         IF_HANDLE_MOVE_ARG("left",  speedLeft,  signed int, gotLeft)
    else IF_HANDLE_MOVE_ARG("right", speedRight, signed int, gotRight)
    else IF_HANDLE_MOVE_ARG("x",     speedX,     signed int, gotX)
    else IF_HANDLE_MOVE_ARG("y",     speedY,     signed int, gotY)
    else IF_HANDLE_MOVE_ARG("weap",  speedWeap,  signed int, gotWeap)
    else if (argName.equalsIgnoreCase(String("flipped")))
    {
      String argVal = server.arg(i);
      gotFlip = true;
      if (argVal.equalsIgnoreCase(String("true")) || argVal.equalsIgnoreCase(String("yes"))) {
        BookWorm.setRobotFlip(true);
      }
      else if (readServerArg(i, &v)) {
        BookWorm.setRobotFlip((v != 0));
      }
      else
      {
        BookWorm.setRobotFlip(false);
      }
    }
  }
  if (gotFlip == false)
  {
    BookWorm.setRobotFlip(false);
  }
  if (gotWeap)
  {
    // TODO: weapon motor is unimplemented
    gotWeap = gotWeap;
  }
  if ((gotLeft && gotRight) || (gotX && gotY))
  {
    lastCommTimestamp = millis();
    moveMixedMode = (gotX && gotY);
  }
}

void handleSetSSID() {
  String str = server.arg("ssid");
  BookWorm.setSsid((char*)str.c_str());
}

void handleSetConfig() {
  int i;
  bool commit = false;
  bool reboot = false;
  bool factoryreset = false;

  serveConfigHeader();

  for (i = 0; i < server.args(); i++)
  {
    String argName = server.argName(i);
    signed int v;
    if (argName.length() <= 0) {
      continue;
    }

#define IF_HANDLE_CONFIG_ARG(__argName, __funcName, __typeCast)   if (argName.equalsIgnoreCase(String(__argName))) { if (readServerArg(i, &v)) { BookWorm.__funcName((__typeCast)v); commit = true; server.sendContent(__argName); server.sendContent(" set to = "); server.sendContent(String(v)); server.sendContent("<br />\n"); } }

         IF_HANDLE_CONFIG_ARG("deadzoneleft",  setServoDeadzoneLeft,  signed int)
    else IF_HANDLE_CONFIG_ARG("deadzoneright", setServoDeadzoneRight, signed int)
    else IF_HANDLE_CONFIG_ARG("biasleft",      setServoBiasLeft,      signed int)
    else IF_HANDLE_CONFIG_ARG("biasright",     setServoBiasRight,     signed int)
    else IF_HANDLE_CONFIG_ARG("servomax",      setServoMax,           unsigned int)
    else IF_HANDLE_CONFIG_ARG("speedgain",     setSpeedGain,          unsigned int)
    else IF_HANDLE_CONFIG_ARG("steeringsensitivity", setSteeringSensitivity, unsigned int)
    else IF_HANDLE_CONFIG_ARG("steeringbalance",     setSteeringBalance,     signed int)
    else IF_HANDLE_CONFIG_ARG("servostoppednopulse", setServoStoppedNoPulse, bool)
    else IF_HANDLE_CONFIG_ARG("servoflip",   setServoFlip,   bool)
    else IF_HANDLE_CONFIG_ARG("stickradius", setStickRadius, uint16_t)
    else IF_HANDLE_CONFIG_ARG("advanced",    setAdvanced,    bool)
    else IF_HANDLE_CONFIG_ARG("weappossafe", setWeapPosSafe, uint16_t)
    else IF_HANDLE_CONFIG_ARG("weapposa",    setWeapPosA,    uint16_t)
    else IF_HANDLE_CONFIG_ARG("weapposb",    setWeapPosB,    uint16_t)
    else IF_HANDLE_CONFIG_ARG("lefthanded",  setLeftHanded,  bool)
    else if (argName.equalsIgnoreCase(String("ssid")))
    {
      String argVal = server.arg(i);
      BookWorm.setSsid((char*)argVal.c_str());
      commit = true;
    }
    else if (argName.equalsIgnoreCase(String("reboot")))
    {
      String argVal = server.arg(i);
      if (argVal.equalsIgnoreCase(String("true")) || argVal.equalsIgnoreCase(String("yes"))) {
        reboot = true;
      }
      else if (readServerArg(i, &v)) {
        reboot = (v != 0);
      }
    }
    else if (argName.equalsIgnoreCase(String("factoryreset")))
    {
      String argVal = server.arg(i);
      if (argVal.equalsIgnoreCase(String("true")) || argVal.equalsIgnoreCase(String("yes"))) {
        factoryreset = true;
      }
      else if (readServerArg(i, &v)) {
        factoryreset = (v != 0);
      }
    }
  }
  if (factoryreset)
  {
    BookWorm.factoryReset();
  }
  if (commit)
  {
    BookWorm.saveNvm();
  }

  if (reboot)
  {
    server.sendContent(String("\n<h1>WARNING: REBOOTING!!!</h1>\n"));
  }
  else
  {
    serveConfigTable();
    serveConfigNet();
  }

  serveConfigHexStr();

  server.sendContent(String("</body></html>"));
  server.client().stop(); // Stop is needed because we sent no content length

  if (reboot)
  {
    ESP.restart();
  }
}

void handleConfig() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }

  serveConfigHeader();
  serveConfigHexStr();
  serveConfigNet();
  serveConfigTable();
  
  server.sendContent(String("</body></html>"));

  server.client().stop(); // Stop is needed because we sent no content length
}

void serveConfigHeader()
{
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  server.sendContent("<html><head><title>Robot Config</title>");
  server.sendContent("<script src='config.js'></script>");
  server.sendContent("<meta name='viewport' content='width=device-width, user-scalable=no, minimum-scale=1.0, maximum-scale=1.0'>");
  server.sendContent("<link rel='stylesheet' type='text/css' href='config.css'>");
  server.sendContent("</head><body><h1>Robot Config</h1>");
}

void serveConfigHexStr()
{
  char str[3];
  int i;
  uint8_t* ptr = (uint8_t*)(&(BookWorm.nvm));
  server.sendContent("\n<p>\n");
  // spit out the NVM struct as a hex blob
  for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
    sprintf(str, "%02X", ptr[i]);
    server.sendContent(str);
  }
  server.sendContent("\n</p>\n");
}

void serveConfigNet()
{
  if (server.client().localIP() == apIP) {
    server.sendContent(String("\n<p>You are connected through the AP: ") + String(BookWorm.SSID) + String("</p>\n"));
  } else {
    server.sendContent("\n<p>Error, local IP does not match AP IP</p>\n");
  }
  server.sendContent(String("\n</p>IP: ") + String(WiFi.softAPIP()) + String("</p>\n"));
}

void serveConfigTable()
{
  server.sendContent(String("\n<table border='2' width='98%'>\n"));

  // void generateConfigItemTxt(const char* label, const char* id, const char* type, const char* value, const char* other, const char* note)
  generateConfigItemTxt("SSID", "ssid", "text", BookWorm.SSID, NULL, "* change requires reboot");
  generateConfigItemTxt("Drive no pulse on stop", "servostoppednopulse", "number", BookWorm.nvm.servoStoppedNoPulse ? "1" : "0", "min='0' max='1' step='1'", "0 = false, 1 = true");
  generateConfigItemTxt("Drive pulse range", "servomax", "number", String(BookWorm.nvm.servoMax).c_str(), "min='0' max='1000' step='50'", "range = 1500 +/- x microseconds");
  generateConfigItemTxt("Drive pulse deadzone left", "deadzoneleft", "number", String(BookWorm.nvm.servoDeadzoneLeft).c_str(), "min='0' max='500' step='1'", "microseconds");
  generateConfigItemTxt("Drive pulse deadzone right", "deadzoneright", "number", String(BookWorm.nvm.servoDeadzoneRight).c_str(), "min='0' max='500' step='1'", "microseconds");
  generateConfigItemTxt("Drive pulse offset left", "biasleft", "number", String(BookWorm.nvm.servoBiasLeft).c_str(), "min='0' max='500' step='1'", "microseconds");
  generateConfigItemTxt("Drive pulse offset right", "biasright", "number", String(BookWorm.nvm.servoBiasRight).c_str(), "min='0' max='500' step='1'", "microseconds");
  generateConfigItemTxt("Speed multiplier", "speedgain", "number", String(BookWorm.nvm.speedGain).c_str(), "min='100' max='10000' step='100'", "multiplier = x / 1000 , 1000 is normal");
  generateConfigItemTxt("Steering sensitivity", "steeringsensitivity", "number", String(BookWorm.nvm.steeringSensitivity).c_str(), "min='100' max='10000' step='100'", "sensitivity = x / 1000 , 1000 is normal");
  generateConfigItemTxt("Steering balance", "steeringbalance", "number", String(BookWorm.nvm.steeringBalance).c_str(), "min='100' max='10000' step='100'", "balance = x / 1000 , positive means apply to left, negative means apply to right");
  generateFlipDropdown(BookWorm.nvm.servoFlip);
  generateConfigItemTxt("Joystick size", "stickradius", "number", String(BookWorm.nvm.stickRadius).c_str(), "min='50' max='1000' step='10'", NULL);
  generateConfigItemTxt("Enabled advanced features?", "advanced", "number", BookWorm.nvm.advanced ? "1" : "0", "min='0' max='1' step='1'", "0 = false, 1 = true, true means enable weapon controls and inverted drive");
  if (BookWorm.nvm.advanced)
  {
    generateConfigItemTxt("Left handed?", "lefthanded", "number", BookWorm.nvm.leftHanded ? "1" : "0", "min='0' max='1' step='1'", "0 = false, 1 = true");
    generateConfigItemTxt("Weapon safe pulse", "weappossafe", "number", String(BookWorm.nvm.weapPosSafe).c_str(), "min='0' max='1500' step='100'", "0 = no need (servo weapon), otherwise use pulse width to stop ESC");
    generateConfigItemTxt("Weapon position \"A\"", "weapposa", "number", String(BookWorm.nvm.weapPosA).c_str(), "min='500' max='2500' step='100'", "pulse width in microseconds");
    generateConfigItemTxt("Weapon position \"B\"", "weapposb", "number", String(BookWorm.nvm.weapPosB).c_str(), "min='500' max='2500' step='100'", "pulse width in microseconds");
  }

  generateConfigItemStart("Factory reset", "factoryreset");
  server.sendContent("click the button to reset all settings to factory default values</td>");
  server.sendContent("</td><td class='tbl_submit'><input type='hidden' id='factoryreset' name='factoryreset' value='true' /><input type='submit' value='Go!' height='100%' /></td></tr></form>\n");

  generateConfigItemStart("Reboot", "reboot");
  server.sendContent("click the button to reboot the ESP8266</td>");
  server.sendContent("</td><td class='tbl_submit'><input type='hidden' id='reboot' name='reboot' value='true' /><input type='submit' value='REBOOT' height='100%' /></td></tr></form>\n");
  server.sendContent("</table>\n");
}

void generateFlipDropdown(uint8_t cur)
{
  uint8_t i;
  generateConfigItemStart("Signal flip", "servoflip");
  server.sendContent("\n\t<select id='servoflip' name='servoflip'>\n");
  for (i = 0; i <= 7; i++)
  {
    server.sendContent("\t\t<option value='");
    server.sendContent(String(i));
    server.sendContent("'");
    if (i == cur)
    {
      server.sendContent(" selected='selected' ");
    }
    server.sendContent(">");
    if ((i & 0x04) != 0) {
      server.sendContent("Signals ARE SWAPPED ; ");
    }
    else {
      server.sendContent("Signals not swapped ; ");
    }
    if ((i & 0x02) != 0) {
      server.sendContent("Left IS REVERSED ; ");
    }
    else {
      server.sendContent("Left is normal ; ");
    }
    if ((i & 0x02) != 0) {
      server.sendContent("Right IS REVERSED ; ");
    }
    else {
      server.sendContent("Right is normal ; ");
    }
    server.sendContent("</option>\n");
  }
  server.sendContent("\t</select><br />* requires reboot to take effect\n");
  generateConfigItemEnd();
}

void handleFingerPng() {
  handleFileRead("/finger.png");
  server.client().stop(); // stop() is needed if we sent no content length
}

void handleJoyJs() {
  handleFileRead("/joy.js");
  server.client().stop(); // stop() is needed if we sent no content length
}

void handleStyleCss() {
  handleFileRead("/style.css");
  server.client().stop(); // stop() is needed if we sent no content length
}

void handleConfigJs() {
  handleFileRead("/config.js");
  server.client().stop(); // stop() is needed if we sent no content length
}

void handleConfigCss() {
  handleFileRead("/config.css");
  server.client().stop(); // stop() is needed if we sent no content length
}
