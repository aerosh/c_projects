#include "procapp.h"
#include "packet.h"
#include "inputpacket.h"
#include "outputpacket.h"
#include "configpacket.h"
#include "diagpacket.h"
#include "packetcontainer.h"
#include "messageid.h"
#include "nodes.h"

#include <iostream>
#include <cstring>
using namespace std;

#include <unistd.h>
#include <pthread.h>

static bool procAppReceivePacket(ProcApp& app, Packet& packet);
static void procAppProcessing(ProcApp& app, Packet& input, Packet& output);
static void procAppSendPacket(ProcApp& app, Packet& packet);
//обработка пакета с конфигурационными параметрами
static void procAppConfig(ProcApp& app, Packet& packet);

static void processingImpl(const Packet& input, Packet& output, const ProcConfig& config);

//функции, исполняемые потоками
static void* processingExecute(void* arg);
static void* inputExecute(void* arg);
static void* outputExecute(void*);

//функции-обработчики для вызова из диспетчера
static void inputHandler(Packet* packet, void* clientData);
static void procConfigHandler(Packet* packet, void* clientData);

static void sendError(Packet* packet, PacketContainer& oc, unsigned source);
static void sendDiagPacket(unsigned failCount, PacketContainer& oc, unsigned destination, unsigned source);

//структурные типы данных для передачи параметров в функцию потока
struct InputThreadData {
   ProcApp* app;
   PacketContainer* ic;
};

struct OutputThreadData {
   ProcApp* app;
   PacketContainer* oc;
};

struct ProcessingThreadData {
   ProcApp* app;
   PacketContainer* ic;
   PacketContainer* oc;
};

int procAppRun(ProcApp& app) {
	
   app.failPacketsCount = 0;

   pcInit(&app.ic);
   pcInit(&app.oc);
   
   //инициализация диспетчера
   dispInit(app.dispatcher);
   dispAddHandler(app.dispatcher,MESSAGE_INPUTPACKET,inputHandler,&app);
   dispAddHandler(app.dispatcher,MESSAGE_PROCCONFIG,procConfigHandler,&app);

   routerInit(app.router,NODE_PROCESSING,&app.oc);
   
   //заполнение структур-параметров функций потоков
   InputThreadData itData;
   itData.app = &app;
   itData.ic = &app.ic;
   ProcessingThreadData ptData;
   ptData.app = &app;
   ptData.ic = &app.ic;
   ptData.oc = &app.oc;
   OutputThreadData otData;
   otData.app = &app;
   otData.oc = &app.oc;

   //объявление дескрипторов потоков
   pthread_t inputThread;
   pthread_t processingThread;
   pthread_t outputThread;
   
   //порождение потоков управления
   pthread_create(&inputThread,NULL,inputExecute,&itData);
   pthread_create(&processingThread,NULL,processingExecute,&ptData);
   pthread_create(&outputThread,NULL,outputExecute,&otData);

   //ожидание завершения потоков
   pthread_join(inputThread,0);
   pthread_join(processingThread,0);
   pthread_join(outputThread,0);
   
   //освобождение ресурсов буферов
   pcDestroy(&app.oc);
   pcDestroy(&app.ic);
   return 0;
}

static bool procAppReceivePacket(ProcApp& app, Packet& packet) {
   
   while ( true ) {
      //прием заголовка пакета
      int ret = read(app.readFd,&packet.header,sizeof(packet.header));
      if ( ret == 0 || ret == -1 )
         return false;
      
      /*
       * проверка контрольной суммы заголовка и 
       * "обнаружение" начала пакета при ее нарушении
       */
      unsigned headerWordsCount = sizeof(packet.header)/sizeof(unsigned);
      unsigned* headerAddress = (unsigned*) &packet.header;
      bool failedHeader = false;
      while ( true ) {
         //проверка совпадения контрольной суммы заголовка
         if ( packetTestHeaderCrc(packet) )
            break;
         
         //учесть пакет в статистике
         if ( !failedHeader ) 
            ++app.failPacketsCount;
         failedHeader = true;
         
         //сместить на 1 слово для поиска заголовка пакета
         for( unsigned i = 0; i<headerWordsCount; ++i )
            headerAddress[i] = headerAddress[i+1];
         
         //дочитать 1 слово из канала
         int ret = read(app.readFd,headerAddress+headerWordsCount-1,sizeof(unsigned));
         if ( ret == 0 || ret == -1 )
            return false;
         
      }
      
      //прием тела пакета
      ret = read(app.readFd,packet.body,packet.header.size+sizeof(unsigned));
      if ( ret == 0 || ret == -1 )
         return false;
      
      //проверка совпадения контрольной суммы тела
      if ( packetTestBodyCrc(packet) )
         break;
   
      ++app.failPacketsCount;
      //игнорируем сбойный пакет
      
   }
   return true;
}


static void procAppProcessing(ProcApp& app, Packet& input, Packet& output)
{
   struct timespec procTime;
   clock_gettime(CLOCK_MONOTONIC,&procTime);
   procTime.tv_nsec += app.procDelay*1000000;
   if ( procTime.tv_nsec >= 1000000000 ) {
        ++procTime.tv_sec;
        procTime.tv_nsec -= 1000000000;
   }

   processingImpl(input,output,app.procConfig);
   output.header.source = app.router.nodeName;
   output.header.destination = NODE_CONSUMER;
   packetSetupHeaderCrc(output);
   packetSetupBodyCrc(output);
   
   clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&procTime,NULL);  
   return;
}

