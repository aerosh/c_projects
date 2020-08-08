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
   //�������� ���������� ���������� ������ � ������ ����
   if ( packet->header.destination == router.nodeName )
      return true;

   //�� ��������� => ������������� ������ �� ���������
   Packet* output = pcStartWritePacket(router.oc);
   //����������� ������ 
   unsigned size = packet->header.size + sizeof(packet->header) + sizeof(unsigned);
   memcpy(output,packet,size);
   pcFinishWritePacket(router.oc);

   return false;
}


