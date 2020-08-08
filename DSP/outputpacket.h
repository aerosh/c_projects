#ifndef outputpacket_h
#define outputpacket_h

struct OutputPacketItem 
{
	unsigned idx;
	unsigned level;
};

#define OUTPUTPACKET_MAXCOUNT 100

struct OutputPacketBody
{
	unsigned count;
	OutputPacketItem data[OUTPUTPACKET_MAXCOUNT];
};

#endif
