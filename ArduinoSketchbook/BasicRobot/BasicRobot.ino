#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <FS.h>
#include <BookWorm.h>

#if defined(HWBOARD_ESP12_NANO) || defined(HWBOARD_ESP12)
///#define ENABLE_BOOT_PIN_RESET
#endif

#define SIMULATED_BATT_READING

/* hostname for mDNS. Should work at least on windows. Try http://robot.local */
const char *myHostname = "robot";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Current WLAN status */
unsigned int wlan_status = WL_IDLE_STATUS;

/** Servo Movement Variables **/
uint32_t lastCommTimestamp = 0;
bool moveMixedMode = false;
signed int speedLeft = 0;
signed int speedRight = 0;
signed int speedX = 0;
signed int speedY = 0;
volatile bool standbyRobot = false;
#ifdef ENABLE_WEAPON
signed int speedWeap = 0;
#endif
#ifdef ENABLE_BATTERY_MONITOR
uint16_t battLvl = 0;
uint32_t battTime = 0;
#endif
#ifdef ENABLE_BOOT_PIN_RESET
#define BOOT_PIN_RESET_LIMIT 5000
uint32_t bootPinLowCnt = 0;
#endif
#ifdef ENABLE_NOCONN_TIMEOUT_RESET
uint32_t noconnTime = 0;
#endif

#ifdef ENABLE_SERVO_DEBUG
uint32_t last_servo_dbg = 0;
#endif

void setup()
{
  delay(1000);
  BookWorm.begin();
  SPIFFS.begin();
  BookWorm.printf("\r\n");
  if (SPIFFS.exists("joy.js") == false && SPIFFS.exists("/joy.js") == false) {
    BookWorm.printf("SPI FLASH FILESYSTEM ERROR: FILE \"joy.js\" MISSING\r\n");
  }
  BookWorm.printf("Configuring access point...\r\n");
  /* You can remove the password parameter if you want the AP to be open. */
  if (WiFi.softAPConfig(apIP, apIP, netMsk) == false) {
    BookWorm.printf("SoftAPConfig Failed!\r\n");
  }
  BookWorm.printf("SSID: %s\r\n", BookWorm.SSID);
  BookWorm.printf("Password: %s\r\n", BookWorm.wifiPassword);
  #ifdef ENABLE_CONFIG_WIFICHANNEL
  BookWorm.printf("WiFi Channel: %u\r\n", BookWorm.nvm->wifiChannel);
  #endif
  if (WiFi.softAP(BookWorm.SSID, BookWorm.wifiPassword
      #ifdef ENABLE_CONFIG_WIFICHANNEL
      , BookWorm.nvm->wifiChannel, 0, 2
      #endif
      ) == false)
      {
        BookWorm.printf("SoftAP Failed to Start!\r\n");
      }
  delay(500); // Without delay I've seen the IP address blank
  BookWorm.printf("AP IP address: %s\r\n", toStringIp(WiFi.softAPIP()).c_str());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages */
  /* these are the ones that lead to the control page */
  server.on("/",             handleRoot);
  server.on("/index.htm",    handleRoot);
  server.on("/index.html",   handleRoot);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handleZ
  server.on("/generate204",  handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink",       handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/connecttest.txt", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.

  /* handlers for the XHR and configuration */
  server.on("/move",      handleMove);
  server.on("/config",    handleConfig);

  /* handlers for files inside the SPIFFS */
  server.onNotFound( []()
  {
    BookWorm.debugf("call handleNotFound\r\n");
    if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
      return;
    }
    server.sendHeader("Connection", "close");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
    serverClientStop();
  });
  
  server.begin(); // Web server start
  BookWorm.printf("HTTP server started\r\n");
  BookWorm.debugf("Free RAM %u\r\n", system_get_free_heap_size());

  #ifdef ENABLE_BOOT_PIN_RESET
  pinMode(pinBoot, INPUT_PULLUP);
  #endif
}

