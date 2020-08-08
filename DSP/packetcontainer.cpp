#include "packetcontainer.h"

void pcInit(PacketContainer* pc)
{
	sem_init(&pc->canRead,0,0);
	sem_init(&pc->canWrite,0,PacketContainer::BufferSize);
	pc->readIndex = 0;
	pc->writeIndex = 0;
	pc->toTrash = false;
}

void pcDestroy(PacketContainer* pc)
{
	sem_destroy(&pc->canRead);
	sem_destroy(&pc->canWrite);
}

Packet* pcStartReadPacket(PacketContainer* pc) {
   
   //������� ���������� ������
   sem_wait(&pc->canRead);
   
   Packet* ret = pc->data + pc->readIndex;
   //����������� �����
   pc->readIndex++;
   if ( pc->readIndex == PacketContainer::BufferSize )
      pc->readIndex = 0;
   
   return ret;
}

void pcFinishReadPacket(PacketContainer* pc) {
   
   //������� ������ ����� ��������� ��� ������ ���������� ������
   sem_post(&pc->canWrite);
   return;
}

Packet* pcStartWritePacket(PacketContainer* pc) {
   
   //�������� ��������� ��� ������ ������� ������
   //��� ���������� � �������� �������
   pc->toTrash =  sem_trywait(&pc->canWrite) == -1;
   if ( pc->toTrash )
      return &pc->trashPacket;
   
   Packet* ret = pc->data + pc->writeIndex;
   //����������� �����
   pc->writeIndex++;
   if ( pc->writeIndex == PacketContainer::BufferSize )
      pc->writeIndex = 0;
   
   return ret;   
}

void pcFinishWritePacket(PacketContainer* pc) {
   
   //����� �����
   if ( !pc->toTrash )
      sem_post(&pc->canRead);

   pc->toTrash = false;

   return;
}
