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

const char* ssid     = "RobotArmy_2.4";
const char* password = "robotsoccer";

ESP8266WebServer webserver(80);
WiFiServer tcpserver(3333);
WiFiClient tcpclient;

#define IR_INTERVAL 1
uint32_t now;
uint32_t lastIrTime = 0;

#ifdef ENABLE_SERIAL_MODE
uint8_t serbuff[(4 * 4) + 2];
uint8_t seridx = 0;
uint32_t sertime = 0;
#endif

uint32_t packet_cnt = 0;
uint32_t packet_cntPrev = 0;
uint32_t packet_rptTime = 0;
bool     packet_canPrint = true;

void setup()
{
  Serial.begin(115200);
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
  static bool tcpavailable = false;
  static bool tcpconnected = false;
  if (tcpconnected == false || tcpclient.connected() == false) {
    tcpclient = tcpserver.available();
  }
  if (tcpclient)
  {
    if (tcpavailable == false) {
      Serial.println("TCP available");
    }
    tcpavailable = true;
    
    if (tcpclient.connected())
    {
      if (tcpconnected == false) {
        Serial.println("TCP connected");
      }
      tcpconnected = true;
      int avail = tcpclient.available();
      if (avail > 0)
      {
        uint8_t* buff = (uint8_t*)malloc(avail);

        tcpclient.read(buff, (size_t )avail);

        packet_cnt += avail;

        handlePacket(buff, avail);

        free(buff);
      }
    }
    else
    {
      if (tcpconnected) {
        Serial.println("TCP disconnected");
      }
      tcpconnected = false;
    }
  }
  else {
    if (tcpavailable) {
      Serial.println("TCP unavailable");
    }
    tcpavailable = false;
  }
#endif

#ifdef ENABLE_KEYBOARD_MODE
  while (Serial.available() > 0) {
    char c = Serial.read();
    packet_cnt++;
    HexbugArmy.handleKey(c);
  }
#elif defined(ENABLE_SERIAL_MODE)
  if ((sertime - now) > 500) {
    seridx = 0;
  }
  while (Serial.available() > 0) {
    char c = Serial.read();
    packet_cnt++;
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

  if ((now - packet_rptTime) > 1000) {
    packet_rptTime = now;
    packet_canPrint = true;
    if (packet_cntPrev != packet_cnt) {
      packet_cntPrev = packet_cnt;
      Serial.print("pktcnt: ");
      Serial.println(packet_cnt, DEC);
    }
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
    packet_cnt++;
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
    if (packet_canPrint) {
      Serial.print("HTTP "); Serial.print(j, DEC); Serial.print(" "), Serial.print(cmds[j].x, DEC); Serial.print(" "), Serial.print(cmds[j].y, DEC); Serial.print(" "), Serial.println(cmds[j].btn);
    }
  }
  packet_canPrint = false;

  webserver.send(200, "text/plain", "done");
}

void handlePacket(uint8_t* buff, int avail)
{
  /*
   * packet format
   * 
   * ID, X, Y, B, ID, X, Y, B, ID, X, Y, B, ID, X, Y, B,
   * 
   * all fields are one byte
   * ID is 0, 1, 2, or 3
   * X and Y are signed 8 bit integers, -127 to 127, 0 means stopped
   * negative means left and down, positive means up and right
   * B means button, the button on the shoulder, true = 1, false = 0
   */
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

    if (j == 3 && packet_canPrint) {
      Serial.print("TCP "); Serial.print(id, DEC); Serial.print(" "), Serial.print(cmd.x, DEC); Serial.print(" "), Serial.print(cmd.y, DEC); Serial.print(" "), Serial.println(cmd.btn);
    }
  }
  packet_canPrint = false;
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
