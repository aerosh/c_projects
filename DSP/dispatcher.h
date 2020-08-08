#ifndef dispatcher_h
#define dispatcher_h

#include "packet.h"

#define DISP_MSGMAXCNT 256

//packet - входной пакет
typedef void (*PacketHandler)(Packet* packet, void* clientData);

struct Dispatcher
{
   struct HandlerDescription {
      PacketHandler handler;
      void* clientData;
   };
   HandlerDescription table[DISP_MSGMAXCNT];
};

//инициализация диспетчера
void dispInit(Dispatcher& disp);
//добавить обработчик сообщения указанного типа
void dispAddHandler(Dispatcher& disp, unsigned id, PacketHandler handler, void* clientData);
//выполнить обработку сообщения указанного типа
int dispProcess(Dispatcher& disp, Packet* packet);

#endif

