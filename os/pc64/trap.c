#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"../port/error.h"
#include	"tos.h"

Vctl *vctl[256];	/* defined in pc/irq.c */

extern int irqhandled(Ureg*, int);
extern void irqinit(void);

int (*breakhandler)(Ureg *ur, Proc*);

static void debugbpt(Ureg*, void*);
static void faultamd64(Ureg*, void*);
static void doublefault(Ureg*, void*);
static void unexpected(Ureg*, void*);
static void _dumpstack(Ureg*);
void dumpureg(Ureg* ureg);

void
trapinit0(void)
{
	u32 d1, v;
	uintptr vaddr;
	Segdesc *idt;
	uintptr ptr[2];

	idt = (Segdesc*)IDTADDR;
	vaddr = (uintptr)vectortable;
	for(v = 0; v < 256; v++){
		d1 = (vaddr & 0xFFFF0000)|SEGP;
		switch(v){
		case VectorBPT:
			d1 |= SEGPL(0)|SEGIG;
			break;

		case VectorSYSCALL:
			d1 |= SEGPL(0)|SEGIG;
			break;

		default:
			d1 |= SEGPL(0)|SEGIG;
			break;
		}

		idt->d0 = (vaddr & 0xFFFF)|(KESEL<<16);
		idt->d1 = d1;
		idt++;

		idt->d0 = (vaddr >> 32);
		idt->d1 = 0;
		idt++;

		vaddr += 6;
	}
	((ushort*)&ptr[1])[-1] = sizeof(Segdesc)*512-1;
	ptr[1] = (uintptr)IDTADDR;
	lidt(&((ushort*)&ptr[1])[-1]);
}

void
trapinit(void)
{
	irqinit();

	nmienable();

	/*
	 * Special traps.
	 * Syscall() is called directly without going through trap().
	 */
	/* 9front specific trapenable(VectorDE, debugexc, 0, "debugexc"); */
	trapenable(VectorBPT, debugbpt, 0, "debugpt");
/*	trapenable(VectorPF, faultamd64, 0, "faultamd64");
	trapenable(Vector2F, doublefault, 0, "doublefault");
	trapenable(Vector15, unexpected, 0, "unexpected");
*/
}

static char* excname[32] = {
	"divide error",
	"debug exception",
	"nonmaskable interrupt",
	"breakpoint",
	"overflow",
	"bounds check",
	"invalid opcode",
	"coprocessor not available",
	"double fault",
	"coprocessor segment overrun",
	"invalid TSS",
	"segment not present",
	"stack exception",
	"general protection violation",
	"page fault",
	"15 (reserved)",
	"coprocessor error",
	"alignment check",
	"machine check",
	"simd error",
	"20 (reserved)",
	"21 (reserved)",
	"22 (reserved)",
	"23 (reserved)",
	"24 (reserved)",
	"25 (reserved)",
	"26 (reserved)",
	"27 (reserved)",
	"28 (reserved)",
	"29 (reserved)",
	"30 (reserved)",
	"31 (reserved)",
};

void
dumprstack(intptr h, intptr rsp, intptr he)
{
	intptr i;
	/* int l=0; */

	print("Forth return stack h 0x%zx R8 RSP 0x%zx RSTACK 0x%zx he 0x%zx\n",
			h, rsp, h+RSTACK, he);
	if(he == 0 || h == 0 || rsp < h || rsp >= he || h+RSTACK < h || h+RSTACK >= he)
		return;
	for(i = h + RSTACK-8; i >= rsp; i-=8){
		if(*(intptr*)i >=h && *(intptr*)i <=he)
			print("	0x%zX: 0x%zX 0x%zX %zd\n", i, *(intptr*)i, *(intptr*)i-h, *(intptr*)i-h);
		else
			print("	0x%zX: 0x%zX\n", i, *(intptr*)i);
	/*	l++;
		if(l == 3){
			l = 0;
			print("\n");
		} */
	}
	print("\n");
}

