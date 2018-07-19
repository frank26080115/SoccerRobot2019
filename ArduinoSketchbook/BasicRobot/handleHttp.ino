/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  handleFileRead("/index.html");
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
  for (i = 0; i < server.args(); i++)
  {
    String argName = server.argName(i);
    signed int v;
    if (argName.length() <= 0) {
      continue;
    }
    if (argName.equalsIgnoreCase(String("left")))
    {
      if (readServerArg(i, &v)) {
        speedLeft = v;
        gotLeft = true;
        //lastCommTimestamp = millis();
      }
    }
    else if (argName.equalsIgnoreCase(String("right")))
    {
      if (readServerArg(i, &v)) {
        speedRight = v;
        gotRight = true;
        //lastCommTimestamp = millis();
      }
    }
  }
  if (gotLeft && gotRight)
  {
    lastCommTimestamp = millis();
  }
}

void handleSetSSID() {
  String str = server.arg("ssid");
  BookWorm.setSsid((char*)str.c_str());
}

void handleSetServo() {
  int i;
  bool commit = false;
  for (i = 0; i < server.args(); i++)
  {
    String argName = server.argName(i);
    signed int v;
    if (argName.length() <= 0) {
      continue;
    }
    if (argName.equalsIgnoreCase(String("deadzoneleft")))
    {
      if (readServerArg(i, &v)) {
        BookWorm.setServoDeadzoneLeft(v);
        commit = true;
      }
    }
    else if (argName.equalsIgnoreCase(String("deadzoneright")))
    {
      if (readServerArg(i, &v)) {
        BookWorm.setServoDeadzoneRight(v);
        commit = true;
      }
    }
    else if (argName.equalsIgnoreCase(String("biasleft")))
    {
      if (readServerArg(i, &v)) {
        BookWorm.setServoBiasLeft(v);
        commit = true;
      }
    }
    else if (argName.equalsIgnoreCase(String("biasright")))
    {
      if (readServerArg(i, &v)) {
        BookWorm.setServoBiasRight(v);
        commit = true;
      }
    }
  }
  if (commit)
  {
    BookWorm.saveNvm();
  }
}

void handleBgImage() {
  handleFileRead("/background.png");
  server.client().stop(); // stop() is needed if we sent no content length
}

void handleInfo() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html>\n<head>\n<title>Robot Config Info</title>\n</head>\n<body>\n"
    "<h1>Robot Config Info</h1>\n"
  );
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the AP: ") + String(BookWorm.SSID) + "</p>\n");
  } else {
    server.sendContent(String("<p>Error, local IP does not match AP IP</p>\n"));
  }
  server.sendContent(String("<p>IP: ") + String(WiFi.softAPIP()) + "</p>\n");
  server.sendContent(String("<hr />\n"));
  server.sendContent(String("<p>Servo Deadzone Left = ") + String(BookWorm.nvm.servoDeadzoneLeft) + "</p>\n");
  server.sendContent(String("<p>Servo Deadzone Right = ") + String(BookWorm.nvm.servoDeadzoneRight) + "</p>\n");
  server.sendContent(String("<p>Servo Bias Left = ") + String(BookWorm.nvm.servoBiasLeft) + "</p>\n");
  server.sendContent(String("<p>Servo Bias Right = ") + String(BookWorm.nvm.servoBiasRight) + "</p>\n");
  
  server.sendContent(
    "</body>\n</html>\n"
  );
  server.client().stop(); // Stop is needed because we sent no content length
}

void handleConfig() {
  handleFileRead("/config.html");
  server.client().stop(); // stop() is needed if we sent no content length
}

