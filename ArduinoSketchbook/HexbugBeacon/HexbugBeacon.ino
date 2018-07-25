#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <HexbugArmy.h>

#define ENABLE_TCP_MODE
#define ENABLE_HTTP_MODE
#define ENABLE_KEYBOARD_MODE
//#define ENABLE_SERIAL_MODE

cHexbugArmy HexbugArmy;

const char* ssid     = "........";
const char* password = "........";

ESP8266WebServer webserver(80);
WiFiServer tcpserver(5045);
WiFiClient tcpclient;

#define IR_INTERVAL 1
uint32_t now;
uint32_t lastIrTime = 0;

#ifdef ENABLE_SERIAL_MODE
uint8_t serbuff[(4 * 4) + 2];
uint8_t seridx = 0;
uint32_t sertime = 0;
#endif

void setup()
{
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

#ifdef ENABLE_TCP_MODE
  tcpserver.begin();
#endif

#ifdef ENABLE_HTTP_MODE
  webserver.on("/", handleRoot);
  webserver.onNotFound(handleRoot);
  webserver.begin();
#endif

  HexbugArmy.begin();

  Serial.println("All init done");
}

void loop()
{
  now = millis();

#ifdef ENABLE_HTTP_MODE
  webserver.handleClient();
#endif

#ifdef ENABLE_TCP_MODE
  tcpclient = tcpserver.available();
  if (tcpclient)
  {
    if (tcpclient.connected())
    {
      int avail = tcpclient.available();
      if (avail > 0)
      {
        uint8_t* buff = (uint8_t*)malloc(avail);

        tcpclient.read(buff, (size_t )avail);

        handlePacket(buff, avail);

        free(buff);
      }
    }
  }
#endif

#ifdef ENABLE_KEYBOARD_MODE
  while (Serial.available() > 0) {
    char c = Serial.read();
    HexbugArmy.handleKey(c);
  }
#elif defined(ENABLE_SERIAL_MODE)
  if ((sertime - now) > 500) {
    seridx = 0;
  }
  while (Serial.available() > 0) {
    char c = Serial.read();
    sertime = now = millis();
    if (seridx == 0 && c != 0xAA) {
      seridx = 0;
      continue;
    }
    if (seridx == 1 && c != 0x55) {
      seridx = 0;
      if (c != 0xAA) {
        continue;
      }
    }
    serbuff[seridx] = c;
    seridx++;
    if (seridx >= ((4 * 4) + 2)) {
      handlePacket(&(serbuff[2]), seridx);
      seridx = 0;
    }
  }
#endif

  if ((now - lastIrTime) > IR_INTERVAL)
  {
    lastIrTime = now;
    HexbugArmy.sendIr();
  }
}

void handleRoot()
{
  int i, j;
  uint8_t filled[4];
  hexbug_cmd_t cmds[4];
  #ifndef ENABLE_HTTP_MODE
  return;
  #endif

  for (i = 0; i < webserver.args(); i++)
  {
    String argName = webserver.argName(i);
    signed int v;
    if (argName.length() <= 0) {
      continue;
    }
    for (j = 0; j < 4; j++)
    {
      if (argName.equalsIgnoreCase(String("x") + String(j)))
      {
        if (readServerArg(i, &v)) {
          filled[j] |= (1 << 0);
          cmds[j].x = v;
        }
      }
      else if (argName.equalsIgnoreCase(String("y") + String(j)))
      {
        if (readServerArg(i, &v)) {
          filled[j] |= (1 << 1);
          cmds[j].y = v;
        }
      }
      else if (argName.equalsIgnoreCase(String("b") + String(j)))
      {
        if (readServerArg(i, &v)) {
          filled[j] |= (1 << 2);
          cmds[j].btn = (bool)v;
        }
      }
    }
  }

  for (j = 0; j < 4; j++)
  {
    if (filled[j] == 0x07) {
      HexbugArmy.command(j, &(cmds[j]));
    }
  }

  webserver.send(200, "text/plain", "done");
}

void handlePacket(uint8_t* buff, int avail)
{
  uint8_t id;
  hexbug_cmd_t cmd;
  for (int i = 0; i < avail; i++)
  {
    int j = i % 4;
    uint8_t d = buff[i];
    if (j == 0) {
      id = d;
    }
    else if (j == 1) {
      cmd.x = d;
    }
    else if (j == 2) {
      cmd.y = d;
    }
    else if (j == 3) {
      cmd.btn = (bool)d;
    }

    if (j == 3) {
      HexbugArmy.command(id, &cmd);
    }
  }
}

bool readServerArg(int argNum, signed int* result)
{
  String argVal = webserver.arg(argNum);
  char* str = (char*)argVal.c_str();
  char* end;
  double dv = strtod(str, &end);
  if (str == end)
  {
    return false;
  }
  signed int v = lround(dv);
  *result = v;
  return true;
}