void
dumppstack(intptr h, intptr psp, intptr he)
{
	intptr i;
	int l=0;

	print("Forth parameter stack h 0x%zx DX PSP 0x%zx PSTACK 0x%zx he 0x%zx\n",
			h, psp, h+PSTACK, he);
	if(he == 0 || h == 0 || psp < h || psp >= he || h+PSTACK < h || h+PSTACK >= he)
		return;
	print("	depth %zd\n", (h+PSTACK - psp)/sizeof(intptr));
	for(i = h + PSTACK-8; i >= psp; i-=8){
		print("	0x%zX: 0x%zX %d", i, *(intptr*)i, *(intptr*)i);
		l++;
		if(l == 3){
			l = 0;
			print("\n");
		}
	}
	print("\n");
}

/*
 *  All traps come here.  It is slower to have all traps call trap()
 *  rather than directly vectoring the handler.  However, this avoids a
 *  lot of code duplication and possible bugs.  The only exception is
 *  VectorSYSCALL.
 *  Trap is called with interrupts disabled via interrupt-gates.
 */
static int
usertrap(int vno)
{
	char buf[ERRMAX];

	if(vno < nelem(excname)){
		spllo();
		sprint(buf, "sys: trap: %s", excname[vno]);
		postnote(up, 1, buf, NDebug);
		return 1;
	}
	return 0;
}

/*
 *  dump registers
 */
void
dumpforthregs(Ureg* ureg)
{
	if(up)
		iprint("cpu%d: registers for %s %ud\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	iprint("  AX %.16lluX  BX TOP %.16lluX  CX %.16lluX\n",
		ureg->ax, ureg->bx, ureg->cx);
	iprint("  DX PSP %.16lluX  SI %.16lluX  DI %.16lluX\n",
		ureg->dx, ureg->si, ureg->di);
	iprint("  BP %.16lluX  R8 RSP %.16lluX\n",
		ureg->bp, ureg->r8);
	iprint("  R9 IP %.16lluX %.16lluX %.16llud\n",
		ureg->r9, ureg->r9-ureg->r11, ureg->r9-ureg->r11);
	iprint(" R10 W %.16lluX %.16lluX %.16llud\n",
		ureg->r10, ureg->r10-ureg->r11, ureg->r10-ureg->r11);
	iprint(" R11 UP %.16lluX R12 UPE %.16lluX\n",
		ureg->r11, ureg->r12);
	iprint(" R13 %.16lluX R14 up %.16lluX R15 m %.16lluX\n",
		ureg->r13, ureg->r14, ureg->r15);
	iprint("  CS %.4lluX   SS %.4lluX    PC %.16lluX  SP %.16lluX\n",
		ureg->cs & 0xffff, ureg->ss & 0xffff, ureg->pc, ureg->sp);
	iprint("TYPE %.2lluX  ERROR %.4lluX FLAGS %.8lluX\n",
		ureg->type & 0xff, ureg->error & 0xffff, ureg->flags & 0xffffffff);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	iprint(" CR0 %8.8llux CR2 %16.16llux CR3 %16.16llux",
		getcr0(), getcr2(), getcr3());
	if(m->cpuiddx & (Mce|Tsc|Pse|Vmex)){
		iprint(" CR4 %16.16llux\n", getcr4());
		if(ureg->type == 18)
			dumpmcregs();
	}
	iprint("  ur %#p up %#p\n", ureg, up);

	/* my stuff, not in 9front */
	print("\n  ur %lux up %lux ureg->bp & ~0xFFF %zx\n", ureg, up, ureg->bp & ~0xFFF);
	if((ureg->bp & ~0xFFF) == FFSTART){
		for(intptr *i = (intptr*)FFSTART; i<=(intptr*)ureg->bp; i++){
			print("0x%p: 0x%zx\n", i, *i);
		}
		for(intptr *i = (intptr*)FFEND; i>=(intptr*)ureg->sp; i--){
			print("0x%p: 0x%zx\n", i, *i);
		}
	}
}

