#include "imiapp.h"
#include "packet.h"
#include "inputpacket.h"
#include "configpacket.h"
#include "messageid.h"
#include "nodes.h"

#include <unistd.h>
#include <iostream>
#include <cstdlib>
using namespace std;

#include <time.h>

static bool imiAppBuildPacket(ImiApp& app, Packet& packet);
static bool imiAppGetFromUser(ImiApp& app, Packet& packet);
static bool imiAppGeneratePacket(ImiApp& app, Packet& packet);

//расчет метки времени, отстоящей от заданной на указанный интервал
struct timespec add(struct timespec left, time_t tv_sec, long tv_nsec) {
   
   left.tv_sec += tv_sec;
   left.tv_nsec += tv_nsec;
   if ( left.tv_nsec >= 1000000000 ) {
      ++left.tv_sec;
      left.tv_nsec -= 1000000000;
   }
   return left;
}

int imiAppRun(ImiApp& app) {
   
   //формирование первой метки времени для генерации пакета
   clock_gettime(CLOCK_MONOTONIC,&app.actTime);
   app.diagTime = app.actTime;
   
   Packet packet;
   
   //формирование и отправка пакета с параметрами обработки 
   ConfigPacketBody* cfgBody = (ConfigPacketBody*)(packet.body);
   cfgBody->coeff = app.procConfig.coeff;
   packet.header.message = MESSAGE_PROCCONFIG;
   packet.header.size = sizeof(ConfigPacketBody);
   packet.header.source = NODE_IMITATOR;
   packet.header.destination = NODE_PROCESSING;
   packetSetupHeaderCrc(packet);
   packetSetupBodyCrc(packet);
   
   if ( app.failOperation == IMI_HEADER_FAIL ) {
      unsigned* header = (unsigned*)(&packet.header);
      unsigned failWordIndex = rand()%(sizeof(PacketHeader)/sizeof(unsigned));
      unsigned failBit = rand()%32;
      header[failWordIndex] ^= (1<<failBit);
   } 
   else if ( app.failOperation == IMI_BODY_FAIL ) {
      unsigned* body = packet.body;
      unsigned failWordIndex = rand()%(packet.header.size/sizeof(unsigned));
      unsigned failBit = rand()%32;
      body[failWordIndex] ^= (1<<failBit);
   }
         
   write(app.writeFd,&packet.header,sizeof(packet.header));
   write(app.writeFd,packet.body,packet.header.size+sizeof(unsigned));
   
   //формирование и отправка ошибочного пакета 
   if ( app.badPacket ) {
      packet.header.message = MESSAGE_OUTPUTPACKET;
      packet.header.size = 0;
      packet.header.source = NODE_IMITATOR;
      packet.header.destination = NODE_PROCESSING;
      packetSetupHeaderCrc(packet);
      packetSetupBodyCrc(packet);
      
      write(app.writeFd,&packet.header,sizeof(packet.header));
      write(app.writeFd,packet.body,packet.header.size+sizeof(unsigned));
   }
   
   while ( 1 ) {
      //формирование пакета
      bool needContinue = imiAppBuildPacket(app,packet);
      if ( !needContinue )
         break;
      
      //запись пакета в канал обмена
      write(app.writeFd,&packet.header,sizeof(packet.header));
      write(app.writeFd,packet.body,packet.header.size+sizeof(unsigned));
   }
   
   return 0;
}

bool imiAppBuildPacket(ImiApp& app, Packet& packet) {
   
   return app.fromUser ? imiAppGetFromUser(app,packet) : imiAppGeneratePacket(app,packet);
}

bool imiAppGetFromUser(ImiApp& app, Packet& packet) {
   
   InputPacketBody* body = (InputPacketBody*)packet.body;
   cout << "count=";
   cin >> body->count;
   if ( body->count > INPUTPACKET_MAXCOUNT )
      return false;
   
   cout << "items=";
   for(unsigned i = 0; i<body->count; ++i)
      cin >> body->data[i].level;
   
   //формирование размера пакета
   packet.header.size = body->count*sizeof(InputPacketItem) + sizeof(body->count);
   packet.header.message = MESSAGE_INPUTPACKET;
   packet.header.source = NODE_IMITATOR;
   packet.header.destination = NODE_PROCESSING;
   packetSetupHeaderCrc(packet);
   packetSetupBodyCrc(packet);
   
   return true;
}

bool imiAppGeneratePacket(ImiApp& app, Packet& packet) {
   
   if ( !app.packetCount )
      return false;
   
   static struct timespec imiTime;
   
   /* определить какое событие произойдет раньше
    * и ожидать время его наступления
    */
   if ( (app.diagTime.tv_sec < app.actTime.tv_sec) ||
      (app.diagTime.tv_sec == app.actTime.tv_sec) && 
      (app.diagTime.tv_nsec < app.actTime.tv_nsec) ) {
      
      clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&app.diagTime,NULL);
      app.diagTime = add(app.diagTime,2,0); 
      //сформировать и отправить диагностический пакет
      packet.header.size = 0; /*в реальности пакет содержит описание состояния каналов*/
      packet.header.message = MESSAGE_INPUTDIAG;
      packet.header.source = NODE_IMITATOR;
      packet.header.destination = NODE_CONSUMER;
      packetSetupHeaderCrc(packet);
      packetSetupBodyCrc(packet);
      
      return true;
   }

   //ожидание момента времени
   int ret = clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&app.actTime,NULL);
   
   //расчет следующей метки времени
   long jitter = long(rand() % (2*app.generationJitterLevel+1)) - app.generationJitterLevel;
   long timeDelay = 100000000 + jitter * 1000000;
   if ( timeDelay < 0 )
      timeDelay = 0;
   app.actTime = add(app.actTime,0,timeDelay);
   
   //формирование пакета
   InputPacketBody* body = (InputPacketBody*)packet.body;
   body->count = app.packetSize;
   for(unsigned i = 0; i<body->count; ++i )
      body->data[i].level =  rand() % app.maxLevel;
   
   //формирование размера пакета
   packet.header.size = body->count*sizeof(InputPacketItem) + sizeof(body->count);
   packet.header.message = MESSAGE_INPUTPACKET;
   packet.header.source = NODE_IMITATOR;
   packet.header.destination = NODE_PROCESSING;
   packetSetupHeaderCrc(packet);
   packetSetupBodyCrc(packet);
   
   //изменение количества генерации пакетов
   if( app.packetCount >0 )
      --app.packetCount;
   return true;
}
