#include "lib9.h"
#include "draw.h"
#include "kernel.h"

typedef struct Memimage Memimage;

static int	screenid;

Screen*
allocscreen(Image *image, Image *fill, int public)
{
	uchar *a;
	Screen *s;
	int id, try;
	Display *d;

	d = image->display;
	if(d != fill->display){
		kwerrstr("allocscreen: image and fill on different displays");
		return 0;
	}
	s = malloc(sizeof(Screen));
	if(s == 0)
		return 0;
	SET(id);
	for(try=0; try<25; try++){
		/* loop until find a free id */
		a = bufimage(d, 1+4+4+4+1);
		if(a == 0){
			free(s);
			return 0;
		}
		id = ++screenid;
		a[0] = 'A';
		BP32INT(a+1, id);
		BP32INT(a+5, image->id);
		BP32INT(a+9, fill->id);
		a[13] = public;
		if(flushimage(d, 0) != -1)
			break;
	}
	s->display = d;
	s->id = id;
	s->image = image;
//	assert(s->image && s->image->chan != 0);

	s->fill = fill;
	return s;
}

Screen*
publicscreen(Display *d, int id, u32 chan)
{
	uchar *a;
	Screen *s;

	s = malloc(sizeof(Screen));
	if(s == 0)
		return 0;
	a = bufimage(d, 1+4+4);
	if(a == 0){
    Error:
		free(s);
		return 0;
	}
	a[0] = 'S';
	BP32INT(a+1, id);
	BP32INT(a+5, chan);
	if(flushimage(d, 0) < 0)
		goto Error;

	s->display = d;
	s->id = id;
	s->image = 0;
	s->fill = 0;
	return s;
}

int
freescreen(Screen *s)
{
	uchar *a;
	Display *d;

	if(s == 0)
		return 0;
	d = s->display;
	a = bufimage(d, 1+4);
	if(a == 0)
		return -1;
	a[0] = 'F';
	BP32INT(a+1, s->id);
	/*
	 * flush(1) because screen is likely holding last reference to
	 * window, and want it to disappear visually.
	 */
	if(flushimage(d, 1) < 0)
		return -1;
	free(s);
	return 1;
}

Image*
allocwindow(Screen *s, Rectangle r, int ref, u32 val)
{
	return _allocwindow(nil, s, r, ref, val);
}

Image*
_allocwindow(Image *i, Screen *s, Rectangle r, int ref, u32 val)
{
	Display *d;

	d = s->display;
	i = _allocimage(i, d, r, s->image->chan, 0, val, s->id, ref);
	if(i == 0)
		return 0;
	i->screen = s;
	i->next = s->display->windows;
	s->display->windows = i;
	return i;
}

static
void
topbottom(Image **w, int n, int top)
{
	int i;
	uchar *b;
	Display *d;

	if(n<0 || n>(Displaybufsize-100)/4){
		_drawprint(2, "top/bottom: ridiculous number of windows\n");
		return;
	}
	if(n==0)
		return;
	/* check that all images are on the same display; only it can check the screens */
	d = w[0]->display;
	for(i=1; i<n; i++)
		if(w[i]->display != d){
			_drawprint(2, "top/bottom: windows not on same display\n");
			return;
		}
	b = bufimage(d, 1+1+2+4*n);
	if (b == 0) {
		_drawprint(2, "top/bottom: no bufimage\n");
		return;
	}
	b[0] = 't';
	b[1] = top;
	BP16INT(b+2, n);
	for(i=0; i<n; i++)
		BP32INT(b+4+4*i, w[i]->id);
}

void
bottomwindow(Image *w)
{
	topbottom(&w, 1, 0);
}

void
topwindow(Image *w)
{
	topbottom(&w, 1, 1);
}

void
bottomnwindows(Image **w, int n)
{
	topbottom(w, n, 0);
}

void
topnwindows(Image **w, int n)
{
	topbottom(w, n, 1);
}

int
originwindow(Image *w, Point log, Point scr)
{
	uchar *b;
	Point delta;

	flushimage(w->display, 0);
	b = bufimage(w->display, 1+4+2*4+2*4);
	if(b == nil)
		return 0;
	b[0] = 'o';
	BP32INT(b+1, w->id);
	BP32INT(b+5, log.x);
	BP32INT(b+9, log.y);
	BP32INT(b+13, scr.x);
	BP32INT(b+17, scr.y);
	if(flushimage(w->display, 1) < 0)
		return -1;
	delta = subpt(log, w->r.min);
	w->r = rectaddpt(w->r, delta);
	w->clipr = rectaddpt(w->clipr, delta);
	return 1;
}
