/*
 * VGA controller
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

typedef struct Vgaseg Vgaseg;
struct Vgaseg {
	QLock;
	uintptr	pa;
	u32	len;
	void*	va;
};

enum {
	Nvgaseg = 4,

	Qdir = 0,
	Qvgactl,
	Qvgaovl,
	Qvgaovlctl,

	Qsegs,
	Qmax = Qsegs+Nvgaseg
};

static Dirtab vgadir[Qmax] = {
	".",	{ Qdir, 0, QTDIR },		0,	0550,
	"vgactl",		{ Qvgactl, 0 },		0,	0660,
	"vgaovl",		{ Qvgaovl, 0 },		0,	0660,
	"vgaovlctl",	{ Qvgaovlctl, 0 },	0, 	0660,
	/* dynamically-created memory segments are added here */
};

static Vgaseg vgasegs[Nvgaseg];
static Lock vgadirlock;
static int nvgadir = Qsegs;

enum {
	CMactualsize,
	CMblank,
	CMblanktime,
	CMdrawinit,
	CMhwaccel,
	CMhwblank,
	CMhwgc,
	CMlinear,
	CMpalettedepth,
	CMpanning,
	CMsize,
	CMtype,
	CMunblank,
};

static Cmdtab vgactlmsg[] = {
	CMactualsize,	"actualsize",	2,
	CMblank,	"blank",	1,
	CMblanktime,	"blanktime",	2,
	CMdrawinit,	"drawinit",	1,
	CMhwaccel,	"hwaccel",	2,
	CMhwblank,	"hwblank",	2,
	CMhwgc,		"hwgc",		2,
	CMlinear,	"linear",	0,
	CMpalettedepth,	"palettedepth",	2,
	CMpanning,	"panning",	2,
	CMsize,		"size",		3,
	CMtype,		"type",		2,
	CMunblank,	"unblank",	1,
};

static void
vgareset(void)
{
	/* reserve the 'standard' vga registers */
	if(ioalloc(0x2b0, 0x2df-0x2b0+1, 0, "vga") < 0)
		panic("vga ports already allocated"); 
	if(ioalloc(0x3c0, 0x3da-0x3c0+1, 0, "vga") < 0)
		panic("vga ports already allocated"); 
	conf.monitor = 1;
}

void
addvgaseg(char *name, u32 pa, u32 size)
{
	int i;
	Dirtab d;
	Vgaseg *s;
	uintptr va;

	va = mmukmap(pa, 0, size);
	if(va == 0)
		return;
	memset(&d, 0, sizeof(d));
	strecpy(d.name, d.name+sizeof(name), name);
	lock(&vgadirlock);
	for(i=0; i<nvgadir; i++)
		if(strcmp(vgadir[i].name, name) == 0){
			unlock(&vgadirlock);
			print("devvga: duplicate segment %s\n", name);
			return;
		}
	if(nvgadir >= nelem(vgadir)){
		unlock(&vgadirlock);
		print("devvga: segment %s: too many segments\n", name);
		return;
	}
	d.qid.path = nvgadir;
	d.perm = 0660;
	d.length = size;
	s = &vgasegs[nvgadir-Qsegs];
	s->pa = pa;
	s->len = size;
	s->va = (void*)va;
	vgadir[nvgadir] = d;
	nvgadir++;
	unlock(&vgadirlock);
}

static s32
vgasegrd(Vgaseg *s, uchar *buf, s32 n, u32 offset)
{
	int i;
	uchar *a, *d;
	uintptr v;

	if(offset >= s->len)
		return 0;
	if(offset+n > s->len)
		n = s->len - offset;
	d = (uchar*)s->va + offset;
	qlock(s);
	if(waserror()){
		qunlock(s);
		nexterror();
	}
	a = buf;
	while(n > 0){
		i = 4 - ((uintptr)d & 3);
		if(i > n)
			i = n;
		if(i == 3)
			i = 2;
		switch(i){
		case 4:
			v = (a[3]<<24) | (a[2]<<16) | (a[1]<<8) | a[0];
			*(ulong*)d = v;
			break;
		case 2:
			v = (a[1]<<8) | a[0];
			*(ushort*)d = v;
			break;
		case 1:
			*d = *a;
			break;
		}
		d += i;
		a += i;
		n -= i;
	}
	poperror();
	qunlock(s);
	return a-buf;
}

