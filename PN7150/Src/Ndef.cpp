#include <cstdio>
#include "Ndef.h"

// Borrowed from Adafruit_NFCShield_I2C
void PrintHex(const uint8_t * data, const long numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++)
  {
    printf("0x");
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      printf("0");
    printf("%X", data[szPos]&0xff);
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      printf(" ");
    }
  }
  printf("\n");
}

// Borrowed from Adafruit_NFCShield_I2C
void PrintHexChar(const uint8_t * data, const long numBytes)
{
  uint32_t szPos;
  for (szPos=0; szPos < numBytes; szPos++)
  {
    // Append leading 0 for small values
    if (data[szPos] <= 0xF)
      printf("0");
    printf("%X", data[szPos]);
    if ((numBytes > 1) && (szPos != numBytes - 1))
    {
      printf(" ");
    }
  }
  printf("  ");
  for (szPos=0; szPos < numBytes; szPos++)
  {
    if (data[szPos] <= 0x1F)
      printf(".");
    else
      printf("%c",(char)data[szPos]);
  }
  printf("\n");
}

// Note if buffer % blockSize != 0, last block will not be written
void DumpHex(const uint8_t * data, const long numBytes, const unsigned int blockSize)
{
    int i;
    for (i = 0; i < (numBytes / blockSize); i++)
    {
        PrintHexChar(data, blockSize);
        data += blockSize;
    }
}
