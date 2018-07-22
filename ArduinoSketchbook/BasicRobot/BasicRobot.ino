#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
#include <BookWorm.h>

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_password = "12345678";

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

/** Continuous Servo Movement Variables **/
uint32_t lastCommTimestamp = 0;
bool moveMixedMode = false;
signed int speedLeft = 0;
signed int speedRight = 0;
signed int speedX = 0;
signed int speedY = 0;
#ifdef ENABLE_WEAPON
signed int speedWeap = 0;
#endif
char ledMode = 0;

void setup() {
  delay(1000);
  BookWorm.begin();
  BookWorm.printf("\r\n");
  BookWorm.printf("Configuring access point...\r\n");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(BookWorm.SSID, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  BookWorm.printf("AP IP address: %s\r\n", toStringIp(WiFi.softAPIP()).c_str());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages */
  server.on("/",             handleRoot);
  server.on("/index.htm",    handleRoot);
  server.on("/index.html",   handleRoot);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink",       handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.

  server.on("/move",      handleMove);
  server.on("/setconfig", handleSetConfig);
  server.on("/config",    handleConfig);

  server.on("/finger.png", handleFingerPng);
  server.on("/style.css",  handleStyleCss);
  server.on("/config.css", handleConfigCss);
  server.on("/joy.js",     handleJoyJs);
  server.on("/config.js",  handleConfigJs);

  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
  BookWorm.printf("HTTP server started\r\n");
  BookWorm.debugf("Free RAM %u\r\n", system_get_free_heap_size());
}

void loop()
{
  uint32_t now;
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
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
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

  if ((now - lastCommTimestamp) > 1000)
  {
    // if timeout, stop the robot
    speedLeft = 0;
    speedRight = 0;
    speedX = 0;
    speedY = 0;

    #ifdef ENABLE_WEAPON
    if (BookWorm.nvm.weapPosSafe > 0) {
      speedWeap = BookWorm.nvm.weapPosSafe;
    }
    else {
      speedWeap = 0;
    }
    #endif

    if (now10 == 0) {
      ledOn = true;
    }
  }
  else {
    if (now10 == 0 || now10 == 2 || now10 == 4
      || (((moveMixedMode != false && (speedX != 0 || speedY != 0)) || (moveMixedMode == false && (speedLeft != 0 || speedRight != 0))) && (now10 == 6 || now10 == 8))
      ) {
      ledOn = true;
    }
  }
  if (lastCommTimestamp > 0 && now10 == 2)
  {
    ledOn = true;
  }

  if (moveMixedMode == false)
  {
    BookWorm.move(speedLeft, speedRight);
  }
  else
  {
    BookWorm.moveMixed(speedY, speedX);
  }
  #ifdef ENABLE_WEAPON
  if (BookWorm.nvm.advanced && BookWorm.nvm.enableWeapon) {
    BookWorm.spinWeapon(speedWeap);
  }
  #endif

  if (ledOn) {
    BookWorm.setLedOn();
  }
  else {
    BookWorm.setLedOff();
  }
}

