/*
 *  devssl - secure sockets layer
 */
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

#include	"mp.h"
#include	"libsec.h"

typedef struct OneWay OneWay;
struct OneWay
{
	QLock	q;
	QLock	ctlq;

	void	*state;		/* encryption state */
	int	slen;			/* secret data length */
	uchar	*secret;	/* secret */
	ulong	mid;		/* message id */
};

enum
{
	/* connection states */
	Sincomplete=	0,
	Sclear=		1,
	Sencrypting=	2,
	Sdigesting=	4,
	Sdigenc=	Sencrypting|Sdigesting,

	/* encryption algorithms */
	Noencryption=	0,
	DESCBC=		1,
	DESECB=		2,
	RC4=		3,
	IDEACBC=	4,
	IDEAECB=		5
};

typedef struct Dstate Dstate;
struct Dstate
{
	Chan	*c;			/* io channel */
	uchar	state;		/* state of connection */
	int	ref;			/* serialized by dslock for atomic destroy */

	uchar	encryptalg;	/* encryption algorithm */
	ushort	blocklen;	/* blocking length */

	ushort	diglen;		/* length of digest */
	DigestState *(*hf)(uchar*, u32, uchar*, DigestState*);	/* hash func */

	/* for SSL format */
	int	max;			/* maximum unpadded data per msg */
	int	maxpad;			/* maximum padded data per msg */
	
	/* input side */
	OneWay	in;
	Block	*processed;
	Block	*unprocessed;

	/* output side */
	OneWay	out;

	/* protections */
	char*	user;
	int	perm;
};

enum
{
	Maxdmsg=	1<<16,
	Maxdstate=	1<<10,
};

Lock	dslock;
int	dshiwat;
int	maxdstate = 20;
Dstate** dstate;

enum{
	Qtopdir		= 1,	/* top level directory */
	Qclonus,
	Qconvdir,			/* directory for a conversation */
	Qdata,
	Qctl,
	Qsecretin,
	Qsecretout,
	Qencalgs,
	Qhashalgs
};

#define TYPE(x) 	((ulong)(x).path & 0xf)
#define CONV(x) 	(((ulong)(x).path >> 4)&(Maxdstate-1))
#define QID(c, y) 	(((c)<<4) | (y))

static char*	encalgs;
static char*	hashalgs;

void producerand(void);

static void alglistinit(void);
static void	ensure(Dstate*, Block**, int);
static void	consume(Block**, uchar*, int);
static void	setsecret(OneWay*, uchar*, int);
static Block*	encryptb(Dstate*, Block*, int);
static Block*	decryptb(Dstate*, Block*);
static Block*	digestb(Dstate*, Block*, int);
static void	checkdigestb(Dstate*, Block*);
static Chan*	buftochan(char*);
static void	sslhangup(Dstate*);
static void dsclone(Chan *c);
static void	dsnew(Chan *c, Dstate **);

static int
sslgen(Chan *c, char *dname, Dirtab *d, int nd, int s, Dir *dp)
{
	Qid q;
	Dstate *ds;
	char *p, *nm;

	USED(dname);
	USED(nd);
	USED(d);
	q.type = QTFILE;
	q.vers = 0;
	if(s == DEVDOTDOT){
		q.path = QID(0, Qtopdir);
		q.type = QTDIR;
		devdir(c, q, "#D", 0, eve, 0555, dp);
		return 1;
	}
	switch(TYPE(c->qid)) {
	case Qtopdir:
		if(s < dshiwat) {
			q.path = QID(s, Qconvdir);
			q.type = QTDIR;
			ds = dstate[s];
			if(ds != 0)
 				nm = ds->user;
 			else
 				nm = eve;
			snprint(up->genbuf, sizeof(up->genbuf), "%d", s);
			devdir(c, q, up->genbuf, 0, nm, DMDIR|0555, dp);
			return 1;
		}
		if(s > dshiwat)
			return -1;
		/* fall through */
	case Qclonus:
		q.path = QID(0, Qclonus);
		devdir(c, q, "clone", 0, eve, 0666, dp);
		return 1;
	case Qconvdir:
		ds = dstate[CONV(c->qid)];
		if(ds != 0)
			nm = ds->user;
		else
			nm = eve;
		switch(s) {
		default:
			return -1;
		case 0:
			q.path = QID(CONV(c->qid), Qctl);
			p = "ctl";
			break;
		case 1:
			q.path = QID(CONV(c->qid), Qdata);
			p = "data";
			break;
		case 2:
			q.path = QID(CONV(c->qid), Qsecretin);
			p = "secretin";
			break;
		case 3:
			q.path = QID(CONV(c->qid), Qsecretout);
			p = "secretout";
			break;
		case 4:
			q.path = QID(CONV(c->qid), Qencalgs);
			p = "encalgs";
			break;
		case 5:
			q.path = QID(CONV(c->qid), Qhashalgs);
			p = "hashalgs";
			break;
		}
		devdir(c, q, p, 0, nm, 0660, dp);
		return 1;
	}
	return -1;
}

