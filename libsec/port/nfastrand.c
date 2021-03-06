#include "os.h"
#include "libsec.h"

#define Maxrand	((1UL<<31)-1)

u32
nfastrand(u32 n)
{
	u32 m, r;
	
	/*
	 * set m to the maximum multiple of n <= 2^31-1
	 * so we want a random number < m.
	 */
	if(n > Maxrand)
		sysfatal("nfastrand: n too large");

	m = Maxrand - Maxrand % n;
	while((r = fastrand()) >= m)
		;
	return r%n;
}
