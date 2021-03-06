#include "lib9.h"
#include "draw.h"
#include "kernel.h"

int
cloadimage(Image *i, Rectangle r, uchar *data, int ndata)
{
	int m, nb, miny, maxy, ncblock;
	uchar *a;

	if(!rectinrect(r, i->r)){
		werrstr("cloadimage: bad rectangle");
		return -1;
	}

	miny = r.min.y;
	m = 0;
	ncblock = _compblocksize(r, i->depth);
	while(miny != r.max.y){
		maxy = atoi((char*)data+0*12);
		nb = atoi((char*)data+1*12);
		if(maxy<=miny || r.max.y<maxy){
			werrstr("creadimage: bad maxy %d", maxy);
			return -1;
		}
		data += 2*12;
		ndata -= 2*12;
		m += 2*12;
		if(nb<=0 || ncblock<nb || nb>ndata){
			werrstr("creadimage: bad count %d", nb);
			return -1;
		}
		a = bufimage(i->display, 21+nb);
		if(a == nil)
			return -1;
		a[0] = 'Y';
		BP32INT(a+1, i->id);
		BP32INT(a+5, r.min.x);
		BP32INT(a+9, miny);
		BP32INT(a+13, r.max.x);
		BP32INT(a+17, maxy);
		memmove(a+21, data, nb);
		miny = maxy;
		data += nb;
		ndata += nb;
		m += nb;
	}
	return m;
}