static void
sslinit(void)
{
	if((dstate = malloc(sizeof(Dstate*) * maxdstate)) == 0)
		panic("sslinit");
	alglistinit();
}

static Chan *
sslattach(char *spec)
{
	Chan *c;

	c = devattach('D', spec);
	c->qid.path = QID(0, Qtopdir);
	c->qid.vers = 0;
	c->qid.type = QTDIR;
	return c;
}

static Walkqid*
sslwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, sslgen);
}

static int
sslstat(Chan *c, uchar *db, int n)
{
	return devstat(c, db, n, 0, 0, sslgen);
}

static Chan*
sslopen(Chan *c, int omode)
{
	Dstate *s, **pp;
	int perm;

	perm = 0;
	omode &= 3;
	switch(omode) {
	case OREAD:
		perm = 4;
		break;
	case OWRITE:
		perm = 2;
		break;
	case ORDWR:
		perm = 6;
		break;
	}

	switch(TYPE(c->qid)) {
	default:
		panic("sslopen");
	case Qtopdir:
	case Qconvdir:
		if(omode != OREAD)
			error(Eperm);
		break;
	case Qclonus:
		dsclone(c);
		break;
	case Qctl:
	case Qdata:
	case Qsecretin:
	case Qsecretout:
		if(waserror()) {
			unlock(&dslock);
			nexterror();
		}
		lock(&dslock);
		pp = &dstate[CONV(c->qid)];
		s = *pp;
		if(s == 0)
			dsnew(c, pp);
		else {
			if((perm & (s->perm>>6)) != perm
			   && (strcmp(up->env->user, s->user) != 0
			     || (perm & s->perm) != perm))
				error(Eperm);

			s->ref++;
		}
		unlock(&dslock);
		poperror();
		break;
	case Qencalgs:
	case Qhashalgs:
		if(omode != OREAD)
			error(Eperm);
		break;
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static int
sslwstat(Chan *c, uchar *db, int n)
{
	Dir *dir;
	Dstate *s;
	int m;

	s = dstate[CONV(c->qid)];
	if(s == 0)
		error(Ebadusefd);
	if(strcmp(s->user, up->env->user) != 0)
		error(Eperm);

	dir = smalloc(sizeof(Dir)+n);
	m = convM2D(db, n, &dir[0], (char*)&dir[1]);
	if(m == 0){
		free(dir);
		error(Eshortstat);
	}

	if(!emptystr(dir->uid))
		kstrdup(&s->user, dir->uid);
	if(dir->mode != ~0UL)
		s->perm = dir->mode;

	free(dir);
	return m;
}

static void
sslclose(Chan *c)
{
	Dstate *s;

	switch(TYPE(c->qid)) {
	case Qctl:
	case Qdata:
	case Qsecretin:
	case Qsecretout:
		if((c->flag & COPEN) == 0)
			break;

		s = dstate[CONV(c->qid)];
		if(s == 0)
			break;

		lock(&dslock);
		if(--s->ref > 0) {
			unlock(&dslock);
			break;
		}
		dstate[CONV(c->qid)] = 0;
		unlock(&dslock);

		sslhangup(s);
		if(s->c)
			cclose(s->c);
		free(s->user);
		free(s->in.secret);
		free(s->out.secret);
		free(s->in.state);
		free(s->out.state);
		free(s);
	}
}

/*
 *  make sure we have at least 'n' bytes in list 'l'
 */
static void
ensure(Dstate *s, Block **l, int n)
{
	int sofar, i;
	Block *b, *bl;

	sofar = 0;
	for(b = *l; b; b = b->next){
		sofar += BLEN(b);
		if(sofar >= n)
			return;
		l = &b->next;
	}

	while(sofar < n){
		bl = devtab[s->c->type]->bread(s->c, Maxdmsg, 0);
		if(bl == 0)
			error(Ehungup);
		*l = bl;
		i = 0;
		for(b = bl; b; b = b->next){
			i += BLEN(b);
			l = &b->next;
		}
		if(i == 0)
			error(Ehungup);

		sofar += i;
	}
}

/*
 *  copy 'n' bytes from 'l' into 'p' and free
 *  the bytes in 'l'
 */
static void
consume(Block **l, uchar *p, int n)
{
	Block *b;
	int i;

	for(; *l && n > 0; n -= i){
		b = *l;
		i = BLEN(b);
		if(i > n)
			i = n;
		memmove(p, b->rp, i);
		b->rp += i;
		p += i;
		if(BLEN(b) < 0)
			panic("consume");
		if(BLEN(b))
			break;
		*l = b->next;
		freeb(b);
	}
}

/*
 *  remove at most n bytes from the queue, if discard is set
 *  dump the remainder
 */
static Block*
qtake(Block **l, int n, int discard)
{
	Block *nb, *b, *first;
	int i;

	first = *l;
	for(b = first; b; b = b->next){
		i = BLEN(b);
		if(i == n){
			if(discard){
				freeblist(b->next);
				*l = 0;
			} else
				*l = b->next;
			b->next = 0;
			return first;
		} else if(i > n){
			i -= n;
			if(discard){
				freeblist(b->next);
				*l = 0;
			} else {
				nb = allocb(i);
				memmove(nb->wp, b->rp+n, i);
				nb->wp += i;
				nb->next = b->next;
				*l = nb;
			}
			b->wp -= i;
			b->next = 0;
			if(BLEN(b) < 0)
				panic("qtake");
			return first;
		} else
			n -= i;
		if(BLEN(b) < 0)
			panic("qtake");
	}
	*l = 0;
	return first;
}

static Block*
sslbread(Chan *c, long n, ulong offset)
{
	volatile struct { Dstate *s; } s;
	volatile struct { int nc; } nc;
	Block *b;
	uchar count[3];
	int len, pad;

	USED(offset);
	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslbread");
	if(s.s->state == Sincomplete)
		error(Ebadusefd);

	nc.nc = 0;
	if(waserror()){
		qunlock(&s.s->in.q);
		if(strcmp(up->env->errstr, "interrupted") == 0){
			if(nc.nc > 0){
				b = allocb(nc.nc);
				memmove(b->wp, count, nc.nc);
				b->wp += nc.nc;
				b->next = s.s->unprocessed;
				s.s->unprocessed = b;
			}
		} else
			sslhangup(s.s);
		nexterror();
	}
	qlock(&s.s->in.q);

	if(s.s->processed == 0){
		/* read in the whole message */
		ensure(s.s, &s.s->unprocessed, 2);

		consume(&s.s->unprocessed, count, 2);
		nc.nc += 2;
		if(count[0] & 0x80){
			len = ((count[0] & 0x7f)<<8) | count[1];
			ensure(s.s, &s.s->unprocessed, len);
			pad = 0;
		} else {
			len = ((count[0] & 0x3f)<<8) | count[1];
			ensure(s.s, &s.s->unprocessed, len+1);
			consume(&s.s->unprocessed, count + nc.nc, 1);
			pad = count[nc.nc];
			nc.nc++;
			if(pad > len){
				print("pad %d buf len %d\n", pad, len);
				error("bad pad in ssl message");
			}
		}
		nc.nc = 0;

		/* put extra on unprocessed queue */
		s.s->processed = qtake(&s.s->unprocessed, len, 0);

		if(waserror()){
			qunlock(&s.s->in.ctlq);
			nexterror();
		}
		qlock(&s.s->in.ctlq);
		switch(s.s->state){
		case Sencrypting:
			s.s->processed = decryptb(s.s, s.s->processed);
			break;
		case Sdigesting:
			s.s->processed = pullupblock(s.s->processed, s.s->diglen);
			if(s.s->processed == 0)
				error("ssl message too short");
			checkdigestb(s.s, s.s->processed);
			s.s->processed->rp += s.s->diglen;
			break;
		case Sdigenc:
			s.s->processed = decryptb(s.s, s.s->processed);
			s.s->processed = pullupblock(s.s->processed, s.s->diglen);
			if(s.s->processed == 0)
				error("ssl message too short");
			checkdigestb(s.s, s.s->processed);
			s.s->processed->rp += s.s->diglen;
			len -= s.s->diglen;
			break;
		}
		s.s->in.mid++;
		qunlock(&s.s->in.ctlq);
		poperror();

		/* remove pad */
		if(pad)
			s.s->processed = qtake(&s.s->processed, len - pad, 1);
	}

	/* return at most what was asked for */
	b = qtake(&s.s->processed, n, 0);

	qunlock(&s.s->in.q);
	poperror();

	return b;
}

static long
sslread(Chan *c, void *a, long n, vlong offset)
{
	volatile struct { Block *b; } b;
	Block *nb;
	uchar *va;
	int i;
	char buf[128];

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, sslgen);

	switch(TYPE(c->qid)) {
	default:
		error(Ebadusefd);
	case Qctl:
		sprint(buf, "%ld", CONV(c->qid));
		return readstr(offset, a, n, buf);
	case Qdata:
		b.b = sslbread(c, n, offset);
		break;
	case Qencalgs:
		return readstr(offset, a, n, encalgs);
	case Qhashalgs:
		return readstr(offset, a, n, hashalgs);
	}

	n = 0;
	va = a;
	for(nb = b.b; nb; nb = nb->next){
		i = BLEN(nb);
		memmove(va+n, nb->rp, i);
		n += i;
	}

	freeblist(b.b);

	return n;
}

/*
 *  this algorithm doesn't have to be great since we're just
 *  trying to obscure the block fill
 */
static void
randfill(uchar *buf, int len)
{
	while(len-- > 0)
		*buf++ = nrand(256);
}

/*
 *  use SSL record format, add in count and digest or encrypt
 */
static long
sslbwrite(Chan *c, Block *b, ulong offset)
{
	volatile struct { Dstate *s; } s;
	volatile struct { Block *b; } bb;
	Block *nb;
	int h, n, m, pad, rv;
	uchar *p;

	bb.b = b;
	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslbwrite");
	if(s.s->state == Sincomplete){
		freeb(b);
		error(Ebadusefd);
	}

	if(waserror()){
		qunlock(&s.s->out.q);
		if(bb.b)
			freeb(bb.b);
		sslhangup(s.s);
		nexterror();
	}
	qlock(&s.s->out.q);

	rv = 0;
	while(bb.b){
		m = n = BLEN(bb.b);
		h = s.s->diglen + 2;

		/* trim to maximum block size */
		pad = 0;
		if(m > s.s->max){
			m = s.s->max;
		} else if(s.s->blocklen != 1){
			pad = (m + s.s->diglen)%s.s->blocklen;
			if(pad){
				if(m > s.s->maxpad){
					pad = 0;
					m = s.s->maxpad;
				} else {
					pad = s.s->blocklen - pad;
					h++;
				}
			}
		}

		rv += m;
		if(m != n){
			nb = allocb(m + h + pad);
			memmove(nb->wp + h, bb.b->rp, m);
			nb->wp += m + h;
			bb.b->rp += m;
		} else {
			/* add header space */
			nb = padblock(bb.b, h);
			bb.b = 0;
		}
		m += s.s->diglen;

		/* SSLv2 style count */
		if(pad){
			nb = padblock(nb, -pad);
			randfill(nb->wp, pad);
			nb->wp += pad;
			m += pad;

			p = nb->rp;
			p[0] = (m>>8);
			p[1] = m;
			p[2] = pad;
			offset = 3;
		} else {
			p = nb->rp;
			p[0] = (m>>8) | 0x80;
			p[1] = m;
			offset = 2;
		}

		switch(s.s->state){
		case Sencrypting:
			nb = encryptb(s.s, nb, offset);
			break;
		case Sdigesting:
			nb = digestb(s.s, nb, offset);
			break;
		case Sdigenc:
			nb = digestb(s.s, nb, offset);
			nb = encryptb(s.s, nb, offset);
			break;
		}

		s.s->out.mid++;

		m = BLEN(nb);
		devtab[s.s->c->type]->bwrite(s.s->c, nb, s.s->c->offset);
		s.s->c->offset += m;
	}
	qunlock(&s.s->out.q);
	poperror();

	return rv;
}

static void
setsecret(OneWay *w, uchar *secret, int n)
{
	free(w->secret);
	w->secret = mallocz(n, 0);
	if(w->secret == nil)
		error(Enomem);
	memmove(w->secret, secret, n);
	w->slen = n;
}

static void
initIDEAkey(OneWay *w)
{

	free(w->state);
	w->state = malloc(sizeof(IDEAstate));
	if(w->state == nil)
		error(Enomem);
	if(w->slen >= 24)
		setupIDEAstate(w->state, w->secret, w->secret+16);
	else if(w->slen >= 16)
		setupIDEAstate(w->state, w->secret, 0);
	else
		error("secret too short");
}

static void
initDESkey(OneWay *w)
{

	free(w->state);
	w->state = malloc(sizeof(DESstate));
	if (!w->state)
		error(Enomem);
	if(w->slen >= 16)
		setupDESstate(w->state, w->secret, w->secret+8);
	else if(w->slen >= 8)
		setupDESstate(w->state, w->secret, 0);
	else
		error("secret too short");
}

/*
 *  40 bit DES is the same as 56 bit DES.  However,
 *  16 bits of the key are masked to zero.
 */
static void
initDESkey_40(OneWay *w)
{
	uchar key[8];


	if(w->slen >= 8) {
		memmove(key, w->secret, 8);
		key[0] &= 0x0f;
		key[2] &= 0x0f;
		key[4] &= 0x0f;
		key[6] &= 0x0f;
	}

	free(w->state);
	w->state = malloc(sizeof(DESstate));
	if (!w->state)
		error(Enomem);
	if(w->slen >= 16)
		setupDESstate(w->state, key, w->secret+8);
	else if(w->slen >= 8)
		setupDESstate(w->state, key, 0);
	else
		error("secret too short");
}

static void
initRC4key(OneWay *w)
{
	free(w->state);
	w->state = malloc(sizeof(RC4state));
	if (!w->state)
		error(Enomem);
	setupRC4state(w->state, w->secret, w->slen);
}

/*
 *  40 bit RC4 is the same as n-bit RC4.  However,
 *  we ignore all but the first 40 bits of the key.
 */
static void
initRC4key_40(OneWay *w)
{
	int slen = w->slen;

	if(slen > 5)
		slen = 5;

	free(w->state);
	w->state = malloc(sizeof(RC4state));
	if (!w->state)
		error(Enomem);
	setupRC4state(w->state, w->secret, slen);
}

/*
 *  128 bit RC4 is the same as n-bit RC4.  However,
 *  we ignore all but the first 128 bits of the key.
 */
static void
initRC4key_128(OneWay *w)
{
	int slen = w->slen;

	if(slen > 16)
		slen = 16;

	free(w->state);
	w->state = malloc(sizeof(RC4state));
	if (!w->state)
		error(Enomem);
	setupRC4state(w->state, w->secret, slen);
}

typedef struct Hashalg Hashalg;
struct Hashalg
{
	char	*name;
	int	diglen;
	DigestState *(*hf)(uchar*, u32, uchar*, DigestState*);
};

Hashalg hashtab[] =
{
	{ "md4", MD4dlen, md4, },
	{ "md5", MD5dlen, md5, },
	{ "sha1", SHA1dlen, sha1, },
	{ "sha", SHA1dlen, sha1, },
	{ 0 }
};

static int
parsehashalg(char *p, Dstate *s)
{
	Hashalg *ha;

	for(ha = hashtab; ha->name; ha++){
		if(strcmp(p, ha->name) == 0){
			s->hf = ha->hf;
			s->diglen = ha->diglen;
			s->state &= ~Sclear;
			s->state |= Sdigesting;
			return 0;
		}
	}
	return -1;
}

typedef struct Encalg Encalg;
struct Encalg
{
	char	*name;
	int	blocklen;
	int	alg;
	void	(*keyinit)(OneWay*);
};

Encalg encrypttab[] =
{
	{ "descbc", 8, DESCBC, initDESkey, },           /* DEPRECATED -- use des_56_cbc */
	{ "desecb", 8, DESECB, initDESkey, },           /* DEPRECATED -- use des_56_ecb */
	{ "des_56_cbc", 8, DESCBC, initDESkey, },
	{ "des_56_ecb", 8, DESECB, initDESkey, },
	{ "des_40_cbc", 8, DESCBC, initDESkey_40, },
	{ "des_40_ecb", 8, DESECB, initDESkey_40, },
	{ "rc4", 1, RC4, initRC4key_40, },              /* DEPRECATED -- use rc4_X      */
	{ "rc4_256", 1, RC4, initRC4key, },
	{ "rc4_128", 1, RC4, initRC4key_128, },
	{ "rc4_40", 1, RC4, initRC4key_40, },
	{ "ideacbc", 8, IDEACBC, initIDEAkey, },
	{ "ideaecb", 8, IDEAECB, initIDEAkey, },
	{ 0 }
};

static int
parseencryptalg(char *p, Dstate *s)
{
	Encalg *ea;

	for(ea = encrypttab; ea->name; ea++){
		if(strcmp(p, ea->name) == 0){
			s->encryptalg = ea->alg;
			s->blocklen = ea->blocklen;
			(*ea->keyinit)(&s->in);
			(*ea->keyinit)(&s->out);
			s->state &= ~Sclear;
			s->state |= Sencrypting;
			return 0;
		}
	}
	return -1;
}

static void
alglistinit(void)
{
	Hashalg *h;
	Encalg *e;
	int n;

	n = 1;
	for(e = encrypttab; e->name != nil; e++)
		n += strlen(e->name) + 1;
	encalgs = malloc(n);
	if(encalgs == nil)
		panic("sslinit");
	n = 0;
	for(e = encrypttab; e->name != nil; e++){
		strcpy(encalgs+n, e->name);
		n += strlen(e->name);
		if(e[1].name == nil)
			break;
		encalgs[n++] = ' ';
	}
	encalgs[n] = 0;

	n = 1;
	for(h = hashtab; h->name != nil; h++)
		n += strlen(h->name) + 1;
	hashalgs = malloc(n);
	if(hashalgs == nil)
		panic("sslinit");
	n = 0;
	for(h = hashtab; h->name != nil; h++){
		strcpy(hashalgs+n, h->name);
		n += strlen(h->name);
		if(h[1].name == nil)
			break;
		hashalgs[n++] = ' ';
	}
	hashalgs[n] = 0;
}

static long
sslwrite(Chan *c, void *a, long n, vlong offset)
{
	volatile struct { Dstate *s; } s;
	volatile struct { Block *b; } b;
	int m, t;
	char *p, *np, *e, buf[32];
	uchar *x;

	s.s = dstate[CONV(c->qid)];
	if(s.s == 0)
		panic("sslwrite");

	t = TYPE(c->qid);
	if(t == Qdata){
		if(s.s->state == Sincomplete)
			error(Ebadusefd);
	
		p = a;
		e = p + n;
		do {
			m = e - p;
			if(m > s.s->max)
				m = s.s->max;
	
			b.b = allocb(m);
			memmove(b.b->wp, p, m);
			b.b->wp += m;

			sslbwrite(c, b.b, offset);

			p += m;
		} while(p < e);
		return n;
	}

	/* mutex with operations using what we're about to change */
	if(waserror()){
		qunlock(&s.s->in.ctlq);
		qunlock(&s.s->out.q);
		nexterror();
	}
	qlock(&s.s->in.ctlq);
	qlock(&s.s->out.q);

	switch(t){
	default:
		panic("sslwrite");
	case Qsecretin:
		setsecret(&s.s->in, a, n);
		goto out;
	case Qsecretout:
		setsecret(&s.s->out, a, n);
		goto out;
	case Qctl:
		break;
	}

	if(n >= sizeof(buf))
		error(Ebadarg);
	strncpy(buf, a, n);
	buf[n] = 0;
	p = strchr(buf, '\n');
	if(p)
		*p = 0;
	p = strchr(buf, ' ');
	if(p)
		*p++ = 0;

	if(strcmp(buf, "fd") == 0){
		s.s->c = buftochan(p);

		/* default is clear (msg delimiters only) */
		s.s->state = Sclear;
		s.s->blocklen = 1;
		s.s->diglen = 0;
		s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
		s.s->in.mid = 0;
		s.s->out.mid = 0;
	} else if(strcmp(buf, "alg") == 0 && p != 0){
		s.s->blocklen = 1;
		s.s->diglen = 0;

		if(s.s->c == 0)
			error("must set fd before algorithm");

		if(strcmp(p, "clear") == 0){
			s.s->state = Sclear;
			s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
			goto out;
		}

		if(s.s->in.secret && s.s->out.secret == 0)
			setsecret(&s.s->out, s.s->in.secret, s.s->in.slen);
		if(s.s->out.secret && s.s->in.secret == 0)
			setsecret(&s.s->in, s.s->out.secret, s.s->out.slen);
		if(s.s->in.secret == 0 || s.s->out.secret == 0)
			error("algorithm but no secret");

		s.s->hf = 0;
		s.s->encryptalg = Noencryption;
		s.s->blocklen = 1;

		for(;;){
			np = strchr(p, ' ');
			if(np)
				*np++ = 0;
			else{
				np = strchr(p, '/');
				if(np)
					*np++ = 0;
			}
			if(parsehashalg(p, s.s) < 0)
			if(parseencryptalg(p, s.s) < 0)
				error(Ebadarg);

			if(np == 0)
				break;
			p = np;
		}

		if(s.s->hf == 0 && s.s->encryptalg == Noencryption)
			error(Ebadarg);

		if(s.s->blocklen != 1){
			/* make multiple of blocklen */
			s.s->max = (1<<15) - s.s->diglen - 1;
			s.s->max -= s.s->max % s.s->blocklen;
			s.s->maxpad = (1<<14) - s.s->diglen - 1;
			s.s->maxpad -= s.s->maxpad % s.s->blocklen;
		} else
			s.s->maxpad = s.s->max = (1<<15) - s.s->diglen - 1;
	} else if(strcmp(buf, "secretin") == 0 && p != 0) {
		m = (strlen(p)*3)/2;
		x = smalloc(m);
		if(waserror()){
			free(x);
			nexterror();
		}
		t = dec64(x, m, p, strlen(p));
		setsecret(&s.s->in, x, t);
		poperror();
		free(x);
	} else if(strcmp(buf, "secretout") == 0 && p != 0) {
		m = (strlen(p)*3)/2;
		x = smalloc(m);
		if(waserror()){
			free(x);
			nexterror();
		}
		t = dec64(x, m, p, strlen(p));
		setsecret(&s.s->out, x, t);
		poperror();
		free(x);
	} else
		error(Ebadarg);

out:
	qunlock(&s.s->in.ctlq);
	qunlock(&s.s->out.q);
	poperror();
	return n;
}


static Block*
encryptb(Dstate *s, Block *b, int offset)
{
	uchar *p, *ep, *p2, *ip, *eip;
	DESstate *ds;
	IDEAstate *is;

	switch(s->encryptalg){
	case DESECB:
		ds = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8)
			block_cipher(ds->expanded, p, 0);
		break;
	case DESCBC:
		ds = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8){
			p2 = p;
			ip = ds->ivec;
			for(eip = ip+8; ip < eip; )
				*p2++ ^= *ip++;
			block_cipher(ds->expanded, p, 0);
			memmove(ds->ivec, p, 8);
		}
		break;
	case IDEAECB:
		is = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8)
			idea_cipher(is->edkey, p, 0);
		break;
	case IDEACBC:
		is = s->out.state;
		ep = b->rp + BLEN(b);
		for(p = b->rp + offset; p < ep; p += 8){
			p2 = p;
			ip = is->ivec;
			for(eip = ip+8; ip < eip; )
				*p2++ ^= *ip++;
			idea_cipher(is->edkey, p, 0);
			memmove(is->ivec, p, 8);
		}
		break;
	case RC4:
		rc4(s->out.state, b->rp + offset, BLEN(b) - offset);
		break;
	}
	return b;
}

