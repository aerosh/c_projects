#include "crc.h"

unsigned crcXOR(unsigned* area, unsigned count) {

   unsigned crc = 0;
   for(unsigned i=0; i<count; i++)
      crc ^= area[i];
   return crc;
}

