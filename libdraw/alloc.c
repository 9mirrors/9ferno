#include "lib9.h"
#include "draw.h"
#include "kernel.h"

#define DP if(1){}else print

Image*
allocimage(Display *d, Rectangle r, u32 chan, int repl, u32 val)
{
	return _allocimage(nil, d, r, chan, repl, val, 0, 0);
}

Image*
_allocimage(Image *ai, Display *d, Rectangle r, u32 chan, int repl, u32 val, int screenid, int refresh)
{
	uchar *a;
	char *err;
	Image *i;
	Rectangle clipr;
	int id;
	int depth;

	err = 0;
	i = 0;

	if(chan == 0){
		kwerrstr("bad channel descriptor");
		return nil;
	}

	depth = chantodepth(chan);
	if(depth == 0){
		err = "bad channel descriptor";
    Error:
		if(err)
			kwerrstr("allocimage: %s", err);
		else
			kwerrstr("allocimage: %r");
		free(i);
		return 0;
	}

	/* flush pending data so we don't get error allocating the image */
	flushimage(d, 0);
	a = bufimage(d, 1+4+4+1+4+1+4*4+4*4+4);
	if(a == 0)
		goto Error;
	d->imageid++;
	id = d->imageid;
	a[0] = 'b';
	BP32INT(a+1, id);
	BP32INT(a+5, screenid);
	a[9] = refresh;
	BP32INT(a+10, chan);
	a[14] = repl;
	BP32INT(a+15, r.min.x);
	BP32INT(a+19, r.min.y);
	BP32INT(a+23, r.max.x);
	BP32INT(a+27, r.max.y);
	if(repl)
		/* huge but not infinite, so various offsets will leave it huge, not overflow */
		clipr = Rect(-0x3FFFFFFF, -0x3FFFFFFF, 0x3FFFFFFF, 0x3FFFFFFF);
	else
		clipr = r;
	BP32INT(a+31, clipr.min.x);
	BP32INT(a+35, clipr.min.y);
	BP32INT(a+39, clipr.max.x);
	BP32INT(a+43, clipr.max.y);
	BP32INT(a+47, val);
	DP("_allocimage id %d screenid %d r %R clipr %R\n", id, screenid, r, clipr);
	if(flushimage(d, 0) < 0)
		goto Error;

	if(ai)
		i = ai;
	else{
		i = malloc(sizeof(Image));
		if(i == nil){
			a = bufimage(d, 1+4);
			if(a){
				a[0] = 'f';
				BP32INT(a+1, id);
				flushimage(d, 0);
			}
			goto Error;
		}
	}
	i->display = d;
	i->id = id;
	i->depth = depth;
	i->chan = chan;
	i->r = r;
	i->clipr = clipr;
	i->repl = repl;
	i->screen = 0;
	i->next = 0;
	return i;
}

Image*
namedimage(Display *d, char *name)
{
	uchar *a;
	char *err, buf[12*12+1];
	Image *i;
	int id, n;
	u32 chan;

	err = 0;
	i = 0;

	n = strlen(name);
	if(n >= 256){
		err = "name too long";
    Error:
		if(err)
			kwerrstr("namedimage: %s", err);
		else
			kwerrstr("namedimage: %r");
		if(i)
			free(i);
		return 0;
	}
	/* flush pending data so we don't get error allocating the image */
	flushimage(d, 0);
	a = bufimage(d, 1+4+1+n);
	if(a == 0)
		goto Error;
	d->imageid++;
	id = d->imageid;
	a[0] = 'n';
	BP32INT(a+1, id);
	a[5] = n;
	memmove(a+6, name, n);
	if(flushimage(d, 0) < 0)
		goto Error;

	if(kchanio(d->ctlchan, buf, sizeof buf, OREAD) < 12*12)
		goto Error;
	buf[12*12] = '\0';

	i = malloc(sizeof(Image));
	if(i == nil){
	Error1:
		a = bufimage(d, 1+4);
		if(a){
			a[0] = 'f';
			BP32INT(a+1, id);
			flushimage(d, 0);
		}
		goto Error;
	}
	i->display = d;
	i->id = id;
	if((chan=strtochan(buf+2*12))==0){
		kwerrstr("bad channel from devdraw");
		goto Error1;
	}
	i->chan = chan;
	i->depth = chantodepth(chan);
	i->repl = atoi(buf+3*12);
	i->r.min.x = atoi(buf+4*12);
	i->r.min.y = atoi(buf+5*12);
	i->r.max.x = atoi(buf+6*12);
	i->r.max.y = atoi(buf+7*12);
	i->clipr.min.x = atoi(buf+8*12);
	i->clipr.min.y = atoi(buf+9*12);
	i->clipr.max.x = atoi(buf+10*12);
	i->clipr.max.y = atoi(buf+11*12);
	i->screen = 0;
	i->next = 0;
	return i;
}

int
nameimage(Image *i, char *name, int in)
{
	uchar *a;
	int n;

	n = strlen(name);
	a = bufimage(i->display, 1+4+1+1+n);
	if(a == 0)
		return 0;
	a[0] = 'N';
	BP32INT(a+1, i->id);
	a[5] = in;
	a[6] = n;
	memmove(a+7, name, n);
	if(flushimage(i->display, 0) < 0)
		return 0;
	return 1;
}

int
_freeimage1(Image *i)
{
	uchar *a;
	Display *d;
	Image *w;

	if(i == 0)
		return 0;
	/* make sure no refresh events occur on this if we block in the write */
	d = i->display;
	/* flush pending data so we don't get error deleting the image */
	flushimage(d, 0);
	a = bufimage(d, 1+4);
	if(a == 0)
		return -1;
	a[0] = 'f';
	BP32INT(a+1, i->id);
	if(i->screen){
		w = d->windows;
		if(w == i)
			d->windows = i->next;
		else
			while(w){
				if(w->next == i){
					w->next = i->next;
					break;
				}
				w = w->next;
			}
	}
	if(flushimage(d, i->screen!=0) < 0)
		return -1;

	return 0;
}

int
freeimage(Image *i)
{
	int ret;

	ret = _freeimage1(i);
	free(i);
	return ret;
}