static Block*
decryptb(Dstate *s, Block *inb)
{
	Block *b, **l;
	uchar *p, *ep, *tp, *ip, *eip;
	DESstate *ds;
	IDEAstate *is;
	uchar tmp[8];
	int i;

	l = &inb;
	for(b = inb; b; b = b->next){
		/* make sure we have a multiple of s->blocklen */
		if(s->blocklen > 1){
			i = BLEN(b);
			if(i % s->blocklen){
				*l = b = pullupblock(b, i + s->blocklen - (i%s->blocklen));
				if(b == 0)
					error("ssl encrypted message too short");
			}
		}
		l = &b->next;

		/* decrypt */
		switch(s->encryptalg){
		case DESECB:
			ds = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep; p += 8)
				block_cipher(ds->expanded, p, 1);
			break;
		case DESCBC:
			ds = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep;){
				memmove(tmp, p, 8);
				block_cipher(ds->expanded, p, 1);
				tp = tmp;
				ip = ds->ivec;
				for(eip = ip+8; ip < eip; ){
					*p++ ^= *ip;
					*ip++ = *tp++;
				}
			}
			break;
		case IDEAECB:
			is = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep; p += 8)
				idea_cipher(is->edkey, p, 1);
			break;
		case IDEACBC:
			is = s->in.state;
			ep = b->rp + BLEN(b);
			for(p = b->rp; p < ep;){
				memmove(tmp, p, 8);
				idea_cipher(is->edkey, p, 1);
				tp = tmp;
				ip = is->ivec;
				for(eip = ip+8; ip < eip; ){
					*p++ ^= *ip;
					*ip++ = *tp++;
				}
			}
			break;
		case RC4:
			rc4(s->in.state, b->rp, BLEN(b));
			break;
		}
	}
	return inb;
}

