#ifndef packet_h
#define packet_h

#define PACKET_MAXSIZE 4096

struct PacketHeader
{
    unsigned message;
    unsigned size;
    unsigned source;  //�������� ������
    unsigned destination;  //���������� ������
    unsigned crc; //����������� �����
};

struct Packet
{
   PacketHeader header;
   unsigned body[PACKET_MAXSIZE];
};

void packetSetupHeaderCrc(Packet& packet);
void packetSetupBodyCrc(Packet& packet);
bool packetTestHeaderCrc(Packet& packet);
bool packetTestBodyCrc(Packet& packet);

#endif