/* go to user space */
void
trap(Ureg *ureg)
{
	int vno, user;

	user = kenter(ureg);
	vno = ureg->type;
	if(irqhandled(ureg, vno) == 0 /* TODO && (!user || !usertrap(vno)) */){
		dumpregs(ureg);
		if(up->fmem != nil){
			dumpforthregs(ureg);
			dumprstack(ureg->r11, ureg->r8, ureg->r12);
			dumppstack(ureg->r11, ureg->dx, ureg->r12);
		}
/*		if(user == 0){ */
			ureg->sp = (uintptr)&ureg->sp;
			_dumpstack(ureg);
/*		}*/
		if(vno < nelem(excname))
			panic("%s", excname[vno]);
		panic("unknown trap/intr: %d", vno);
	}
	splhi();

	if(user){
		if(up->procctl || up->nnote)
			notify(ureg);
		kexit(ureg);
	}
}

/*
 *  dump registers
 */
void
dumpregs(Ureg* ureg)
{
	if(up)
		iprint("cpu%d: registers for %s %ud\n",
			m->machno, up->text, up->pid);
	else
		iprint("cpu%d: registers for kernel\n", m->machno);

	iprint("  AX %.16lluX  BX TOP %.16lluX  CX %.16lluX\n",
		ureg->ax, ureg->bx, ureg->cx);
	iprint("  DX PSP %.16lluX  SI %.16lluX  DI %.16lluX\n",
		ureg->dx, ureg->si, ureg->di);
	iprint("  BP %.16lluX  R8 RSP %.16lluX  R9 IP %.16lluX\n",
		ureg->bp, ureg->r8, ureg->r9);
	iprint(" R10 W %.16lluX R11 UP %.16lluX R12 UPE %.16lluX\n",
		ureg->r10, ureg->r11, ureg->r12);
	iprint(" R13 %.16lluX R14 up %.16lluX R15 m %.16lluX\n",
		ureg->r13, ureg->r14, ureg->r15);
	iprint("  CS %.4lluX   SS %.4lluX    PC %.16lluX  SP %.16lluX\n",
		ureg->cs & 0xffff, ureg->ss & 0xffff, ureg->pc, ureg->sp);
	iprint("TYPE %.2lluX  ERROR %.4lluX FLAGS %.8lluX\n",
		ureg->type & 0xff, ureg->error & 0xffff, ureg->flags & 0xffffffff);

	/*
	 * Processor control registers.
	 * If machine check exception, time stamp counter, page size extensions
	 * or enhanced virtual 8086 mode extensions are supported, there is a
	 * CR4. If there is a CR4 and machine check extensions, read the machine
	 * check address and machine check type registers if RDMSR supported.
	 */
	iprint(" CR0 %8.8llux CR2 %16.16llux CR3 %16.16llux",
		getcr0(), getcr2(), getcr3());
	if(m->cpuiddx & (Mce|Tsc|Pse|Vmex)){
		iprint(" CR4 %16.16llux\n", getcr4());
		if(ureg->type == 18)
			dumpmcregs();
	}
	iprint("  ur %#p up %#p\n", ureg, up);

	/* my stuff, not in 9front */
	print("\n  ur %lux up %lux ureg->bp & ~0xFFF %zx\n", ureg, up, ureg->bp & ~0xFFF);
	if((ureg->bp & ~0xFFF) == FFSTART){
		for(intptr *i = (intptr*)FFSTART; i<=(intptr*)ureg->bp; i++){
			print("0x%p: 0x%zx\n", i, *i);
		}
		for(intptr *i = (intptr*)FFEND; i>=(intptr*)ureg->sp; i--){
			print("0x%p: 0x%zx\n", i, *i);
		}
	}
}

