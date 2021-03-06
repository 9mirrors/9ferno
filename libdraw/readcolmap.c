#include "lib9.h"
#include "draw.h"
#include "bio.h"

static u32
getval(char **p)
{
	u32 v;
	char *q;

	v = strtoul(*p, &q, 0);
	v |= v<<8;
	v |= v<<16;
	*p = q;
	return v;
}

void
readcolmap(Display *d, RGB *colmap)
{
	int i;
	char *p, *q;
	Biobuf *b;
	char buf[128];

	sprint(buf, "/dev/draw/%d/colormap", d->dirno);
	b = Bopen(buf, OREAD);
	if(b == 0)
		drawerror(d, "rdcolmap: can't open colormap device");

	for(;;) {
		p = Brdline(b, '\n');
		if(p == 0)
			break;
		i = strtoul(p, &q, 0);
		if(i < 0 || i > 255) {
			_drawprint(2, "rdcolmap: bad index\n");
			exits("bad");
		}
		p = q;
		colmap[255-i].red = getval(&p);
		colmap[255-i].green = getval(&p);
		colmap[255-i].blue = getval(&p);
	}
	Bterm(b);
}
