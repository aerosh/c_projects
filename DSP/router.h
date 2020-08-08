#ifndef router_h
#define router_h

struct PacketContainer;
struct Packet;

struct Router
{
    unsigned nodeName; //��� ����
    PacketContainer* oc; //��������� ����� ��� ������ �������
};

//������������� ��������������
void routerInit(Router& router, unsigned node, PacketContainer* oc);
//������ ������ � ��� ��������������� ��� �������������
//���������� ������� ������������� ��������� ������
bool routerAction(Router& router, Packet* packet);

#endif