static s32
vgasegwr(Vgaseg *s, uchar *buf, s32 n, u32 offset)
{
	int i;
	uchar *a, *r;
	uintptr v;

	if(offset >= s->len)
		return 0;
	if(offset+n > s->len)
		n = s->len - offset;
	r = (uchar*)s->va + offset;
	qlock(s);
	if(waserror()){
		qunlock(s);
		nexterror();
	}
	a = buf;
	while(n > 0){
		i = 4 - ((uintptr)r & 3);
		if(i > n)
			i = n;
		if(i == 3)
			i = 2;
		switch(i){
		case 4:
			v = *(ulong*)r;
			a[0] = v;
			a[1] = v>>8;
			a[2] = v>>16;
			a[3] = v>>24;
			break;
		case 2:
			v = *(ushort*)r;
			a[0] = v;
			a[1] = v>>8;
			break;
		case 1:
			*a = *r;
			break;
		}
		r += i;
		a += i;
		n -= i;
	}
	poperror();
	qunlock(s);
	return a-buf;
}

static Chan*
vgaattach(char* spec)
{
	if(*spec && strcmp(spec, "0"))
		error(Eio);
	return devattach('v', spec);
}

Walkqid*
vgawalk(Chan* c, Chan *nc, char** name, s32 nname)
{
	return devwalk(c, nc, name, nname, vgadir, nvgadir, devgen);
}

static int
vgastat(Chan* c, uchar* dp, s32 n)
{
	return devstat(c, dp, n, vgadir, nvgadir, devgen);
}

static Chan*
vgaopen(Chan* c, u32 omode)
{
	VGAscr *scr;
	static char *openctl = "openctl\n";

	scr = &vgascreen[0];
	if ((u64)c->qid.path == Qvgaovlctl) {
		if (scr->dev && scr->dev->ovlctl)
			scr->dev->ovlctl(scr, c, openctl, strlen(openctl));
		else 
			error(Enonexist);
	}
	return devopen(c, omode, vgadir, nvgadir, devgen);
}

static void
vgaclose(Chan* c)
{
	VGAscr *scr;
	static char *closectl = "closectl\n";

	scr = &vgascreen[0];
	if((u64)c->qid.path == Qvgaovlctl)
		if(scr->dev && scr->dev->ovlctl){
			if(waserror()){
				print("ovlctl error: %s\n", up->errstr);
				return;
			}
			scr->dev->ovlctl(scr, c, closectl, strlen(closectl));
			poperror();
		}
}

static void
checkport(int start, int end)
{
	/* standard vga regs are OK */
	if(start >= 0x2b0 && end <= 0x2df+1)
		return;
	if(start >= 0x3c0 && end <= 0x3da+1)
		return;

	if(iounused(start, end))
		return;
	error(Eperm);
}

