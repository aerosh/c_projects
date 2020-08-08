#ifndef packetcontainer_h
#define packetcontainer_h

#include "packet.h"
#include <semaphore.h>

struct PacketContainer
{
   static const unsigned BufferSize=4;

	Packet data[BufferSize];
	sem_t canRead;
	sem_t canWrite;
	unsigned readIndex;
	unsigned writeIndex;
	Packet trashPacket;
	bool toTrash; //идет запись в мусорную корзину
};

void pcInit(PacketContainer* oc);
void pcDestroy(PacketContainer* oc);
Packet* pcStartReadPacket(PacketContainer* oc);
void pcFinishReadPacket(PacketContainer* oc);
Packet* pcStartWritePacket(PacketContainer* oc);
void pcFinishWritePacket(PacketContainer* oc);

#endif
