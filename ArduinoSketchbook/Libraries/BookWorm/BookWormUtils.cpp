#include "BookWorm.h"
#include <ESP8266WebServer.h>

void cBookWorm::checkHardwareConfig(void* serverPtr, bool useSerial)
{
	ESP8266WebServer* server = (ESP8266WebServer*)serverPtr;
	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();
	char tmpbuff[128];

	sprintf(tmpbuff, "Flash Actual ID:   %08X\r\n", ESP.getFlashChipId());
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }
	sprintf(tmpbuff, "Flash Actual Size: %u bytes\r\n", realSize);
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }

	sprintf(tmpbuff, "Flash Config'ed Size:  %u bytes\r\n", ideSize);
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }
	sprintf(tmpbuff, "Flash Config'ed Speed: %u Hz\r\n", ESP.getFlashChipSpeed());
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }
	sprintf(tmpbuff, "Flash Config'ed Mode:  %s\r\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }

	if (ideSize != realSize) {
		sprintf(tmpbuff, "Actual Config Does Not Match\r\n");
		if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }
	}

	#ifdef HWBOARD_ESP01
	sprintf(tmpbuff, "PCB: ESP-01\r\n");
	#elif defined(HWBOARD_ESP12)
	sprintf(tmpbuff, "PCB: ESP-12\r\n");
	#elif defined(HWBOARD_ESP12_NANO)
	sprintf(tmpbuff, "PCB: ESP-12 Nano\r\n");
	#endif
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }

	sprintf(tmpbuff, "Free Heap Mem: %ul bytes\r\n", ESP.getFreeHeap());
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }

	sprintf(tmpbuff, "Compile Date: " __DATE__ " ; Time: " __TIME__ "\r\n");
	if (server != NULL)  { server->sendContent(tmpbuff); server->sendContent("<br />"); } if (useSerial) { Serial.print(tmpbuff); }
}

/*
Delays a number of milliseconds, while feeding the watchdog timer
Insanely inaccurate

return: none
parameters: number of milliseconds to delay
*/
void cBookWorm::delayWhileFeeding(int x)
{
	int i;
	for (i = 0; i < (x - 10); i += 10)
	{
		delay(10);
		if ((i % 200) == 0) {
			ESP.wdtFeed();
		}
	}
	for (; i < x; i += 1)
	{
		delay(1);
		ESP.wdtFeed();
	}
}