static void procAppSendPacket(ProcApp& app, Packet& packet)
{
   write(app.consumerFd,&packet.header,sizeof(packet.header));
   write(app.consumerFd,packet.body,packet.header.size+sizeof(unsigned));
   return;
}

static void processingImpl(const Packet& input, Packet& output, const ProcConfig& config)
{
   InputPacketBody* iBody = (InputPacketBody*)input.body;
   OutputPacketBody* oBody = (OutputPacketBody*)output.body;
   
   unsigned outCount = 0;
   bool left = false;
   unsigned leftIdx = 0;
   for(unsigned i = 1; i+1 < iBody->count; ++i) {
      unsigned level = iBody->data[i].level;
      if ( iBody->data[i-1].level < level ) {
         left = true;
         leftIdx = i;
      }
      
      if ( left && level > iBody->data[i+1].level ) {
         if ( config.coeff*iBody->data[i-1].level > level && 
              config.coeff*iBody->data[i+1].level > level ) {
            oBody->data[outCount].level = level;
            oBody->data[outCount].idx = (leftIdx + i) / 2;
            outCount++;
         }
         left = false;
      }
   }
   oBody->count = outCount;
   output.header.size = oBody->count*sizeof(OutputPacketItem)+sizeof(oBody->count);
   output.header.message = MESSAGE_OUTPUTPACKET;
   return;
}

static void* processingExecute(void* arg) {

   ProcessingThreadData* params = (ProcessingThreadData*)(arg);
   PacketContainer* ic = params->ic;
   PacketContainer* oc = params->oc;
   Dispatcher* dispatcher = &params->app->dispatcher;
   Router* router = &params->app->router;
   
   unsigned oldFailPacketsCount = 0;
   while ( 1 ) {
      
      //обнаружение приема нового сбойного пакета и информирование о событии
      unsigned nowFailPacketsCount = params->app->failPacketsCount;
      if ( oldFailPacketsCount != nowFailPacketsCount ) {         
         sendDiagPacket(nowFailPacketsCount, params->app->oc, NODE_CONSUMER, router->nodeName);
         oldFailPacketsCount = nowFailPacketsCount;         
      }
      
      Packet* input = pcStartReadPacket(ic);
      
      //передать на анализ маршрутизатору
      bool needDispatch = routerAction(*router,input);
      if ( needDispatch ) {
         //передать на обработку диспетчеру
         int ret = dispProcess(*dispatcher,input);
         if ( ret == -1 ) {
            sendError(input,params->app->oc, router->nodeName);
         }
      }
      
      pcFinishReadPacket(ic);
   }
   return 0;
}

static void* inputExecute(void* arg) {

   InputThreadData* params = (InputThreadData*)(arg);
   PacketContainer* ic = params->ic;
   while ( 1 ) {
      Packet* input = pcStartWritePacket(ic);
      if ( !procAppReceivePacket(*params->app,*input) )
         break;
      pcFinishWritePacket(ic);
   }
   return 0;
}

static void* outputExecute(void* arg) {

   OutputThreadData* params = (OutputThreadData*)(arg);
   PacketContainer* oc = params->oc;
   while ( 1 ) {
      Packet* output = pcStartReadPacket(oc);
      procAppSendPacket(*params->app,*output);
      pcFinishReadPacket(oc);
   }
   return 0;
}

static void procAppConfig(ProcApp& app, Packet& packet) {
   
   ConfigPacketBody* cfgBody = (ConfigPacketBody*)packet.body;
   app.procConfig.coeff = cfgBody->coeff;
   return;
}

static void inputHandler(Packet* packet, void* clientData) {

   ProcApp* app = (ProcApp*)clientData;

   Packet* output = pcStartWritePacket(&app->oc);
   procAppProcessing(*app,*packet,*output);
   pcFinishWritePacket(&app->oc);

   return;
}

static void procConfigHandler(Packet* packet, void* clientData) {

   ProcApp* app = (ProcApp*)clientData;

   procAppConfig(*app,*packet);
   return;
}

static void sendError(Packet* packet, PacketContainer& oc, unsigned source) {
   
   Packet* output = pcStartWritePacket(&oc);
   
   //формирование заголовка
   output->header.size = packet->header.size + sizeof(packet->header) + sizeof(unsigned);
   output->header.message = MESSAGE_BADPACKET;
   output->header.source = source;
   output->header.destination = NODE_CONSUMER;
   packetSetupHeaderCrc(*output);
   
   //копирование плохого пакета 
   memcpy(output->body,packet,output->header.size);
   packetSetupBodyCrc(*output);
   
   pcFinishWritePacket(&oc);
   return;
}

static void sendDiagPacket(unsigned failCount, PacketContainer& oc, unsigned destination, unsigned source) {
   
   Packet* output = pcStartWritePacket(&oc);

   //формирование тела сообщения
   DiagPacketBody* body = (DiagPacketBody*)(output->body);
   body->failPacketsCount = failCount;
   
   //формирование заголовка
   output->header.size = sizeof(DiagPacketBody);
   output->header.message = MESSAGE_NODEDIAG;
   output->header.source = source;
   output->header.destination = destination;
   packetSetupHeaderCrc(*output);
   
   packetSetupBodyCrc(*output);
   
   pcFinishWritePacket(&oc);
   return;   
}
