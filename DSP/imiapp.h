#ifndef imiapp_h
#define imiapp_h

#include "procconfig.h"

#include <time.h>

/* 
 * внесение искажений в пакет с параметрами обработки
 * 1 - искажение в заголовке
 * 2 - искажение в теле
 */

#define IMI_HEADER_FAIL 1
#define IMI_BODY_FAIL 2

struct ImiApp {
   int writeFd;
   
   struct timespec actTime; //время генерации пакета
   struct timespec diagTime; //время генерации пакета с диагностикой приемных каналов
   bool fromUser; //признак формирования данных от пользователя
   int packetCount; //оставшееся количество имитируемых пакетов
   unsigned packetSize; //размер пакета (в отсчетах)
   unsigned maxLevel; //максимальное значение отсчета
   unsigned generationJitterLevel;  //уровень дрожания темпа генерации пакетов имитатором в миллисекундах
   ProcConfig procConfig; //параметры обработки для формирования пакета конфигурации
   bool badPacket; //формирование недопустимого пакета
   unsigned failOperation; //управление искажением пакета с параметрами обработки
};

int imiAppRun(ImiApp& app);

#endif
