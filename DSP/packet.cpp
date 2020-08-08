#include "packet.h"
#include "crc.h"

void packetSetupHeaderCrc(Packet& packet) {

   unsigned wordsCount = sizeof(packet.header)/sizeof(unsigned) - 1;
   packet.header.crc = crcXOR((unsigned*)&packet.header, wordsCount);
   return;
}

void packetSetupBodyCrc(Packet& packet) {

   unsigned* crcPtr = (unsigned*)((unsigned char*)(packet.body) + packet.header.size);
   *crcPtr = crcXOR(packet.body,packet.header.size/sizeof(unsigned));
   return;
}

bool packetTestHeaderCrc(Packet& packet) {
   
   unsigned wordsCount = sizeof(packet.header)/sizeof(unsigned) - 1;
   unsigned crc = crcXOR((unsigned*)&packet.header, wordsCount);
   
   return crc == packet.header.crc;
   
}

bool packetTestBodyCrc(Packet& packet) {

   unsigned* packetCrcPtr = (unsigned*)((unsigned char*)(packet.body) + packet.header.size);
   unsigned crc = crcXOR(packet.body,packet.header.size/sizeof(unsigned));
   
   return crc == *packetCrcPtr;
}