static s32
vgaread(Chan* c, void* a, s32 n, s64 off)
{
	int len;
	char *p, *s;
	VGAscr *scr;
	u32 offset = off;
	char chbuf[30];

	switch((u64)c->qid.path){

	case Qdir:
		return devdirread(c, a, n, vgadir, nvgadir, devgen);

	case Qvgactl:
		scr = &vgascreen[0];

		p = malloc(READSTR);
		if(waserror()){
			free(p);
			nexterror();
		}

		len = 0;

		if(scr->dev)
			s = scr->dev->name;
		else
			s = "cga";
		len += snprint(p+len, READSTR-len, "type %s\n", s);

		if(scr->gscreen) {
			len += snprint(p+len, READSTR-len, "size %dx%dx%d %s\n",
				scr->gscreen->r.max.x, scr->gscreen->r.max.y,
				scr->gscreen->depth, chantostr(chbuf, scr->gscreen->chan));

			if(Dx(scr->gscreen->r) != Dx(physgscreenr) 
			|| Dy(scr->gscreen->r) != Dy(physgscreenr))
				len += snprint(p+len, READSTR-len, "actualsize %dx%d\n",
					physgscreenr.max.x, physgscreenr.max.y);
		}

		len += snprint(p+len, READSTR-len, "blank time %ud idle %d state %s\n",
			blanktime, drawidletime(), scr->isblank ? "off" : "on");
		len += snprint(p+len, READSTR-len, "hwaccel %s\n", hwaccel ? "on" : "off");
		len += snprint(p+len, READSTR-len, "hwblank %s\n", hwblank ? "on" : "off");
		len += snprint(p+len, READSTR-len, "panning %s\n", panning ? "on" : "off");
		snprint(p+len, READSTR-len, "addr 0x%zux\n", scr->aperture);
		n = readstr(offset, a, n, p);
		poperror();
		free(p);

		return n;

	case Qvgaovl:
	case Qvgaovlctl:
		error(Ebadusefd);
		break;

	default:
		if(c->qid.path < nvgadir)
			return vgasegrd(&vgasegs[c->qid.path], a, n, offset);
		error(Egreg);
		break;
	}

	return 0;
}

static char Ebusy[] = "vga already configured";

static void
vgactl(Cmdbuf *cb)
{
	int align, i, size, x, y, z;
	char *chanstr, *p;
	u32 chan;
	Cmdtab *ct;
	VGAscr *scr;
	extern VGAdev *vgadev[];
	extern VGAcur *vgacur[];

	scr = &vgascreen[0];
	ct = lookupcmd(cb, vgactlmsg, nelem(vgactlmsg));
	switch(ct->index){
	case CMhwgc:
		if(strcmp(cb->f[1], "off") == 0){
			lock(&cursor);
			if(scr->cur){
				if(scr->cur->disable)
					scr->cur->disable(scr);
				scr->cur = nil;
			}
			unlock(&cursor);
			return;
		}

		for(i = 0; vgacur[i]; i++){
			if(strcmp(cb->f[1], vgacur[i]->name))
				continue;
			lock(&cursor);
			if(scr->cur && scr->cur->disable)
				scr->cur->disable(scr);
			scr->cur = vgacur[i];
			if(scr->cur->enable)
				scr->cur->enable(scr);
			unlock(&cursor);
			return;
		}
		break;

	case CMtype:
		for(i = 0; vgadev[i]; i++){
			if(strcmp(cb->f[1], vgadev[i]->name))
				continue;
			if(scr->dev && scr->dev->disable)
				scr->dev->disable(scr);
			scr->dev = vgadev[i];
			if(scr->dev->enable)
				scr->dev->enable(scr);
			return;
		}
		break;

	case CMsize:
		/*TODO if(drawhasclients())
			error(Ebusy);*/

		x = strtoul(cb->f[1], &p, 0);
		if(x == 0 || x > 2048)
			error(Ebadarg);
		if(*p)
			p++;

		y = strtoul(p, &p, 0);
		if(y == 0 || y > 2048)
			error(Ebadarg);
		if(*p)
			p++;

		z = strtoul(p, &p, 0);

		chanstr = cb->f[2];
		if((chan = strtochan(chanstr)) == 0)
			error("bad channel");

		if(chantodepth(chan) != z)
			error("depth, channel do not match");

		cursoroff();
		deletescreenimage();
		/* TODO if(screensize(x, y, z, chan))
			error(Egreg); */
		vgascreenwin(scr);
		cursoron();
		return;

	case CMactualsize:
		if(scr->gscreen == nil)
			error("set the screen size first");

		x = strtoul(cb->f[1], &p, 0);
		if(x == 0 || x > 2048)
			error(Ebadarg);
		if(*p)
			p++;

		y = strtoul(p, nil, 0);
		if(y == 0 || y > 2048)
			error(Ebadarg);

		if(x > scr->gscreen->r.max.x || y > scr->gscreen->r.max.y)
			error("physical screen bigger than virtual");

		physgscreenr = Rect(0,0,x,y);
		scr->gscreen->clipr = physgscreenr;
		return;
	
	case CMpalettedepth:
		x = strtoul(cb->f[1], &p, 0);
		if(x != 8 && x != 6)
			error(Ebadarg);

		scr->palettedepth = x;
		return;

	case CMdrawinit:
		memimagedraw(scr->gscreen, scr->gscreen->r, memblack, ZP, nil, ZP, S);
		if(scr && scr->dev && scr->dev->drawinit)
			scr->dev->drawinit(scr);
		return;
	
	case CMlinear:
		if(cb->nf!=2 && cb->nf!=3)
			error(Ebadarg);
		size = strtoul(cb->f[1], 0, 0);
		if(cb->nf == 2)
			align = 0;
		else
			align = strtoul(cb->f[2], 0, 0);
		if(screenaperture(scr, size, align))
			error("not enough free address space");
		return;

	case CMblank:
		drawblankscreen(1);
		return;
	
	case CMunblank:
		drawblankscreen(0);
		return;
	
	case CMblanktime:
		blanktime = strtoul(cb->f[1], 0, 0);
		return;

	case CMpanning:
		if(strcmp(cb->f[1], "on") == 0){
			if(scr == nil || scr->cur == nil)
				error("set screen first");
			if(!scr->cur->doespanning)
				error("panning not supported");
			scr->gscreen->clipr = scr->gscreen->r;
			panning = 1;
		}
		else if(strcmp(cb->f[1], "off") == 0){
			scr->gscreen->clipr = physgscreenr;
			panning = 0;
		}else
			break;
		return;

	case CMhwaccel:
		if(strcmp(cb->f[1], "on") == 0)
			hwaccel = 1;
		else if(strcmp(cb->f[1], "off") == 0)
			hwaccel = 0;
		else
			break;
		return;
	
	case CMhwblank:
		if(strcmp(cb->f[1], "on") == 0)
			hwblank = 1;
		else if(strcmp(cb->f[1], "off") == 0)
			hwblank = 0;
		else
			break;
		return;
	}

	cmderror(cb, "bad VGA control message");
}