/* displays in the order pushed into the stack */
void
dumpureg(Ureg* ureg)
{
	if(up)
		print("cpu%d: registers for %s %ud\n",
			m->machno, up->text, up->pid);
	else
		print("cpu%d: registers for kernel\n", m->machno);

	print("SS %4.4zuX SP %zux\n", ureg->ss & 0xFFFF, ureg->usp);
	print("	FLAGS %zux CS %zux PC %zux ECODE %zux TRAP %zux\n",
		ureg->flags, ureg->cs, ureg->pc, ureg->ecode, ureg->trap);
	print("	GS %4.4ux  FS %4.4ux  ES %4.4ux  DS %4.4ux\n",
		ureg->gs & 0xFFFF, ureg->fs & 0xFFFF, ureg->es & 0xFFFF,
		ureg->ds & 0xFFFF);

	print("	R15 m %8.8zux  R14 up %8.8zux R13 %8.8zux\n",
		ureg->r15, ureg->r14, ureg->r13);
	print("	R12 UPE %8.8zux	R11 UP %8.8zzux	R10 W %8.8zux\n"
			"	R9 IP %8.8zux	R8 RSP %8.8zux\n",
		ureg->r12, ureg->r11, ureg->r10,
		ureg->r9, ureg->r8);
	print("	BP RARG %8.8zux	DI %8.8zzux	SI %8.8zux\n"
			"	DX PSP %8.8zux	CX %8.8zux	BX TOP %8.8zux\n"
			"	AX %8.8zux\n",
		ureg->bp, ureg->di, ureg->si,
		ureg->dx, ureg->cx, ureg->bx,
		ureg->ax);
}

/*
 * Fill in enough of Ureg to get a stack trace, and call a function.
 * Used by debugging interface rdb.
 */
void
callwithureg(void (*fn)(Ureg*))
{
	Ureg ureg;
	ureg.pc = getcallerpc(&fn);
	ureg.sp = (uintptr)&fn;
	fn(&ureg);
}

static void
_dumpstack(Ureg *ureg)
{
	uintptr l, v, i, estack;
	extern ulong etext;
	int x;
	char *s;

	if((s = getconf("*nodumpstack")) != nil && strcmp(s, "0") != 0){
		iprint("dumpstack disabled\n");
		return;
	}
	iprint("dumpstack\n");

	x = 0;
	x += iprint("ktrace /kernel/path %#p %#p <<EOF\n", ureg->pc, ureg->sp);
	i = 0;
	if(up
	&& (uintptr)&l >= (uintptr)up->kstack
	&& (uintptr)&l <= (uintptr)up->kstack+KSTACK)
		estack = (uintptr)up->kstack+KSTACK;
	else if((uintptr)&l >= (uintptr)m->stack
	&& (uintptr)&l <= (uintptr)m+MACHSIZE)
		estack = (uintptr)m+MACHSIZE;
	else
		return;
	x += iprint("estackx %p\n", estack);

	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		v = *(uintptr*)l;
		if((KTZERO < v && v < (uintptr)&etext) || estack-l < 32){
			/*
			 * Could Pick off general CALL (((uchar*)v)[-5] == 0xE8)
			 * and CALL indirect through AX
			 * (((uchar*)v)[-2] == 0xFF && ((uchar*)v)[-2] == 0xD0),
			 * but this is too clever and misses faulting address.
			 */
			x += iprint("%.8lux=%.8lux ", (ulong)l, (ulong)v);
			i++;
		}
		if(i == 4){
			i = 0;
			x += iprint("\n");
		}
	}
	if(i)
		iprint("\n");
	iprint("EOF\n");

	if(ureg->type != VectorNMI)
		return;

	i = 0;
	for(l = (uintptr)&l; l < estack; l += sizeof(uintptr)){
		iprint("%.8p ", *(uintptr*)l);
		if(++i == 8){
			i = 0;
			iprint("\n");
		}
	}
	if(i)
		iprint("\n");
}

void
dumpstack(void)
{
	callwithureg(_dumpstack);
}

