#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
#include <BookWorm.h>

/*
   This example serves a "hello world" on a WLAN and a SoftAP at the same time.
   The SoftAP allow you to configure WLAN parameters at run time. They are not setup in the sketch but saved on EEPROM.

   Connect your computer or cell phone to wifi network ESP_ap with password 12345678. A popup may appear and it allow you to go to WLAN config. If it does not then navigate to http://192.168.4.1/wifi and config it there.
   Then wait for the module to connect to your wifi and take note of the WLAN IP it got. Then you can disconnect from ESP_ap and return to your regular WLAN.

   Now the ESP8266 is in your network. You can reach it through http://192.168.x.x/ (the IP you took note of) or maybe at http://esp8266.local too.

   This is a captive portal because through the softAP it will redirect any http request to http://192.168.4.1/
*/

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
unsigned int status = WL_IDLE_STATUS;

/** Continuous Servo Movement Variables **/
uint32_t lastCommTimestamp = 0;
signed int speedLeft = 0;
signed int speedRight = 0;

void setup() {
  delay(1000);
  BookWorm.begin();
  BookWorm.printf("\r\n");
  BookWorm.printf("Configuring access point...\r\n");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(BookWorm.SSID, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  BookWorm.printf("AP IP address: %s\r\n", String(WiFi.softAPIP()).c_str());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages */
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.

  server.on("/move", handleMove);
  server.on("/setssid", handleSetSSID);
  server.on("/setservo", handleSetServo);
  server.on("/info", handleInfo);
  server.on("/config.html", handleConfig);

  server.on("/background.png", handleBgImage);

  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
  BookWorm.printf("HTTP server started\r\n");
}

void loop()
{
  uint32_t now;
  {
    unsigned int s = WiFi.status();
    if (status != s) { // WLAN status change
      BookWorm.printf("WiFi Status: 0x%02X\r\n", s);
      status = s;
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
  if ((now - lastCommTimestamp) > 1000)
  {
    // if timeout, stop the robot
    speedLeft = 0;
    speedRight = 0;
  }
  BookWorm.move(speedLeft, speedRight);
}

