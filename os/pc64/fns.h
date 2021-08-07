#include "../port/portfns.h"
void	aamloop(int);
Dirtab*	addarchfile(char*, u32, s32(*)(Chan*,void*,s32,s64), s32(*)(Chan*,void*,s32,s64));
void	archinit(void);
void	archreset(void);
int	bios32call(BIOS32ci*, u16int[3]);
int	bios32ci(BIOS32si*, BIOS32ci*);
void	bios32close(BIOS32si*);
BIOS32si* bios32open(char*);
void	bootargs(ulong);
void	bootargsinit(void);
int	checksum(void *, int);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
#define	clearmmucache()				/* x86 doesn't have one */
void	clockintr(Ureg*, void*);
s32	(*cmpswap)(s32*, s32, s32);
s32	cmpswap486(s32*, s32, s32);
void	(*coherence)(void);
void	cpuid(u32, u32, u32 regs[]);
void	fpuinit(void);
int	cpuidentify(void);
void	cpuidprint(void);
void	(*cycles)(uvlong*);
void	delay(int);
int	dmacount(int);
int	dmadone(int);
void	dmaend(int);
int	dmainit(int, int);
s32	dmasetup(int, void*, s32, s32);
void	dumpregs(Ureg*);
int	ecinit(int cmdport, int dataport);
int	ecread(uchar addr);
int	ecwrite(uchar addr, uchar val);
#define	evenaddr(x)				/* x86 doesn't care */
u64	fastticks(u64*);
u64	fastticks2ns(u64);
u64	fastticks2us(u64);
void	fpinit(void);
void	(*fprestore)(FPsave*);
void	(*fpsave)(FPsave*);
void	fpuprocsetup(Proc*);
void	fpuprocfork(Proc*);
void	fpuprocsave(Proc*);
void	fpuprocrestore(Proc*);
int	fpusave(void);
void	fpurestore(int);
u64	getcr0(void);
u64	getcr2(void);
u64	getcr3(void);
u64	getcr4(void);
char*	getconf(char*);
void	guesscpuhz(int);
void	mwait(void*);
int	i8042auxcmd(int);
int	i8042auxcmdval(int);
void	i8042auxenable(void (*)(int, int));
int i8042auxdetect(void);
void	i8042reset(void);
void	i8250console(void);
void	i8253enable(void);
void	i8253init(void);
void	i8253link(void);
uvlong	i8253read(uvlong*);
void	i8253timerset(uvlong);
void	i8259init(void);
int	i8259isr(int);
int	i8259enable(Vctl*);
int	i8259vecno(int);
int	i8259disable(int);
void	idle(void);
void	idlehands(void);
int	inb(int);
void	insb(int, void*, int);
ushort	ins(int);
void	inss(int, void*, int);
ulong	inl(int);
void	insl(int, void*, int);
int	intrdisable(int, void (*)(Ureg *, void *), void*, int, char*);
void	intrenable(int, void (*)(Ureg*, void*), void*, int, char*);
void	invlpg(uintptr);
void	iofree(u32);
void	ioinit(void);
s32	iounused(u32, u32);
s32	ioalloc(s32, u32, u32, char*);
u32	ioreserve(u32, u32, u32, char*);
int	iprint(char*, ...);
int	isaconfig(char*, int, ISAConf*);
int	isvalid_va(void*);
void	kbdenable(void);
void	kbdinit(void);
void	kdbenable(void);
#define	kmapinval()
void	lapicclock(Ureg*, void*);
void	lapictimerset(uvlong);
void	lgdt(void*);
void	lidt(void*);
void	links(void);
void	ltr(ulong);
void	mach0init(void);
void	machinit(void);
void	mathinit(void);
void	mb386(void);
void	mb586(void);
void	meminit(void);
void	meminit0(void);
void	memreserve(uintptr, uintptr);
void	mfence(void);
#define mmuflushtlb() putcr3(getcr3())
void	mmuinit(void);
uintptr	mmukmap(uintptr, uintptr, int);
int	mmukmapsync(uintptr);
uintptr*	mmuwalk(uintptr*, uintptr, int, int);
char*	mtrr(u64, u64, char *);
char*	mtrrattr(u64, u64 *);
void	mtrrclock(void);
int	mtrrprint(char *, s32);
void	mtrrsync(void);
uchar	nvramread(intptr);
void	nvramwrite(intptr, uchar);
void	outb(int, int);
void	outsb(int, void*, int);
void	outs(int, ushort);
void	outss(int, void*, int);
void	outl(int, ulong);
void	outsl(int, void*, int);
void	patwc(void*, int);
void	pcicfginit(void);
int	(*pcicfgrw8)(int, int, int, int);
int	(*pcicfgrw16)(int, int, int, int);
int	(*pcicfgrw32)(int, int, int, int);
int	pciscan(int bno, Pcidev **list, Pcidev *parent);
u32	pcibarsize(Pcidev*, int);
int	pcicfgr8(Pcidev*, int);
int	pcicfgr16(Pcidev*, int);
int	pcicfgr32(Pcidev*, int);
void	pcicfgw8(Pcidev*, int, int);
void	pcicfgw16(Pcidev*, int, int);
void	pcicfgw32(Pcidev*, int, int);
void	pciclrbme(Pcidev*);
void	pciclrioe(Pcidev*);
void	pciclrmwi(Pcidev*);
int	pcigetpms(Pcidev*);
void	pcihinv(Pcidev*);
uchar	pciipin(Pcidev*, uchar);
Pcidev* pcimatch(Pcidev*, int, int);
Pcidev* pcimatchtbdf(int);
void	pcireset(void);
void	pcisetbme(Pcidev*);
void	pcisetioe(Pcidev*);
int	pcisetpms(Pcidev*, int);
void	pcmcisread(PCMslot*);
int	pcmcistuple(int, int, int, void*, int);
PCMmap*	pcmmap(int, ulong, int, int);
int	pcmspecial(char*, ISAConf*);
int	(*_pcmspecial)(char *, ISAConf *);
void	pcmspecialclose(int);
void	(*_pcmspecialclose)(int);
void	pcmunmap(int, PCMmap*);
void	pmap(uintptr, u64, s64);
void	poolsizeinit(void);
void	procsave(Proc*);
void	procsetup(Proc*);
void	punmap(uintptr, vlong);
void	putcr0(u64);
void	putcr2(u64);
void	putcr3(u64);
void	putcr4(u64);
void	putxcr0(u64);
void	putdr(u64*);
void	putdr01236(u64*);
void	putdr6(u64);
void	putdr7(u64);
void*	rampage(void);
s32	rdmsr(s32, s64*);
ulong rdtsc32(void);
void	rdrandbuf(void*, u32);
void*	rsdsearch(void);
void	screeninit(void);
int	screenprint(char*, ...);			/* debugging */
void	(*screenputs)(char*, int);
void	setconfenv(void);
void*	sigsearch(char*, int);
s32	segflush(void*, u32);
void	showframe(void*, void*);
void	syncclock(void);
uvlong	tscticks(uvlong*);
void	trapenable(int, void (*)(Ureg*, void*), void*, char*);
void	trapinit(void);
void	trapinit0(void);
int	tas(void*);
uintptr	umballoc(uintptr, u32, u32);
void	umbfree(uintptr, u32);
uintptr	umbrwmalloc(uintptr, int, int);
void	umbrwfree(uintptr, int);
u64	upaalloc(u64, u32, u32);
u64	upamalloc(u64, u32, u32);
u64	upaallocwin(u64, u32, u32, u32);
void	upafree(uintptr, u32);
void	upareserve(uintptr, u32);
u64	us2fastticks(u64);
void	vectortable(void);
void*	vmap(uintptr, int);
void	vunmap(void*, int);
void	wbinvd(void);
s32	wrmsr(ulong, ulong);
int	xchgw(ushort*, int);
ulong	kzeromap(ulong, ulong, int);
void	nmiscreen(void);
int	kbdinready(void);

#define	userureg(ur)	(((ur)->cs & 3) == 3)
#define	waserror()	(up->nerrlab++, setlabel(&up->errlab[up->nerrlab-1]))
#define getcallerpc(x)	(((uintptr*)(x))[-1])
#define KADDR(a)	((void*)((uintptr)(a)|KZERO))
#define PADDR(a)	((uintptr)(a)&~(uintptr)KZERO)

#define	dcflush(a, b)
#define	clockcheck();
#define 	dumplongs(x, y, z)
