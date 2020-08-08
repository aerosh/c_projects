#ifndef dispatcher_h
#define dispatcher_h

#include "packet.h"

#define DISP_MSGMAXCNT 256

//packet - ������� �����
typedef void (*PacketHandler)(Packet* packet, void* clientData);

struct Dispatcher
{
   struct HandlerDescription {
      PacketHandler handler;
      void* clientData;
   };
   HandlerDescription table[DISP_MSGMAXCNT];
};

//������������� ����������
void dispInit(Dispatcher& disp);
//�������� ���������� ��������� ���������� ����
void dispAddHandler(Dispatcher& disp, unsigned id, PacketHandler handler, void* clientData);
//��������� ��������� ��������� ���������� ����
int dispProcess(Dispatcher& disp, Packet* packet);

#endif

