#ifndef __CRC_H__
#define __CRC_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
 uint8_t width;
 uint32_t poly;
 uint32_t init;
 bool refIn;
 bool refOut;
 uint32_t xorOut;
}CRC_Type;

uint32_t CrcCheck(CRC_Type crcType, const uint8_t *buffer, uint32_t length);
uint32_t calculate_crc32(void * addr ,size_t size);
#endif
