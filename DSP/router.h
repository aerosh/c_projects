#ifndef router_h
#define router_h

struct PacketContainer;
struct Packet;

struct Router
{
    unsigned nodeName; //имя узла
    PacketContainer* oc; //кольцевой буфер для выдачи пакетов
};

//инициализация маршрутизатора
void routerInit(Router& router, unsigned node, PacketContainer* oc);
//анализ пакета и его перенаправление при необходимости
//возвращает признак необходимости обработки пакета
bool routerAction(Router& router, Packet* packet);

#endif
