#include "consumerapp.h"
#include "packet.h"
#include "outputpacket.h"
#include "diagpacket.h"
#include "messageid.h"

#include <unistd.h>
#include <cstdlib>
#include <iostream>
using namespace std;

static void consumerAppOutPacket(ConsumerApp& app, Packet& packet);
static bool consumerAppReceivePacket(ConsumerApp& app, Packet& packet);

int consumerAppRun(ConsumerApp& app) {
   
   Packet packet;
   while ( 1 ) {
      bool needContinue = consumerAppReceivePacket(app,packet);
      if ( !needContinue )
         break;
      
      consumerAppOutPacket(app,packet);
   }
   
   return 0;
}

static void consumerAppOutPacket(ConsumerApp& app, Packet& packet) {
   
   //обнаружение информации о плохом пакете
   if ( packet.header.message == MESSAGE_BADPACKET ) {
      cout << "bad packet" << endl;
      return;
   }
   //обнаружение информации о состоянии приемных каналов 
   if ( packet.header.message == MESSAGE_INPUTDIAG ) {
      cout << "input diag packet" << endl;
      return;
   }
   //обнаружение информации о диагностике узла 
   if ( packet.header.message == MESSAGE_NODEDIAG ) {
      DiagPacketBody* body = (DiagPacketBody*) packet.body;
      cout << "node: " << packet.header.source << ' ' <<
              "fail packets: " << body->failPacketsCount << endl;
      return;
   }
   
   //обработка только пакета с результатами
   if ( packet.header.message != MESSAGE_OUTPUTPACKET )
      return;
   
   OutputPacketBody* body = (OutputPacketBody*)packet.body;
   cout << "count= " << body->count << endl;
   
   cout << "items= ";
   for(unsigned i = 0; i<body->count; ++i)
      cout << "(" << body->data[i].idx << ' ' << body->data[i].level << ") ";
   cout << endl;
   
   return;
}

static bool consumerAppReceivePacket(ConsumerApp& app, Packet& packet)
{
   int ret = read(app.readFd,&packet.header,sizeof(packet.header));
   if ( ret == 0 || ret == -1 )
      return false;
   
   if ( packet.header.size == 0 )
      return true;
   
   ret = read(app.readFd,packet.body,packet.header.size+sizeof(unsigned));
   if ( ret == 0 || ret == -1 )
      return false;
   
   return true;
}