static Block*
digestb(Dstate *s, Block *b, int offset)
{
	uchar *p;
	DigestState ss;
	uchar msgid[4];
	ulong n, h;
	OneWay *w;

	w = &s->out;

	memset(&ss, 0, sizeof(ss));
	h = s->diglen + offset;
	n = BLEN(b) - h;

	/* hash secret + message */
	(*s->hf)(w->secret, w->slen, 0, &ss);
	(*s->hf)(b->rp + h, n, 0, &ss);

	/* hash message id */
	p = msgid;
	n = w->mid;
	*p++ = n>>24;
	*p++ = n>>16;
	*p++ = n>>8;
	*p = n;
	(*s->hf)(msgid, 4, b->rp + offset, &ss);

	return b;
}

static void
checkdigestb(Dstate *s, Block *inb)
{
	uchar *p;
	DigestState ss;
	uchar msgid[4];
	int n, h;
	OneWay *w;
	uchar digest[128];
	Block *b;

	w = &s->in;

	memset(&ss, 0, sizeof(ss));

	/* hash secret */
	(*s->hf)(w->secret, w->slen, 0, &ss);

	/* hash message */
	h = s->diglen;
	for(b = inb; b; b = b->next){
		n = BLEN(b) - h;
		if(n < 0)
			panic("checkdigestb");
		(*s->hf)(b->rp + h, n, 0, &ss);
		h = 0;
	}

	/* hash message id */
	p = msgid;
	n = w->mid;
	*p++ = n>>24;
	*p++ = n>>16;
	*p++ = n>>8;
	*p = n;
	(*s->hf)(msgid, 4, digest, &ss);

	/* requires pullupblock */
	if(memcmp(digest, inb->rp, s->diglen) != 0)
		error("bad digest");
}

