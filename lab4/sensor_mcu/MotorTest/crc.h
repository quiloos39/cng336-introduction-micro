#ifndef CRC_H_
#define CRC_H_
#define CRC_POLY 0b11010100

#include <stdint.h>

void rollLeft(uint8_t *bh, uint8_t *bl) {
	// roll left from low byte to high byte
	*(bh) <<= 1;
	*(bh) |= (*(bl) >> 7); // paste the msb of bl
	*(bl) <<= 1;
}
uint8_t crc11(uint8_t bh, uint8_t bl) {
	// assume byte array's least significant (byte) is padded with 0s
	
	bh ^= CRC_POLY;
	int counter = 0;
	while (counter < (16-5)) {
		if (bh & 0b10000000) {
			bh ^= CRC_POLY;
			} else {
			rollLeft(&bh, &bl);
			counter++;
		}
	}
	
	return bh >> 3;
}

uint8_t crc3(uint8_t b) {
	return crc11((uint8_t)0, b);
}

// check functions return True if all is OK
uint8_t crc3Check(uint8_t b) {
	return crc3(b) == 0;
}
uint8_t crc11Check(uint8_t bh, uint8_t bl) {
	return crc11(bh, bl) == 0;
}

#endif