static void
debugexc(Ureg *ureg, void *)
{
	u64int dr6, m;
	char buf[ERRMAX];
	char *p, *e;
	int i;

	dr6 = getdr6();
	if(up == nil)
		panic("kernel debug exception dr6=%#.8ullx", dr6);
	putdr6(up->dr[6]);
	if(userureg(ureg))
		qlock(&up->debug);
	else if(!canqlock(&up->debug))
		return;
	m = up->dr[7];
	m = (m >> 4 | m >> 3) & 8 | (m >> 3 | m >> 2) & 4 | (m >> 2 | m >> 1) & 2 | (m >> 1 | m) & 1;
	m &= dr6;
	if(m == 0){
		sprint(buf, "sys: debug exception dr6=%#.8ullx", dr6);
		postnote(up, 0, buf, NDebug);
	}else{
		p = buf;
		e = buf + sizeof(buf);
		p = seprint(p, e, "sys: watchpoint ");
		for(i = 0; i < 4; i++)
			if((m & 1<<i) != 0)
				p = seprint(p, e, "%d%s", i, (m >> i + 1 != 0) ? "," : "");
		postnote(up, 0, buf, NDebug);
	}
	qunlock(&up->debug);
}

static void
debugbpt(Ureg* ureg, void*)
{
	char buf[ERRMAX];

	if(up == 0)
		panic("kernel bpt");
	/* restore pc to instruction that caused the trap */
	ureg->pc--;
	sprint(buf, "sys: breakpoint");
	postnote(up, 1, buf, NDebug);
}

static void
doublefault(Ureg*, void*)
{
	panic("double fault");
}

static void
unexpected(Ureg* ureg, void*)
{
	print("unexpected trap %zud; ignoring\n", ureg->trap);
}

static void
faultamd64(Ureg* ureg, void*)
{
	uintptr addr;
	int read, user, n, insyscall, f;
	char buf[ERRMAX];

	addr = getcr2();
	read = !(ureg->error & 2);
	user = userureg(ureg);
	if(!user){
		{
			extern void _peekinst(void);
			
			if((void(*)(void))ureg->pc == _peekinst){
				ureg->pc += 2;
				return;
			}
		}
		if(addr >= (uintptr)USTKTOP)
			panic("kernel fault: bad address pc=%#p addr=%#p", ureg->pc, addr);
		if(up == nil)
			panic("kernel fault: no user process pc=%#p addr=%#p", ureg->pc, addr);
	}
	if(up == nil)
		panic("user fault: up=0 pc=%#p addr=%#p", ureg->pc, addr);

	/* forth specific dump stack */
	dumprstack(ureg->r11, ureg->r8, ureg->r12);
	dumppstack(ureg->r11, ureg->dx, ureg->r12);

	insyscall = up->insyscall;
	up->insyscall = 1;
	f = fpusave();
	if(!user && waserror()){
		if(up->nerrlab == 0){
			pprint("suicide: sys: %s\n", up->errstr);
			pexit(up->errstr, 1);
		}
		int s = splhi();
		fpurestore(f);
		up->insyscall = insyscall;
		splx(s);
		nexterror();
	}
	n = -1 /* fault(addr, ureg->pc, read) no paging for us*/;
	if(n < 0){
		if(!user){
			dumpregs(ureg);
			panic("fault: %#p", addr);
		}
		/* checkpages(); */
		sprint(buf, "sys: trap: fault %s addr=%#p",
			read ? "read" : "write", addr);
		postnote(up, 1, buf, NDebug);
	}
	if(!user) poperror();
	splhi();
	fpurestore(f);
	up->insyscall = insyscall;
}

/*
 *  Call user, if necessary, with note.
 *  Pass user the Ureg struct and the note on his stack.
 */
