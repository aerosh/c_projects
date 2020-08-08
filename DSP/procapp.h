#ifndef procapp_h
#define procapp_h

#include "procconfig.h"
#include "dispatcher.h"
#include "packetcontainer.h"
#include "router.h"


struct ProcApp
{
   ProcConfig procConfig;
   int readFd;
   int consumerFd;
   unsigned procDelay; //�������� ��� �������� ��������� ��������� ������������
   Dispatcher dispatcher; //��������� �������
   PacketContainer ic; //��������� ����� ������
   PacketContainer oc; //��������� ����� ������
   Router router; //������������� �������
   volatile unsigned failPacketsCount; //���������� ������� �������
};

int procAppRun(ProcApp& app);

#endif
