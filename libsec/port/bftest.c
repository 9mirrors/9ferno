#include <u.h>
#include <libc.h>
#include "libsec.h"

enum{
	Bsz = 8,
};

typedef struct Testvector Testvector;

struct Testvector{
	uchar key[Bsz];
	uchar plain[Bsz];
	uchar cipher[Bsz];
};

/*
 * Blowfish test vectors from https://www.schneier.com/code/vectors.txt
 */
Testvector vector [] = {
	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x4E, 0xF9, 0x97, 0x45, 0x61, 0x98, 0xDD, 0x78}},

	{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	 {0x51, 0x86, 0x6F, 0xD5, 0xB8, 0x5E, 0xCB, 0x8A}},

	{{0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	 {0x7D, 0x85, 0x6F, 0x9A, 0x61, 0x30, 0x63, 0xF2}},

	{{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
	 {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
	 {0x24, 0x66, 0xDD, 0x87, 0x8B, 0x96, 0x3C, 0x9D}},

	{{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
	 {0x61, 0xF9, 0xC3, 0x80, 0x22, 0x81, 0xB0, 0x96}},

	{{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
	 {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0x7D, 0x0C, 0xC6, 0x30, 0xAF, 0xDA, 0x1E, 0xC7}},

	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x4E, 0xF9, 0x97, 0x45, 0x61, 0x98, 0xDD, 0x78}},

	{{0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10},
	 {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0x0A, 0xCE, 0xAB, 0x0F, 0xC6, 0xA0, 0xA2, 0x8D}},

	{{0x7C, 0xA1, 0x10, 0x45, 0x4A, 0x1A, 0x6E, 0x57},
	 {0x01, 0xA1, 0xD6, 0xD0, 0x39, 0x77, 0x67, 0x42},
	 {0x59, 0xC6, 0x82, 0x45, 0xEB, 0x05, 0x28, 0x2B}},

	{{0x01, 0x31, 0xD9, 0x61, 0x9D, 0xC1, 0x37, 0x6E},
	 {0x5C, 0xD5, 0x4C, 0xA8, 0x3D, 0xEF, 0x57, 0xDA},
	 {0xB1, 0xB8, 0xCC, 0x0B, 0x25, 0x0F, 0x09, 0xA0}},

	{{0x07, 0xA1, 0x13, 0x3E, 0x4A, 0x0B, 0x26, 0x86},
	 {0x02, 0x48, 0xD4, 0x38, 0x06, 0xF6, 0x71, 0x72},
	 {0x17, 0x30, 0xE5, 0x77, 0x8B, 0xEA, 0x1D, 0xA4}},

	{{0x38, 0x49, 0x67, 0x4C, 0x26, 0x02, 0x31, 0x9E},
	 {0x51, 0x45, 0x4B, 0x58, 0x2D, 0xDF, 0x44, 0x0A},
	 {0xA2, 0x5E, 0x78, 0x56, 0xCF, 0x26, 0x51, 0xEB}},

	{{0x04, 0xB9, 0x15, 0xBA, 0x43, 0xFE, 0xB5, 0xB6},
	 {0x42, 0xFD, 0x44, 0x30, 0x59, 0x57, 0x7F, 0xA2},
	 {0x35, 0x38, 0x82, 0xB1, 0x09, 0xCE, 0x8F, 0x1A}},

	{{0x01, 0x13, 0xB9, 0x70, 0xFD, 0x34, 0xF2, 0xCE},
	 {0x05, 0x9B, 0x5E, 0x08, 0x51, 0xCF, 0x14, 0x3A},
	 {0x48, 0xF4, 0xD0, 0x88, 0x4C, 0x37, 0x99, 0x18}},

	{{0x01, 0x70, 0xF1, 0x75, 0x46, 0x8F, 0xB5, 0xE6},
	 {0x07, 0x56, 0xD8, 0xE0, 0x77, 0x47, 0x61, 0xD2},
	 {0x43, 0x21, 0x93, 0xB7, 0x89, 0x51, 0xFC, 0x98}},

	{{0x43, 0x29, 0x7F, 0xAD, 0x38, 0xE3, 0x73, 0xFE},
	 {0x76, 0x25, 0x14, 0xB8, 0x29, 0xBF, 0x48, 0x6A},
	 {0x13, 0xF0, 0x41, 0x54, 0xD6, 0x9D, 0x1A, 0xE5}},

	{{0x07, 0xA7, 0x13, 0x70, 0x45, 0xDA, 0x2A, 0x16},
	 {0x3B, 0xDD, 0x11, 0x90, 0x49, 0x37, 0x28, 0x02},
	 {0x2E, 0xED, 0xDA, 0x93, 0xFF, 0xD3, 0x9C, 0x79}},

	{{0x04, 0x68, 0x91, 0x04, 0xC2, 0xFD, 0x3B, 0x2F},
	 {0x26, 0x95, 0x5F, 0x68, 0x35, 0xAF, 0x60, 0x9A},
	 {0xD8, 0x87, 0xE0, 0x39, 0x3C, 0x2D, 0xA6, 0xE3}},

	{{0x37, 0xD0, 0x6B, 0xB5, 0x16, 0xCB, 0x75, 0x46},
	 {0x16, 0x4D, 0x5E, 0x40, 0x4F, 0x27, 0x52, 0x32},
	 {0x5F, 0x99, 0xD0, 0x4F, 0x5B, 0x16, 0x39, 0x69}},

	{{0x1F, 0x08, 0x26, 0x0D, 0x1A, 0xC2, 0x46, 0x5E},
	 {0x6B, 0x05, 0x6E, 0x18, 0x75, 0x9F, 0x5C, 0xCA},
	 {0x4A, 0x05, 0x7A, 0x3B, 0x24, 0xD3, 0x97, 0x7B}},

	{{0x58, 0x40, 0x23, 0x64, 0x1A, 0xBA, 0x61, 0x76},
	 {0x00, 0x4B, 0xD6, 0xEF, 0x09, 0x17, 0x60, 0x62},
	 {0x45, 0x20, 0x31, 0xC1, 0xE4, 0xFA, 0xDA, 0x8E}},

	{{0x02, 0x58, 0x16, 0x16, 0x46, 0x29, 0xB0, 0x07},
	 {0x48, 0x0D, 0x39, 0x00, 0x6E, 0xE7, 0x62, 0xF2},
	 {0x75, 0x55, 0xAE, 0x39, 0xF5, 0x9B, 0x87, 0xBD}},

	{{0x49, 0x79, 0x3E, 0xBC, 0x79, 0xB3, 0x25, 0x8F},
	 {0x43, 0x75, 0x40, 0xC8, 0x69, 0x8F, 0x3C, 0xFA},
	 {0x53, 0xC5, 0x5F, 0x9C, 0xB4, 0x9F, 0xC0, 0x19}},

	{{0x4F, 0xB0, 0x5E, 0x15, 0x15, 0xAB, 0x73, 0xA7},
	 {0x07, 0x2D, 0x43, 0xA0, 0x77, 0x07, 0x52, 0x92},
	 {0x7A, 0x8E, 0x7B, 0xFA, 0x93, 0x7E, 0x89, 0xA3}},

	{{0x49, 0xE9, 0x5D, 0x6D, 0x4C, 0xA2, 0x29, 0xBF},
	 {0x02, 0xFE, 0x55, 0x77, 0x81, 0x17, 0xF1, 0x2A},
	 {0xCF, 0x9C, 0x5D, 0x7A, 0x49, 0x86, 0xAD, 0xB5}},

	{{0x01, 0x83, 0x10, 0xDC, 0x40, 0x9B, 0x26, 0xD6},
	 {0x1D, 0x9D, 0x5C, 0x50, 0x18, 0xF7, 0x28, 0xC2},
	 {0xD1, 0xAB, 0xB2, 0x90, 0x65, 0x8B, 0xC7, 0x78}},

	{{0x1C, 0x58, 0x7F, 0x1C, 0x13, 0x92, 0x4F, 0xEF},
	 {0x30, 0x55, 0x32, 0x28, 0x6D, 0x6F, 0x29, 0x5A},
	 {0x55, 0xCB, 0x37, 0x74, 0xD1, 0x3E, 0xF2, 0x01}},

	{{0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},
	 {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0xFA, 0x34, 0xEC, 0x48, 0x47, 0xB2, 0x68, 0xB2}},

	{{0x1F, 0x1F, 0x1F, 0x1F, 0x0E, 0x0E, 0x0E, 0x0E},
	 {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0xA7, 0x90, 0x79, 0x51, 0x08, 0xEA, 0x3C, 0xAE}},

	{{0xE0, 0xFE, 0xE0, 0xFE, 0xF1, 0xFE, 0xF1, 0xFE},
	 {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0xC3, 0x9E, 0x07, 0x2D, 0x9F, 0xAC, 0x63, 0x1D}},

	{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	 {0x01, 0x49, 0x33, 0xE0, 0xCD, 0xAF, 0xF6, 0xE4}},

	{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0xF2, 0x1E, 0x9A, 0x77, 0xB7, 0x1C, 0x49, 0xBC}},

	{{0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF},
	 {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	 {0x24, 0x59, 0x46, 0x88, 0x57, 0x54, 0x36, 0x9A}},

	{{0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10},
	 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
	 {0x6B, 0x5C, 0x5A, 0x9C, 0x5D, 0x9E, 0x0A, 0x5A}}
};

uchar CBCkey[16] = { 
	 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 
	 0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87
};

uchar CBCiv[8] = {
	 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

uchar CBCdata[29] = {
	 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x20, 
	 0x4E, 0x6F, 0x77, 0x20, 0x69, 0x73, 0x20, 0x74, 
	 0x68, 0x65, 0x20, 0x74, 0x69, 0x6D, 0x65, 0x20, 
	 0x66, 0x6F, 0x72, 0x20, 0x00
};

uchar CBCcipher[32] = {
	 0x6B, 0x77, 0xB4, 0xD6, 0x30, 0x06, 0xDE, 0xE6,
	 0x05, 0xB1, 0x56, 0xE2, 0x74, 0x03, 0x97, 0x93,
	 0x58, 0xDE, 0xB9, 0xE7, 0x15, 0x46, 0x16, 0xD9,
	 0x59, 0xF1, 0x65, 0x2B, 0xD5, 0xFF, 0x92, 0xCC
};

int
testECB(Testvector *t)
{
	BFstate s;
	int i;
	uchar aux[Bsz];

	memcpy(aux, t->plain, Bsz);

	memset(&s, 0, sizeof(BFstate));
	setupBFstate(&s, t->key, Bsz, nil);
	bfECBencrypt(aux, Bsz, &s);

	if(memcmp(aux, t->cipher, Bsz) != 0){
		fprint(2, "ECB encrypt failed, ciphertext is:\n");
		for(i = 0; i < Bsz; i++)
			fprint(2, "%02X", aux[i]);
		fprint(2, "\nand should be:\n");
		for(i = 0; i < Bsz; i++)
			fprint(2, "%02X", t->cipher[i]);
		fprint(2, "\n");
		return -1;
	}

	memset(&s, 0, sizeof(BFstate));
	setupBFstate(&s, t->key, Bsz, nil);
	bfECBdecrypt(aux, Bsz, &s);

	if(memcmp(aux, t->plain, Bsz) != 0){
		fprint(2, "ECB decrypt failed, plaintext is:\n");
		for(i = 0; i < Bsz; i++)
			fprint(2, "%02X", aux[i]);
		fprint(2, "\nand should be:\n");
		for(i = 0; i < Bsz; i++)
			fprint(2, "%02X", t->plain[i]);
		fprint(2, "\n");
		return -1;
	}
	return 0;
}

int
testCBC(void)
{
	BFstate s;
	uchar aux[32];
	int i;

	memset(aux, 0 , sizeof(aux));
	memcpy(aux, CBCdata, sizeof(CBCdata));
	memset(&s, 0, sizeof(BFstate));
	setupBFstate(&s, CBCkey, sizeof(CBCkey), CBCiv);
	bfCBCencrypt(aux, 32, &s);
	
	if(memcmp(aux, CBCcipher, sizeof(CBCcipher)) != 0){
		fprint(2, "CBC encrypt failed, ciphertext is:\n");
		for(i = 0; i < sizeof(aux); i++)
			fprint(2, "%02X", aux[i]);
		fprint(2, "\nand should be:\n");
		for(i = 0; i < sizeof(CBCcipher); i++)
			fprint(2, "%02X", CBCcipher[i]);
		fprint(2, "\n");
		return -1;
	}

	memset(&s, 0, sizeof(BFstate));
	setupBFstate(&s, CBCkey, sizeof(CBCkey), CBCiv);
	bfCBCdecrypt(aux, 32, &s);

	if(memcmp(aux, CBCdata, sizeof(CBCdata)) != 0){
		fprint(2, "CBC decrypt failed, plaintext is:\n");
		for(i = 0; i < sizeof(aux); i++)
			fprint(2, "%02X", aux[i]);
		fprint(2, "\nand should be:\n");
		for(i = 0; i < sizeof(CBCdata); i++)
			fprint(2, "%02X", CBCdata[i]);
		fprint(2, "\n");
		return -1;
	}

	return 0;
}

void
main(int argc, char **argv)
{
	int i;

	if(argc != 1)
		sysfatal("usage: %s", argv[0]);

	for(i=0; i < nelem(vector); i++)
		if(testECB(&vector[i]) < 0)
			sysfatal("TestECB %d failed", i);

	if(testCBC() < 0)
		sysfatal("TestCBC failed");
	exits(nil);
}
