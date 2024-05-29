/*  
 *一口Linux
 *2021.6.21
 *version: 1.0.0
*/
#include "crc.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef enum {
 REF_4BIT = 4,
 REF_5BIT = 5,
 REF_6BIT = 6,
 REF_7BIT = 7,
 REF_8BIT = 8,
 REF_16BIT = 16,
 REF_32BIT = 32
}REFLECTED_MODE;

uint32_t ReflectedData(uint32_t data, REFLECTED_MODE mode)
{
 data = ((data & 0xffff0000) >> 16) | ((data & 0x0000ffff) << 16);
 data = ((data & 0xff00ff00) >> 8) | ((data & 0x00ff00ff) << 8);
 data = ((data & 0xf0f0f0f0) >> 4) | ((data & 0x0f0f0f0f) << 4);
 data = ((data & 0xcccccccc) >> 2) | ((data & 0x33333333) << 2);
 data = ((data & 0xaaaaaaaa) >> 1) | ((data & 0x55555555) << 1);

 switch (mode)
 {
 case REF_32BIT:
  return data;
 case REF_16BIT:
  return (data >> 16) & 0xffff;
 case REF_8BIT:
  return (data >> 24) & 0xff;
 case REF_7BIT:
  return (data >> 25) & 0x7f;
 case REF_6BIT:
  return (data >> 26) & 0x7f;
 case REF_5BIT:
  return (data >> 27) & 0x1f;
 case REF_4BIT:
  return (data >> 28) & 0x0f;
 }
 return 0;
}

uint8_t CheckCrc4(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_4BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 4;
  poly <<= 4;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 4) ^ xorOut;
 }
}

uint8_t CheckCrc5(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_5BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 3;
  poly <<= 3;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 3) ^ xorOut;
 }
}

uint8_t CheckCrc6(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_6BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 2;
  poly <<= 2;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 2) ^ xorOut;
 }
}

uint8_t CheckCrc7(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint8_t i;
 uint8_t crc;

 if (refIn == true)
 {
  crc = init;
  poly = ReflectedData(poly, REF_7BIT);

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x01)
    {
     crc >>= 1;
     crc ^= poly;
    }
    else
    {
     crc >>= 1;
    }
   }
  }

  return crc ^ xorOut;
 }
 else
 {
  crc = init << 1;
  poly <<= 1;

  while (length--)
  {
   crc ^= *buffer++;
   for (i = 0; i < 8; i++)
   {
    if (crc & 0x80)
    {
     crc <<= 1;
     crc ^= poly;
    }
    else
    {
     crc <<= 1;
    }
   }
  }

  return (crc >> 1) ^ xorOut;
 }
}

uint8_t CheckCrc8(uint8_t poly, uint8_t init, bool refIn, bool refOut, uint8_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint8_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT);
  }
  else
  {
   crc ^= *buffer++;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x80)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_8BIT);
 }

 return crc ^ xorOut;
}

uint16_t CheckCrc16(uint16_t poly, uint16_t init, bool refIn, bool refOut, uint16_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint16_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT) << 8;
  }
  else
  {
   crc ^= (*buffer++) << 8;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x8000)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_16BIT);
 }

 return crc ^ xorOut;
}

uint32_t CheckCrc32(uint32_t poly, uint32_t init, bool refIn, bool refOut, uint32_t xorOut,
 const uint8_t *buffer, uint32_t length)
{
 uint32_t i = 0;
 uint32_t crc = init;

 while (length--)
 {
  if (refIn == true)
  {
   crc ^= ReflectedData(*buffer++, REF_8BIT) << 24;
  }
  else
  {
   crc ^= (*buffer++) << 24;
  }

  for (i = 0; i < 8; i++)
  {
   if (crc & 0x80000000)
   {
    crc <<= 1;
    crc ^= poly;
   }
   else
   {
    crc <<= 1;
   }
  }
 }

 if (refOut == true)
 {
  crc = ReflectedData(crc, REF_32BIT);
 }

 return crc ^ xorOut;
}

uint32_t CrcCheck(CRC_Type crcType, const uint8_t *buffer, uint32_t length)
{
 switch (crcType.width)
 {
 case 4:
  return CheckCrc4(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 5:
  return CheckCrc5(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 6:
  return CheckCrc6(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 7:
  return CheckCrc7(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 8:
  return CheckCrc8(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 16:
  return CheckCrc16(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 case 32:
  return CheckCrc32(crcType.poly, crcType.init, crcType.refIn, crcType.refOut,
   crcType.xorOut, buffer, length);
 }
 return 0;
}

uint32_t calculate_crc32(void * addr ,size_t size) {
    CRC_Type crcType={ 32, 0x04c11db7, 0xffffffff, true, true, 0xffffffff };
    uint8_t * allocatedMemory = (uint8_t *)addr;
    uint32_t crc32 =  CrcCheck(crcType, allocatedMemory, size);
    return crc32;
}
