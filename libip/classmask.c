#include "u.h"
#include "kern.h"
#include "ip.h"

static uchar classmask[4][16] = {
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0x00,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0x00,0x00,
	0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0xff,  0xff,0xff,0xff,0x00,
};

static uchar v6loopback[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01
};

static uchar v6linklocal[IPaddrlen] = {
	0xfe, 0x80, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static uchar v6linklocalmask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static int v6llpreflen = 8;	/* link-local prefix length in bytes */

static uchar v6multicast[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static uchar v6multicastmask[IPaddrlen] = {
	0xff, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0
};
static int v6mcpreflen = 1;	/* multicast prefix length */

static uchar v6solicitednode[IPaddrlen] = {
	0xff, 0x02, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0x01,
	0xff, 0, 0, 0
};
static uchar v6solicitednodemask[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0x0, 0x0, 0x0
};
static int v6snpreflen = 13;

uchar*
defmask(uchar *ip)
{
	if(isv4(ip))
		return classmask[ip[IPv4off]>>6];
	else {
		if(ipcmp(ip, v6loopback) == 0)
			return IPallbits;
		else if(memcmp(ip, v6linklocal, v6llpreflen) == 0)
			return v6linklocalmask;
		else if(memcmp(ip, v6solicitednode, v6snpreflen) == 0)
			return v6solicitednodemask;
		else if(memcmp(ip, v6multicast, v6mcpreflen) == 0)
			return v6multicastmask;
		return IPallbits;
	}
}

void
maskip(uchar *from, uchar *mask, uchar *to)
{
	int i;

	for(i = 0; i < IPaddrlen; i++)
		to[i] = from[i] & mask[i];
}
