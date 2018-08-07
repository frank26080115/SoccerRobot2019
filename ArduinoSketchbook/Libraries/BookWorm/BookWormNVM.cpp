#include "BookWorm.h"
#include <EEPROM.h>
#include "checksum.h"

/*
Loads a string of hexadecimal characters into NVM structure
Validates checksum
Automatically detects length to see if it contains WiFi info or not
Optional commit into NVM

returns:
	boolean, true on success
	errCode is a pointer to error Code
			1 means wrong length
			2 means illegal char detected
			3 means checksum failed

parameters:
	str, a string of hexadecimal to process
	save, automatically save (commit) to NVM if successful
*/
bool cBookWorm::loadNvmHex(char* str, uint8_t* errCode, bool save)
{
	uint8_t* tmpbuff = (uint8_t*)malloc(sizeof(bookworm_nvm_t));
	bookworm_nvm_t* castPtr = (bookworm_nvm_t*)tmpbuff;

	int slen = strlen(str);
	uint16_t chksum;
	uint32_t userLength = calcUserNvmLength(false);
	uint32_t userLength2 = calcUserNvmLength(true);
	int i, j, k, accum;
	for (i = 0, j = 0, k = 0; i < slen, j < sizeof(bookworm_nvm_t); i++)
	{
		if ((k % 2) == 0) {
			accum = 0;
		}
		else {
			accum <<= 8;
		}

		char c = str[i];
		if (c >= '0' && c <= '9') {
			accum += c - '0';
			k++;
		}
		else if (c >= 'a' && c <= 'f') {
			accum += c - 'a' + 0xA;
			k++;
		}
		else if (c >= 'A' && c <= 'F') {
			accum += c - 'A' + 0xA;
			k++;
		}
		else if ((c == ' ' || c == '\t' || c == '\n' || c == '\r') && (k % 2) == 0) {
			accum = 0;
			k = k;
		}
		else {
			if (errCode != NULL) { *errCode = 2; /* valid chars failed */ }
			free(tmpbuff); return false;
		}

		if ((k % 2) == 1) {
			tmpbuff[j] = accum;
			j++;
		}
	}
	if (j == sizeof(bookworm_nvm_t))
	{
		// wifi + user blocks
		chksum = bookworm_fletcher16(tmpbuff, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
		if (chksum != castPtr->checksum) {
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		memcpy((void*)(&(this->nvm)), (const void*)tmpbuff, sizeof(bookworm_nvm_t));
		free(tmpbuff);
		if (save) {
			saveNvm();
		}
		return true;
	}
	else if (j == userLength2)
	{
		// user block only
		uint16_t* chksum2;
		chksum = bookworm_fletcher16(tmpbuff, userLength);
		chksum2 = (uint16_t*)(&tmpbuff[userLength + sizeof(uint8_t)]);
		if (chksum != (*chksum2)) {
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		memcpy(&(this->nvm.divider1), tmpbuff, userLength);
		free(tmpbuff);
		if (save) {
			saveNvm();
		}
		return true;
	}
	else
	{
		if (errCode != NULL) { *errCode = 1; /* length check failed */ }
		free(tmpbuff); return false;
	}
}

/*
Calculates the size of the NVM block that does not contain WiFi data

return: size in bytes
parameter: boolean, whether or not to include the checksum in the size calculation
*/
uint32_t cBookWorm::calcUserNvmLength(bool withChecksum)
{
	uint8_t* ptrUser = (uint8_t*)&(this->nvm.divider1);
	uint8_t* ptrUserEnd = (uint8_t*)&(this->nvm.divider2);
	uint32_t userLength = (uint32_t)ptrUserEnd;
	userLength -= (uint32_t)ptrUser;
	if (withChecksum) {
		userLength += sizeof(uint8_t) + sizeof(uint16_t);
	}
	return userLength;
}

/*
Ensures that the checksum fields inside NVM cache is correct
Used before printing out HexBlob

return: none
parameter: none
*/
void cBookWorm::ensureNvmChecksums(void)
{
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	uint8_t* ptrUser = (uint8_t*)&(this->nvm.divider1);
	int i;
	uint16_t chksum, chksumUser;
	uint32_t userLength = calcUserNvmLength(false);
	this->nvm.eeprom_version1 = BOOKWORM_EEPROM_VERSION;
	this->nvm.eeprom_version2 = BOOKWORM_EEPROM_VERSION;
	this->nvm.ssid[BOOKWORM_SSID_SIZE] = 0;
	this->SSID[BOOKWORM_SSID_SIZE] = 0;
	this->nvm.password[BOOKWORM_PASSWORD_SIZE] = 0;
	chksumUser = bookworm_fletcher16(ptrUser, userLength);
	this->nvm.checksumUser = chksumUser;
	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	this->nvm.checksum = chksum;
}

/*
Sets all NVM items to default values, and generates a SSID

return: none
parameters: none
*/
void cBookWorm::factoryReset() {
	defaultValues();
	this->generateSsid(this->SSID);
	this->setSsid(this->SSID);
	this->printf("factory reset performed\r\n");
}


/*
Load saved information from NVM

return: successful or not
parameters: none
*/
bool cBookWorm::loadNvm()
{
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	int i;
	uint16_t chksum;

	this->debugf("NVM reading: ");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		ptr[i] = EEPROM.read(i);
		this->debugf(" %02X", ptr[i]);
	}
	this->debugf(" done!\r\n");

	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	if (chksum == this->nvm.checksum && BOOKWORM_EEPROM_VERSION == this->nvm.eeprom_version1 && BOOKWORM_EEPROM_VERSION == this->nvm.eeprom_version2)
	{
		memcpy(this->SSID, this->nvm.ssid, 32);
		this->SSID[BOOKWORM_SSID_SIZE] = 0;

		setWifiChannel(this->nvm.wifiChannel); // this does a range check

		loadPinAssignments();

		#if defined(ENABLE_WEAPON) && (pinServoWeapon == pinServoLeft || pinServoWeapon == pinServoRight)
		if (this->nvm.enableWeapon == false) {
			Serial.begin(BOOKWORM_BAUD);
			this->serialHasBegun = true;
		}
		#endif

		this->debugf("NVM loaded successfully, checksum %04X\r\n", chksum);

		return true;
	}
	else
	{
		this->debugf("NVM load failed, \r\n\tchecksum %04X != %04X\r\n", chksum, this->nvm.checksum);
		if (BOOKWORM_EEPROM_VERSION != this->nvm.eeprom_version1) {
			this->debugf("\tversion %04X != %04X\r\n", BOOKWORM_EEPROM_VERSION, this->nvm.eeprom_version1);
		}
		if (BOOKWORM_EEPROM_VERSION != this->nvm.eeprom_version2) {
			this->debugf("\tversion %04X != %04X\r\n", BOOKWORM_EEPROM_VERSION, this->nvm.eeprom_version2);
		}
		return false;
	}
}

/*
Save information into NVM

return: none
parameters: none
*/
void cBookWorm::saveNvm()
{
	int i;
	uint8_t* ptr = (uint8_t*)&(this->nvm);
	ensureNvmChecksums();
	this->debugf("EEPROM writing: ");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
		this->debugf(" %02X", ptr[i]);
	}
	EEPROM.commit();
	this->debugf(" done!\r\n");
}
