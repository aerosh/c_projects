#include "dispatcher.h"

void dispInit(Dispatcher& disp) {

   for(unsigned i=0; i<DISP_MSGMAXCNT; ++i)
      disp.table[i].handler = 0;
    return;
}

void dispAddHandler(Dispatcher& disp, unsigned id, PacketHandler handler, 
   void* clientData) {

   if ( id < DISP_MSGMAXCNT ) {
      disp.table[id].handler = handler;
      disp.table[id].clientData = clientData;
   }
   return;
}

int dispProcess(Dispatcher& disp, Packet* packet) {

   if ( !packet )
      return -1;

   unsigned message = packet->header.message;
   if ( message >= DISP_MSGMAXCNT )
      return -1;

   PacketHandler handler = disp.table[message].handler;
   if ( !handler )
      return -1;

   //вызов обработчика
   (*handler)(packet,disp.table[message].clientData);
   return 0;
}