/* get channel associated with an fd */
static Chan*
buftochan(char *p)
{
	Chan *c;
	int fd;

	if(p == 0)
		error(Ebadarg);
	fd = strtoul(p, 0, 0);
	if(fd < 0)
		error(Ebadarg);
	c = fdtochan(up->env->fgrp, fd, -1, 0, 1);	/* error check and inc ref */
	return c;
}

/* hang up a digest connection */
static void
sslhangup(Dstate *s)
{
	qlock(&s->in.q);
	freeblist(s->processed);
	s->processed = 0;
	freeblist(s->unprocessed);
	s->unprocessed = 0;
	s->state = Sincomplete;
	qunlock(&s->in.q);
}

static void
dsclone(Chan *ch)
{
	Dstate **pp, **ep, **np;
	int newmax;

	lock(&dslock);
	if(waserror()) {
		unlock(&dslock);
		nexterror();
	}
	ep = &dstate[maxdstate];
	for(pp = dstate; pp < ep; pp++) {
		if(*pp == 0) {
			dsnew(ch, pp);
			break;
		}
	}
	if(pp >= ep) {
		if(maxdstate >= Maxdstate)
			error(Enodev);
		newmax = 2 * maxdstate;
		if(newmax > Maxdstate)
			newmax = Maxdstate;

		np = realloc(dstate, sizeof(Dstate*) * newmax);
		if(np == 0)
			error(Enomem);
		dstate = np;
		pp = &dstate[maxdstate];
		memset(pp, 0, sizeof(Dstate*)*(newmax - maxdstate));

		maxdstate = newmax;
		dsnew(ch, pp);
	}
	poperror();
	unlock(&dslock);
}

static void
dsnew(Chan *ch, Dstate **pp)
{
	Dstate *s;
	int t;

	*pp = s = mallocz(sizeof(*s), 1);
	if(s == nil)
		error(Enomem);
	if(pp - dstate >= dshiwat)
		dshiwat++;
	s->state = Sincomplete;
	s->ref = 1;
	kstrdup(&s->user, up->env->user);
	s->perm = 0660;
	t = TYPE(ch->qid);
	if(t == Qclonus)
		t = Qctl;
	ch->qid.path = QID(pp - dstate, t);
	ch->qid.vers = 0;
	ch->qid.type = QTFILE;
}

Dev ssldevtab = {
	'D',
	"ssl",

	sslinit,
	sslattach,
	sslwalk,
	sslstat,
	sslopen,
	devcreate,
	sslclose,
	sslread,
	sslbread,
	sslwrite,
	sslbwrite,
	devremove,
	sslwstat
};