char Enooverlay[] = "No overlay support";

static s32
vgawrite(Chan* c, void* a, s32 n, s64 off)
{
	uintptr offset = off;
	Cmdbuf *cb;
	VGAscr *scr;

	switch((u64)c->qid.path){

	case Qdir:
		error(Eperm);

	case Qvgactl:
		if(offset || n >= READSTR)
			error(Ebadarg);
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		vgactl(cb);
		poperror();
		free(cb);
		return n;

	case Qvgaovl:
		scr = &vgascreen[0];
		if (scr->dev == nil || scr->dev->ovlwrite == nil) {
			error(Enooverlay);
			break;
		}
		return scr->dev->ovlwrite(scr, a, n, off);

	case Qvgaovlctl:
		scr = &vgascreen[0];
		if (scr->dev == nil || scr->dev->ovlctl == nil) {
			error(Enooverlay);
			break;
		}
		scr->dev->ovlctl(scr, c, a, n);
		return n;

	default:
		if(c->qid.path < nvgadir)
			return vgasegwr(&vgasegs[c->qid.path], a, n, offset);
		error(Egreg);
		break;
	}

	return 0;
}

Dev vgadevtab = {
	'v',
	"vga",

	vgareset,
	devinit,
	devshutdown,
	vgaattach,
	vgawalk,
	vgastat,
	vgaopen,
	devcreate,
	vgaclose,
	vgaread,
	devbread,
	vgawrite,
	devbwrite,
	devremove,
	devwstat,
};
