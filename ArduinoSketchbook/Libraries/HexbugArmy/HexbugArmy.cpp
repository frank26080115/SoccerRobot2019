#include <HexbugArmy.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

#define IR_LED 4  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
#define MAX_HEXBUG_CNT 4
#define MAX_CMD_OFF_CNT 5
#define DEADBAND 8
#define DUTY_SLICE 4
#define MAX_STICK 100
#define CMD_TIMEOUT 1000

#define ID_SHIFT          0
#define BTNSHIFT_LEFT     7
#define BTNSHIFT_RIGHT    7
#define BTNSHIFT_UP       7
#define BTNSHIFT_DOWN     7
#define BTNSHIFT_TRIGGER  7

void cHexbugArmy::begin()
{
	this->begin(IR_LED, MAX_HEXBUG_CNT);
}

void cHexbugArmy::begin(uint8_t pin, uint8_t bugs)
{
	int i;

	irsend = new IRsend(pin);

	this->bugCnt = bugs;
	if (this->bugCnt < 1) {
		this->bugCnt = 1;
	}
	if (this->bugCnt > MAX_HEXBUG_CNT) {
		this->bugCnt = MAX_HEXBUG_CNT;
	}

	for (i = 0; i < this->bugCnt; i++)
	{
		hexbug[i] = new cHexbug();
		hexbug[i]->begin(i, irsend);
	}
}

void cHexbug::begin(uint8_t id, IRsend* ir)
{
	this->id = id % MAX_HEXBUG_CNT;
	this->cmdOffCnt = MAX_CMD_OFF_CNT;
	this->irsend = ir;
	this->cmdX = 0;
	this->cmdY = 0;
	this->cmdBtn = false;
}

void cHexbugArmy::sendIr()
{
	int i;
	for (i = 0; i < this->bugCnt; i++)
	{
		hexbug[i]->sendIr();
	}
}

void cHexbug::sendIr()
{
	uint32_t now = millis();
	uint32_t code = 0;

	if ((now - cmdMillis) > CMD_TIMEOUT) {
		this->cmdX = 0;
		this->cmdY = 0;
		this->cmdBtn = false;
	}

	if ((this->cmdX < DEADBAND && this->cmdX > -DEADBAND) && (this->cmdY < DEADBAND && this->cmdY > -DEADBAND) && this->cmdBtn == false)
	{
		if (this->cmdOffCnt >= MAX_CMD_OFF_CNT) {
			// send nothing!
			return;
		}
		this->cmdOffCnt++;
		sendIrOff();
		return;
	}

	this->cmdOffCnt = 0;

	this->sendCnt++;
	if (this->sendCnt > DUTY_SLICE) {
		this->sendCnt = 0;
	}

	int absX = cmdX > 0 ? cmdX : -cmdX;
	int absY = cmdY > 0 ? cmdY : -cmdY;

	int j = 0, k = 0;

	#define DUTY_CHUNK ((MAX_STICK - (DEADBAND - (DUTY_SLICE / 2))) / DUTY_SLICE)

	for (int i = 0; i < DUTY_SLICE; i++)
	{
		if (absX >= (MAX_STICK - ((i + 1) * DUTY_CHUNK))) {
			j = DUTY_SLICE - i;
			break;
		}
	}
	for (int i = 0; i < DUTY_SLICE; i++)
	{
		if (absY >= (MAX_STICK - ((i + 1) * DUTY_CHUNK))) {
			k = DUTY_SLICE - i;
			break;
		}
	}

	if (j >= sendCnt)
	{
		code |= (1 << (cmdX < 0 ? BTNSHIFT_LEFT : BTNSHIFT_RIGHT));
	}
	else if (k >= sendCnt)
	{
		code |= (1 << (cmdY < 0 ? BTNSHIFT_DOWN : BTNSHIFT_UP));
	}

	if (cmdBtn) {
		code |= (1 << BTNSHIFT_TRIGGER);
	}

	if (code == 0) {
		sendIrOff();
		return;
	}

	code |= id;

	this->irsend->sendHexbug(code, 8, 0);
}

void cHexbug::sendIrOff()
{
	uint32_t code = !id;
	code &= (MAX_HEXBUG_CNT * 2) - 1;
	this->irsend->sendHexbug(code, 8, 0);
}

void cHexbug::command(int x, int y, bool btn)
{
	this->cmdX = x;
	this->cmdY = y;
	this->cmdBtn = btn;
	this->cmdMillis = millis();
}

void cHexbug::command(hexbug_cmd_t* cmd)
{
	this->command(cmd->x, cmd->y, cmd->btn);
}

void cHexbugArmy::command(uint8_t idx, hexbug_cmd_t* cmd)
{
	this->hexbug[idx % this->bugCnt]->command(cmd->x, cmd->y, cmd->btn);
}

void cHexbug::setBugId(uint8_t x)
{
	this->id = x % MAX_HEXBUG_CNT;
}

void cHexbugArmy::setBugId(uint8_t idx, uint8_t id)
{
	this->hexbug[idx]->setBugId(id);
}

bool cHexbugArmy::handleKey(char c)
{
	hexbug_cmd_t cmd;
	cmd.x = 0; cmd.y = 0; cmd.btn = false;
	uint8_t id;
	bool ret = false;
	if (c >= '0' && c <= '9') {
		id = 0xFF;
	}
	else if (c == 'w' || c == 'a' || c == 's' || c == 'd' || c == 'x') {
		id = 0;
	}
	else if (c == 'W' || c == 'A' || c == 'S' || c == 'D' || c == 'X') {
		id = 1;
	}
	else if (c == 'i' || c == 'j' || c == 'k' || c == 'l' || c == ',') {
		id = 2;
	}
	else if (c == 'I' || c == 'J' || c == 'K' || c == 'L' || c == '<') {
		id = 3;
	}
	switch (c)
	{
		case '8':
		case 'w':
		case 'W':
		case 'i':
		case 'I':
			cmd.y = MAX_STICK;
			ret = true;
			break;
		case '2':
		case 's':
		case 'S':
		case 'k':
		case 'K':
			cmd.y = -MAX_STICK;
			ret = true;
			break;
		case '6':
		case 'd':
		case 'D':
		case 'l':
		case 'L':
			cmd.x = MAX_STICK;
			ret = true;
			break;
		case '4':
		case 'a':
		case 'A':
		case 'j':
		case 'J':
			cmd.x = -MAX_STICK;
			ret = true;
			break;
		case '5':
		case 'x':
		case 'X':
		case ',':
		case '<':
			cmd.btn = true;
			ret = true;
			break;
		case '7':
			cmd.x = -MAX_STICK;
			cmd.y =  MAX_STICK;
			ret = true;
			break;
		case '9':
			cmd.x =  MAX_STICK;
			cmd.y =  MAX_STICK;
			ret = true;
			break;
		case '1':
			cmd.x = -MAX_STICK;
			cmd.y = -MAX_STICK;
			ret = true;
			break;
		case '3':
			cmd.x =  MAX_STICK;
			cmd.y = -MAX_STICK;
			ret = true;
			break;
	}

	if (ret)
	{
		if (id == 0xFF)
		{
			int i;
			for (i = 0; i < this->bugCnt; i++)
			{
				this->command(i, &cmd);
			}
		}
		else
		{
			this->command(id, &cmd);
		}
	}

	return ret;
}