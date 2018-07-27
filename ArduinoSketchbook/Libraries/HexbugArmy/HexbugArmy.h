#ifndef _HEXBUGARMY_H_
#define _HEXBUGARMY_H_

#include <stdint.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

typedef struct
{
	int8_t x;
	int8_t y;
	bool btn;
}
hexbug_cmd_t;

class cHexbug
{
	public:
		void begin(uint8_t id, IRsend* ir);
		void command(int8_t x, int8_t y, bool btn);
		void command(hexbug_cmd_t*);
		void sendIr();
		void setBugId(uint8_t);
	private:
		void sendIrOff();
		uint8_t calcIdOn();
		uint8_t calcIdOff();
		uint8_t id;
		uint8_t sendCnt;
		int cmdX;
		int cmdY;
		bool cmdBtn;
		uint32_t cmdMillis;
		uint8_t cmdOffCnt;
		IRsend* irsend;
};

class cHexbugArmy
{
	public:
		void begin();
		void begin(uint8_t pin, uint8_t bugs);
		void command(uint8_t idx, hexbug_cmd_t* cmd);
		void sendIr();
		void sendIrNext();
		void setBugId(uint8_t idx, uint8_t id);
		bool handleKey(char);
		bool handleKey(char c, int8_t spd);
		void printStats();
	private:
		cHexbug* hexbug[4];
		IRsend* irsend;
		uint8_t bugCnt;
};

#endif
