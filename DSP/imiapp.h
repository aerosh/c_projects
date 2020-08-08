#ifndef imiapp_h
#define imiapp_h

#include "procconfig.h"

#include <time.h>

/* 
 * �������� ��������� � ����� � ����������� ���������
 * 1 - ��������� � ���������
 * 2 - ��������� � ����
 */

#define IMI_HEADER_FAIL 1
#define IMI_BODY_FAIL 2

struct ImiApp {
   int writeFd;
   
   struct timespec actTime; //����� ��������� ������
   struct timespec diagTime; //����� ��������� ������ � ������������ �������� �������
   bool fromUser; //������� ������������ ������ �� ������������
   int packetCount; //���������� ���������� ����������� �������
   unsigned packetSize; //������ ������ (� ��������)
   unsigned maxLevel; //������������ �������� �������
   unsigned generationJitterLevel;  //������� �������� ����� ��������� ������� ���������� � �������������
   ProcConfig procConfig; //��������� ��������� ��� ������������ ������ ������������
   bool badPacket; //������������ ������������� ������
   unsigned failOperation; //���������� ���������� ������ � ����������� ���������
};

int imiAppRun(ImiApp& app);

#endif
