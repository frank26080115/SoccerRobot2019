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
			4 means version does not match

parameters:
	str, a string of hexadecimal to process
	save, automatically save (commit) to NVM if successful
*/
bool cBookWorm::loadNvmHex(char* str, uint8_t* errCode, bool save)
{
	uint8_t* tmpbuff = (uint8_t*)malloc(sizeof(bookworm_nvm_t));
	bookworm_nvm_t* castPtr = (bookworm_nvm_t*)tmpbuff;

	debugf("NVM hexblob\r\n%s\r\n", str);

	int slen = strlen(str);
	uint16_t chksum;
	uint32_t userLength = calcUserNvmLength(false);
	uint32_t userLengthWithChksum = calcUserNvmLength(true);
	int i, j, k;
	for (i = 0, j = 0; i < slen && j < sizeof(bookworm_nvm_t); i += 2, j++)
	{
		char c[2];
		c[0] = str[i];
		c[1] = str[i + 1];
		uint8_t d = 0;
		for (k = 0; k < 2; k++)
		{
			d <<= 4;
			char cc = c[k];
			if (cc >= '0' && cc <= '9') {
				d += cc - '0';
			}
			else if (cc >= 'a' && cc <= 'f') {
				d += cc - 'a' + 0xA;
			}
			else if (cc >= 'A' && cc <= 'F') {
				d += cc - 'A' + 0xA;
			}
			else if ((cc == ' ' || cc == '\t' || cc == '\n' || cc == '\r' || cc == '\0') && k == 0) {
				i++;
				k--;
				c[0] = str[i];
				c[1] = str[i + 1];
				d = 0;
			}
			else {
				if (errCode != NULL) { *errCode = 2; /* valid chars failed */
					debugf("bad char %02X pos %u, j %u\r\n", c, i, j);
				}
				free(tmpbuff); return false;
			}
		}

		tmpbuff[j] = d;
	}

	if (j == sizeof(bookworm_nvm_t))
	{
		// wifi + user blocks
		chksum = bookworm_fletcher16(tmpbuff, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
		if (chksum != castPtr->checksum) {
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		if (calcEepromVersion() != castPtr->eeprom_version2 || calcEepromVersion() != castPtr->eeprom_version1) {
			if (errCode != NULL) { *errCode = 4; /* version matching failed */ }
			free(tmpbuff); return false;
		}
		memcpy((void*)(&(this->nvm)), (const void*)tmpbuff, sizeof(bookworm_nvm_t));
		free(tmpbuff);
		if (save) {
			saveNvm();
		}
		return true;
	}
	else if (j == userLengthWithChksum)
	{
		// user block only
		uint8_t* ptrUser = &(castPtr->divider1);
		// shift to align with structure
		uint32_t diff = ((uint32_t)ptrUser) - ((uint32_t)tmpbuff);
		for (i = sizeof(bookworm_nvm_t) - 1; i >= diff; i--) {
			tmpbuff[i] = tmpbuff[i - diff];
		}
		chksum = bookworm_fletcher16(ptrUser, userLength);
		if (chksum != castPtr->checksumUser) {
			debugf("bad chksum, calculated %04X, read in %04X\r\n", chksum, castPtr->checksumUser);
			if (errCode != NULL) { *errCode = 3; /* checksum failed */ }
			free(tmpbuff); return false;
		}
		if (calcEepromVersion() != castPtr->eeprom_version2) {
			if (errCode != NULL) { *errCode = 4; /* version matching failed */ }
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
	this->nvm.eeprom_version1 = calcEepromVersion();
	this->nvm.eeprom_version2 = calcEepromVersion();
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
	uint8_t* tmpbuff = (uint8_t*)malloc(sizeof(bookworm_nvm_t));
	bookworm_nvm_t* castPtr = (bookworm_nvm_t*)tmpbuff;
	uint8_t* ptr = (uint8_t*)&(this->nvm);

	/*
	warning: writing directly into "ptr" could cause a crash
	cause is unknown
	workaround is to use a temporary buffer and then copying into ptr
	using memcpy
	*/

	int i, sz;
	uint16_t chksum;

	sz = sizeof(bookworm_nvm_t);
	this->debugf("NVM reading (%u):\r\n", sz);
	for (i = 0; i < sz; i++) {
		uint8_t dataByte;
		dataByte = EEPROM.read(i);
		this->debugf(".");
		tmpbuff[i] = dataByte;
		this->debugf("%02X", dataByte);
		if (((i + 1) % 16) == 0) {
			this->debugf("\r\n");
		}
	}
	this->debugf("\r\ndone!\r\n");

	chksum = bookworm_fletcher16(ptr, sizeof(bookworm_nvm_t) - sizeof(uint16_t));
	if (chksum == castPtr->checksum && calcEepromVersion() == castPtr->eeprom_version1 && calcEepromVersion() == castPtr->eeprom_version2)
	{
		memcpy((void*)ptr, (const void*)tmpbuff, sizeof(bookworm_nvm_t));
		free(tmpbuff);

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
		this->printf("NVM load failed, \r\n\tchecksum %04X != %04X\r\n", chksum, castPtr->checksum);
		if (calcEepromVersion() != this->nvm.eeprom_version1) {
			this->printf("\tversion %04X != %04X\r\n", calcEepromVersion(), castPtr->eeprom_version1);
		}
		if (calcEepromVersion() != this->nvm.eeprom_version2) {
			this->printf("\tversion %04X != %04X\r\n", calcEepromVersion(), castPtr->eeprom_version2);
		}
		free(tmpbuff);
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
	this->debugf("EEPROM writing:");
	for (i = 0; i < sizeof(bookworm_nvm_t); i++) {
		EEPROM.write(i, ptr[i]);
		if ((i % 16) == 0) {
			this->debugf("\r\n");
		}
		this->debugf("%02X ", ptr[i]);
	}
	EEPROM.commit();
	this->debugf(" done!\r\n");
}

uint32_t cBookWorm::calcEepromVersion()
{
	uint32_t x = BOOKWORM_EEPROM_VERSION;
	uint32_t sz = sizeof(bookworm_nvm_t);
	sz <<= 12;
	#ifdef ENABLE_WEAPON
	x |= 0x80000000;
	#endif
	#ifdef ENABLE_BATTERY_MONITOR
	x |= 0x40000000;
	#endif
	x |= sz;
	return x;
}
