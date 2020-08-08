#include "procapp.h"
#include "imiapp.h"
#include "consumerapp.h"

#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>

struct ControlParameters 
{
   bool fromUser; //данные от пользователя
   unsigned packetCount; //количество имитируемых пакетов. 0 => бесконечно
   unsigned packetSize; //количество отсчётов в имитируемом пакете
   unsigned maxLevel; //максимально возможное значение отсчёта сигнала
   ProcConfig procConfig;  //параметры обработки
   unsigned procDelay; //длительность времени обработки в миллисекундах (не менее)
   unsigned generationJitterLevel; //уровень дрожания темпа генерации пакетов имитатором в миллисекундах
   bool badPacket; //формирование недопустимого пакетв
   bool failOperation; //пояснения см. в имитаторе
   
};

bool optionsParse(int argc, char** argv, ControlParameters& params);

int main(int argc, char** argv)
{
   ProcConfig defaultConfig;
   defaultConfig.coeff = 2;
   
   ControlParameters params;
   params.fromUser = true;
   params.packetCount = 4;
   params.packetSize = 16;
   params.maxLevel = 1024;
   params.procConfig = defaultConfig;
   params.procDelay = 0;
   params.generationJitterLevel = 0;
   params.badPacket = false;
   params.failOperation = false;
   
   if ( !optionsParse(argc, argv, params) )
      return 5;
   
   int pipeFd[2];
   if ( pipe(pipeFd) == -1 ) {
      perror("pipe");
      return 2;
   }
   int pipeConsumer[2];
   if ( pipe(pipeConsumer) == -1 ) {
      perror("pipe");
      return 3;
   }
   
   //порождение процесса обработки
   pid_t pid = fork();
   if ( pid < 0 ) {
      perror("fork");
      return 1;
   }
   if ( pid == 0 ) {
      close(pipeFd[1]);
      close(pipeConsumer[0]);
      
      ProcApp app;
      app.procConfig = defaultConfig;
      app.readFd = pipeFd[0];
      app.consumerFd = pipeConsumer[1];
      app.procDelay = params.procDelay;
      int ret = procAppRun(app);
      close(app.readFd);
      close(app.consumerFd);
      return ret;
   }
   
   //порождение процесса-имитатора
   pid_t imiPid = fork();
   if ( imiPid < 0 ) {
      perror("fork");
      return 1;
   }
   if ( imiPid == 0 ) {
      close(pipeFd[0]);
      close(pipeConsumer[0]);
      close(pipeConsumer[1]);
      
      ImiApp app;
      app.writeFd = pipeFd[1];
      app.fromUser = params.fromUser;
      app.packetCount = params.packetCount;
      app.packetSize = params.packetSize;
      app.maxLevel = params.maxLevel;
      app.generationJitterLevel = params.generationJitterLevel;
      app.procConfig = params.procConfig;
      app.badPacket = params.badPacket;
      app.failOperation = params.failOperation;
      
      int ret = imiAppRun(app);
      close(app.writeFd);
      return ret;
   }
   
   //порождение процесса-потребителя
   pid_t consumerPid = fork();
   if ( consumerPid < 0 ) {
      perror("fork");
      return 1;
   }
   if ( consumerPid == 0 ) {
      close(pipeFd[0]);
      close(pipeFd[1]);
      close(pipeConsumer[1]);
      
      ConsumerApp app;
      app.readFd = pipeConsumer[0];
      int ret = consumerAppRun(app);
      close(app.readFd);
      return ret;
   }
   
   close(pipeFd[0]);
   close(pipeFd[1]);	
   close(pipeConsumer[0]);
   close(pipeConsumer[1]);
   
   //ожидание завершения всех потомков
   for( unsigned i = 0; i < 3; ++i) {
      int status;
      while ( wait(&status) == -1 )
         continue;
   }
   return 0;
}

bool optionsParse(int argc, char **argv, ControlParameters& params)
{
   while ( 1 ) {
      int c = getopt (argc, argv, "ic:s:l:k:t:j:bf:");
      if ( c == -1 )
         break;
      switch (c) {
         case 'i':
            params.fromUser = false;
            break;
         case 'c':
            params.packetCount = atoi(optarg);
            break;
         case 's':
            params.packetSize = atoi(optarg);
            break;
         case 'l':
            params.maxLevel = atoi(optarg);
            break;
         case 'k':
            params.procConfig.coeff = atoi(optarg);
            break;
         case 't':
            params.procDelay = atoi(optarg);
            break;
         case 'j':
            params.generationJitterLevel = atoi(optarg);
            break;
         case 'b':
            params.badPacket = true;
            break;
         case 'f':
            params.failOperation = atoi(optarg);
            break;
         default:
            return false;
      }	
   }
   return true;
}