int
notify(Ureg* ureg)
{
	int l;
	uintptr sp;
	Note *n;

	if(up->procctl)
		procctl();
	if(up->nnote == 0)
		return 0;
	spllo();
	qlock(&up->debug);
	up->notepending = 0;
	n = &up->note[0];
	if(strncmp(n->msg, "sys:", 4) == 0){
		l = strlen(n->msg);
		if(l > ERRMAX-15)	/* " pc=0x12345678\0" */
			l = ERRMAX-15;
		sprint(n->msg+l, " pc=%#p", ureg->pc);
	}

	if(n->flag!=NUser && (up->notified || up->notify==0)){
		qunlock(&up->debug);
		if(n->flag == NDebug){
			up->fpstate &= ~FPillegal;
			pprint("suicide: %s\n", n->msg);
		}
		pexit(n->msg, n->flag!=NDebug);
	}

	if(up->notified){
		qunlock(&up->debug);
		splhi();
		return 0;
	}

	if(!up->notify){
		qunlock(&up->debug);
		pexit(n->msg, n->flag!=NDebug);
	}
	sp = ureg->sp;
	sp -= 256;	/* debugging: preserve context causing problem */
	sp -= sizeof(Ureg);
if(0) print("%s %ud: notify %#p %#p %#p %s\n",
	up->text, up->pid, ureg->pc, ureg->sp, sp, n->msg);

	if(!okaddr((uintptr)up->notify, 1, 0)
	|| !okaddr(sp-ERRMAX-4*BY2WD, sizeof(Ureg)+ERRMAX+4*BY2WD, 1)){
		qunlock(&up->debug);
		up->fpstate &= ~FPillegal;
		pprint("suicide: bad address in notify\n");
		pexit("Suicide", 0);
	}

	memmove((Ureg*)sp, ureg, sizeof(Ureg));
	*(Ureg**)(sp-BY2WD) = up->ureg;	/* word under Ureg is old up->ureg */
	up->ureg = (void*)sp;
	sp -= BY2WD+ERRMAX;
	memmove((char*)sp, up->note[0].msg, ERRMAX);
	sp -= 3*BY2WD;
	((uintptr*)sp)[2] = sp + 3*BY2WD;	/* arg2 string */
	((uintptr*)sp)[1] = (uintptr)up->ureg;	/* arg1 is ureg* */
	((uintptr*)sp)[0] = 0;			/* arg0 is pc */
	ureg->sp = sp;
	ureg->pc = (uintptr)up->notify;
	ureg->bp = (uintptr)up->ureg;		/* arg1 passed in RARG */
	ureg->cs = UESEL;
	ureg->ss = UDSEL;
	up->notified = 1;
	up->nnote--;
	memmove(&up->lastnote, &up->note[0], sizeof(Note));
	memmove(&up->note[0], &up->note[1], up->nnote*sizeof(Note));
	qunlock(&up->debug);
	splhi();
	if(up->fpstate == FPactive){
		fpsave(up->fpsave);
		up->fpstate = FPinactive;
	}
	up->fpstate |= FPillegal;
	return 1;
}

/* TODO include 9front port/fault.c */
int
okaddr(uintptr, u32, int)
{
	return 1;
}
void
validaddr(uintptr addr, ulong len, int write)
{
	if(!okaddr(addr, len, write)){
		pprint("suicide: invalid address %#p/%lud in sys call pc=%#p\n", addr, len, userpc());
		postnote(up, 1, "sys: bad address in syscall", NDebug);
		error(Ebadarg);
	}
}

/*
 *   Return user to state before notify()
 */