void loop()
{
  uint32_t now;

  /* this chunk of code is for WiFi client, not really needed since we are an AP, not a client */
  {
    unsigned int s = WiFi.status();
    if (wlan_status != s) { // WLAN status change
      BookWorm.printf("WiFi Status: 0x%02X\r\n", s);
      wlan_status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        BookWorm.printf("IP addr: %s\r\n", String(WiFi.localIP()).c_str());

        // Setup MDNS responder
        if (!MDNS.begin(myHostname)) {
          BookWorm.printf("Error setting up MDNS responder!\r\n");
        } else {
          BookWorm.printf("mDNS responder started\r\n");
          // Add service to MDNS-SD
          MDNS.addService("http", "tcp", 80);
        }
      }
    }
  }

  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();

  // Move the robot if needed
  now = millis();

  int now10 = now / 100;
  now10 %= 10;
  bool ledOn = false;
  bool isTimeOut = (now - lastCommTimestamp) > 1000;

  if (isTimeOut || standbyRobot) // check for timeout
  {
    // if timeout, stop the robot
    speedLeft = 0;
    speedRight = 0;
    speedX = 0;
    speedY = 0;

    #ifdef ENABLE_WEAPON
    if (BookWorm.nvm->weapPosSafe > 0) {
      speedWeap = BookWorm.nvm->weapPosSafe;
    }
    else {
      speedWeap = 0;
    }
    #endif

    if (now10 == 0) { // blink at least once
      ledOn = true;
    }

    /*
    if (isTimeOut) {
      standbyRobot = false;
    }
    */
  }
  else
  {
    // blink frequently if moving
    if (now10 == 0 || now10 == 2 || now10 == 4
      || (((moveMixedMode != false && (speedX != 0 || speedY != 0)) || (moveMixedMode == false && (speedLeft != 0 || speedRight != 0))) && (now10 == 6 || now10 == 8))
      ) {
      ledOn = true;
    }
  }

  // blink twice if at least something is connected
  if (lastCommTimestamp > 0 && now10 == 2)
  {
    ledOn = true;
  }
  else
  {
    // blank
  }

  if (moveMixedMode == false)
  {
    BookWorm.move(speedLeft, speedRight);
  }
  else
  {
    BookWorm.moveMixed(speedY, speedX);
  }

  // spin/move the weapon if needed
  #ifdef ENABLE_WEAPON
  if (BookWorm.nvm->advanced && BookWorm.nvm->enableWeapon) {
    BookWorm.spinWeapon(speedWeap);
  }
  #endif

  // blink the LED
  if (ledOn && standbyRobot == false) {
    BookWorm.setLedOn();
  }
  else {
    BookWorm.setLedOff();
  }

  #ifdef ENABLE_BATTERY_MONITOR
  #ifndef SIMULATED_BATT_READING
  if (BookWorm.nvm->vdiv_r2 > 0 && BookWorm.calcMaxBattVoltage() > 0 && lastCommTimestamp > 0)
  {
    if ((BookWorm.nvm->vdiv_filter > 0 && BookWorm.nvm->vdiv_filter < 1000) || (now - battTime) > 1000) {
      battLvl = BookWorm.readBatteryVoltageFiltered();
      battTime = now;
    }
  }
  #endif
  #endif

  #ifdef ENABLE_BOOT_PIN_RESET
  if (digitalRead(pinBoot) == LOW)
  {
    bootPinLowCnt++;
    if (bootPinLowCnt > BOOT_PIN_RESET_LIMIT) {
      BookWorm.printf("Boot Pin Factory Reset");
      BookWorm.factoryReset();
      BookWorm.saveNvm();
      ESP.reset();
    }
  }
  else
  {
    bootPinLowCnt = 0;
  }
  #endif

  #ifdef ENABLE_NOCONN_TIMEOUT_RESET
  #error TODO not implemented yet
  if (now > noconnTime)
  {
    if ((now - noconnTime) > (60000 * 5))
    {
      char tmpbuf[BOOKWORM_SSID_SIZE + 1];
      BookWorm.generateSsid(tmpbuf)
      WiFi.softAP(tmpbuf, BOOKWORM_DEFAULT_PASSWORD);
    }
  }
  #endif

  #ifdef ENABLE_SERVO_DEBUG
  if ((now - last_servo_dbg) > 500) {
    BookWorm.printf("Tr=%d , ", BookWorm.dbgDriveY);
    BookWorm.printf("St=%d , ", BookWorm.dbgDriveX);
    BookWorm.printf("L[%u]=%u , ", BookWorm.dbgLeftPin, BookWorm.dbgLeftTicks);
    BookWorm.printf("R[%u]=%u , ", BookWorm.dbgRightPin, BookWorm.dbgRightTicks);
    #ifdef ENABLE_WEAPON
    BookWorm.printf("W=%u , ", BookWorm.dbgWeaponTicks);
    #endif
    BookWorm.printf("\r\n");
    last_servo_dbg = now;
  }
  #endif
}
