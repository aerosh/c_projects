#include "router.h"
#include "packetcontainer.h"
#include <string.h>

void routerInit(Router& router, unsigned node, PacketContainer* oc)
{
    router.nodeName = node;
    router.oc = oc;
    return;
}

bool routerAction(Router& router, Packet* packet)
{
   //проверка совпадения получателя пакета с именем узла
   if ( packet->header.destination == router.nodeName )
      return true;

   //не совпадают => перенаправить дальше по конвейеру
   Packet* output = pcStartWritePacket(router.oc);
   //копирование пакета 
   unsigned size = packet->header.size + sizeof(packet->header) + sizeof(unsigned);
   memcpy(output,packet,size);
   pcFinishWritePacket(router.oc);

   return false;
}