void
noted(Ureg* ureg, ulong arg0)
{
	Ureg *nureg;
	uintptr oureg, sp;

	up->fpstate &= ~FPillegal;
	spllo();
	qlock(&up->debug);
	if(arg0!=NRSTR && !up->notified) {
		qunlock(&up->debug);
		pprint("call to noted() when not notified\n");
		pexit("Suicide", 0);
	}
	up->notified = 0;

	nureg = up->ureg;	/* pointer to user returned Ureg struct */

	/* sanity clause */
	oureg = (uintptr)nureg;
/* TODO we check that this register is within the kernel data segment
	if(!okaddr(oureg-BY2WD, BY2WD+sizeof(Ureg), 0)){
		qunlock(&up->debug);
		pprint("bad ureg in noted or call to noted when not notified\n");
		pexit("Suicide", 0);
	}
*/
	/* don't let user change system flags or segment registers */
	setregisters(ureg, (char*)ureg, (char*)nureg, sizeof(Ureg));

	switch(arg0){
	case NCONT:
	case NRSTR:
if(0) print("%s %ud: noted %#p %#p\n",
	up->text, up->pid, nureg->pc, nureg->sp);
		if(!okaddr(nureg->pc, 1, 0) || !okaddr(nureg->sp, BY2WD, 0)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		up->ureg = (Ureg*)(*(uintptr*)(oureg-BY2WD));
		qunlock(&up->debug);
		break;

	case NSAVE:
		if(!okaddr(nureg->pc, 1, 0)
		|| !okaddr(nureg->sp, BY2WD, 0)){
			qunlock(&up->debug);
			pprint("suicide: trap in noted\n");
			pexit("Suicide", 0);
		}
		qunlock(&up->debug);
		sp = oureg-4*BY2WD-ERRMAX;
		splhi();
		ureg->sp = sp;
		ureg->bp = oureg;		/* arg 1 passed in RARG */
		((uintptr*)sp)[1] = oureg;	/* arg 1 0(FP) is ureg* */
		((uintptr*)sp)[0] = 0;		/* arg 0 is pc */
		break;

	default:
		up->lastnote.flag = NDebug;
		/* fall through */

	case NDFLT:
		qunlock(&up->debug);
		if(up->lastnote.flag == NDebug)
			pprint("suicide: %s\n", up->lastnote.msg);
		pexit(up->lastnote.msg, up->lastnote.flag!=NDebug);
	}
}

/* the USTKTOP macro depends on up being populated
	this function will double fault as up is being set to 0 before using USTKTOP
 */
uintptr
execregs(uintptr entry, ulong ssize, ulong nargs)
{
	uintptr *sp, top;
	Ureg *ureg;

	top = (uintptr)(USTKTOP-sizeof(Tos));		/* address of kernel/user shared data */
	sp = (uintptr*)(USTKTOP - ssize);
	*--sp = nargs;
	ureg = up->dbgreg;
	ureg->sp = (uintptr)sp;
	ureg->pc = entry;
	ureg->cs = KESEL;
	ureg->ss = KDSEL;
	ureg->r14 = ureg->r15 = 0;	/* extern user registers */
	return top;		/* address of kernel/user shared data */
}

/*
 *  return the userpc the last exception happened at
 */
uintptr
userpc(void)
{
	Ureg *ureg;

	ureg = (Ureg*)up->dbgreg;
	return ureg->pc;
}

/* This routine must save the values of registers the user is not permitted
 * to write from devproc and noted() and then restore the saved values before returning.
 */
void
setregisters(Ureg* ureg, char* pureg, char* uva, int n)
{
	u64int flags;

	flags = ureg->flags;
	memmove(pureg, uva, n);
	ureg->cs = KESEL;
	ureg->ss = KDSEL;
	ureg->flags = (ureg->flags & 0x00ff) | (flags & 0xff00);
	/* ureg->pc &= UADDRMASK; */
}


static void
linkproc(void)
{
	spllo();
	up->kpfun(up->kparg);
	pexit("kproc dying", 0);
}

void
kprocchild(Proc* p, void (*func)(void*), void* arg)
{
	/*
	 * gotolabel() needs a word on the stack in
	 * which to place the return PC used to jump
	 * to linkproc().
	 */
	p->sched.pc = (uintptr)linkproc;
	p->sched.sp = (uintptr)p->kstack+KSTACK-BY2WD;

	p->kpfun = func;
	p->kparg = arg;
}

uintptr
dbgpc(Proc *p)
{
	Ureg *ureg;

	ureg = p->dbgreg;
	if(ureg == 0)
		return 0;

	return ureg->pc;
}
