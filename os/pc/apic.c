#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "mp.h"

#define DP if(1){}else print

enum {					/* Local APIC registers */
	LapicID		= 0x0020,	/* ID */
	LapicVER	= 0x0030,	/* Version */
	LapicTPR	= 0x0080,	/* Task Priority */
	LapicAPR	= 0x0090,	/* Arbitration Priority */
	LapicPPR	= 0x00A0,	/* Processor Priority */
	LapicEOI	= 0x00B0,	/* EOI */
	LapicLDR	= 0x00D0,	/* Logical Destination */
	LapicDFR	= 0x00E0,	/* Destination Format */
	LapicSVR	= 0x00F0,	/* Spurious Interrupt Vector */
	LapicISR	= 0x0100,	/* Interrupt Status (8 registers) */
	LapicTMR	= 0x0180,	/* Trigger Mode (8 registers) */
	LapicIRR	= 0x0200,	/* Interrupt Request (8 registers) */
	LapicESR	= 0x0280,	/* Error Status */
	LapicICRLO	= 0x0300,	/* Interrupt Command */
	LapicICRHI	= 0x0310,	/* Interrupt Command [63:32] */
	LapicTIMER	= 0x0320,	/* Local Vector Table 0 (TIMER) */
	LapicPCINT	= 0x0340,	/* Performance Counter LVT */
	LapicLINT0	= 0x0350,	/* Local Vector Table 1 (LINT0) */
	LapicLINT1	= 0x0360,	/* Local Vector Table 2 (LINT1) */
	LapicERROR	= 0x0370,	/* Local Vector Table 3 (ERROR) */
	LapicTICR	= 0x0380,	/* Timer Initial Count */
	LapicTCCR	= 0x0390,	/* Timer Current Count */
	LapicTDCR	= 0x03E0,	/* Timer Divide Configuration */
};

enum {					/* LapicSVR */
	LapicENABLE	= 0x00000100,	/* Unit Enable */
	LapicFOCUS	= 0x00000200,	/* Focus Processor Checking Disable */
};

enum {					/* LapicICRLO */
					/* [14] IPI Trigger Mode Level (RW) */
	LapicDEASSERT	= 0x00000000,	/* Deassert level-sensitive interrupt */
	LapicASSERT	= 0x00004000,	/* Assert level-sensitive interrupt */

					/* [17:16] Remote Read Status */
	LapicINVALID	= 0x00000000,	/* Invalid */
	LapicWAIT	= 0x00010000,	/* In-Progress */
	LapicVALID	= 0x00020000,	/* Valid */

					/* [19:18] Destination Shorthand */
	LapicFIELD	= 0x00000000,	/* No shorthand */
	LapicSELF	= 0x00040000,	/* Self is single destination */
	LapicALLINC	= 0x00080000,	/* All including self */
	LapicALLEXC	= 0x000C0000,	/* All Excluding self */
};

enum {					/* LapicESR */
	LapicSENDCS	= 0x00000001,	/* Send CS Error */
	LapicRCVCS	= 0x00000002,	/* Receive CS Error */
	LapicSENDACCEPT	= 0x00000004,	/* Send Accept Error */
	LapicRCVACCEPT	= 0x00000008,	/* Receive Accept Error */
	LapicSENDVECTOR	= 0x00000020,	/* Send Illegal Vector */
	LapicRCVVECTOR	= 0x00000040,	/* Receive Illegal Vector */
	LapicREGISTER	= 0x00000080,	/* Illegal Register Address */
};

enum {					/* LapicTIMER */
					/* [17] Timer Mode (RW) */
	LapicONESHOT	= 0x00000000,	/* One-shot */
	LapicPERIODIC	= 0x00020000,	/* Periodic */

					/* [19:18] Timer Base (RW) */
	LapicCLKIN	= 0x00000000,	/* use CLKIN as input */
	LapicTMBASE	= 0x00040000,	/* use TMBASE */
	LapicDIVIDER	= 0x00080000,	/* use output of the divider */
};

static uchar lapictdxtab[] = {		/* LapicTDCR */
	0x0B,	/* divide by 1 */
	0x00,	/* divide by 2 */
	0x01,	/* divide by 4 */
	0x02,	/* divide by 8 */
	0x03,	/* divide by 16 */
	0x08,	/* divide by 32 */
	0x09,	/* divide by 64 */
	0x0A,	/* divide by 128 */
};

static ulong* lapicbase;

typedef struct Apictimer Apictimer;
struct Apictimer
{
	uvlong	hz;
	ulong	max;
	ulong	min;
	ulong	div;
	int	tdx;
};

static Apictimer lapictimer[MAXMACH];

static ulong
lapicr(int r)
{
	return *(lapicbase+(r/sizeof(*lapicbase)));
}

static void
lapicw(int r, ulong data)
{
	*(lapicbase+(r/sizeof(*lapicbase))) = data;
	data = *(lapicbase+(LapicID/sizeof(*lapicbase)));
	USED(data);
}

static void
showglobalconfig(void)
{
	u64 flags;
	u32 iopl;
	uintptr apic_base;

	print("APIC global configuration\n");
	flags = rflags();
	print("	RFLAGS: 0x%zux, ", flags);
	if(flags & 0x200)
		print("maskable interrupts enabled, ");
	else
		print("maskable interrupts disabled, ");
	iopl = flags & (0x11 << 12);
	print("iopl 0x%x", iopl);
	print("\n");

	rdmsr(0x1B, (s64*)&apic_base);
	print("	APIC_BASE: 0x%zux, ", apic_base);
	if(apic_base & 0x100)
		print("BSP, ");
	else
		print("not BSP, ");
	if(apic_base & 0x400)
		print("enable x2APIC mode, ");
	else
		print("disable x2APIC mode, ");
	if(apic_base & 0x800)
		print("APIC global enable");
	else
		print("APIC global disable");
	print("\n");
}

void
showlapicregisters(void)
{
	showglobalconfig();
	print("	LapicID	 %lux\n", lapicr(LapicID));
	print("	LapicVER %lux\n", lapicr(LapicVER));
	print("	LapicTPR %lux\n", lapicr(LapicTPR));
	print("	LapicAPR %lux\n", lapicr(LapicAPR));
	print("	LapicPPR %lux\n", lapicr(LapicPPR));
	print("	LapicEOI %lux\n", lapicr(LapicEOI));
	print("	LapicLDR %lux\n", lapicr(LapicLDR));
	print("	LapicDFR %lux\n", lapicr(LapicDFR));
	print("	LapicSVR %lux\n", lapicr(LapicSVR));
	print("	LapicISR %lux\n", lapicr(LapicISR));
	print("	LapicTMR %lux\n", lapicr(LapicTMR));
	print("	LapicIRR %lux\n", lapicr(LapicIRR));
	print("	LapicESR %lux\n", lapicr(LapicESR));
	print("	LapicICRLO %lux\n", lapicr(LapicICRLO));
	print("	LapicICRHI %lux\n", lapicr(LapicICRHI));
	print("	LapicTIMER %lux\n", lapicr(LapicTIMER));
	print("	LapicPCINT %lux\n", lapicr(LapicPCINT));
	print("	LapicLINT0 %lux\n", lapicr(LapicLINT0));
	print("	LapicLINT1 %lux\n", lapicr(LapicLINT1));
	print("	LapicERROR %lux\n", lapicr(LapicERROR));
	print("	LapicTICR %lux\n", lapicr(LapicTICR));
	print("	LapicTCCR %lux\n", lapicr(LapicTCCR));
	print("	LapicTDCR %lux\n", lapicr(LapicTDCR));
}

void
lapiconline(void)
{
	Apictimer *a;

	a = &lapictimer[m->machno];

	/*
	 * Reload the timer to de-synchronise the processors,
	 * then lower the task priority to allow interrupts to be
	 * accepted by the APIC.
	 */
	microdelay((TK2MS(1)*1000/conf.nmach) * m->machno);
	lapicw(LapicTICR, a->max);
	lapicw(LapicTIMER, LapicCLKIN|LapicPERIODIC|(VectorPIC+IrqTIMER));

	/*
	 * not strickly neccesary, but reported (osdev.org) to be
	 * required for some machines.
	 */
	lapicw(LapicTDCR, lapictdxtab[a->tdx]);

	lapicw(LapicTPR, 0);
	/* showlapicregisters(); */
}

/*
 *  use the i8253/tsc clock to figure out our lapic timer rate.
 */
static void
lapictimerinit(void)
{
	uvlong x, v, hz;
	Apictimer *a;
	int s;

	if(m->machno != 0){
		lapictimer[m->machno] = lapictimer[0];
		return;
	}

	s = splhi();
	a = &lapictimer[m->machno];
	a->tdx = 0;
Retry:
	lapicw(LapicTIMER, ApicIMASK|LapicCLKIN|LapicONESHOT|(VectorPIC+IrqTIMER));
	lapicw(LapicTDCR, lapictdxtab[a->tdx]);

	x = fastticks(&hz);
	x += hz/10;
	lapicw(LapicTICR, 0xffffffff);
	do{
		v = fastticks(nil);
	}while(v < x);

	v = (0xffffffffUL-lapicr(LapicTCCR))*10;
	if(v > hz-(hz/10)){
		if(v > hz+(hz/10) && a->tdx < nelem(lapictdxtab)-1){
			a->tdx++;
			goto Retry;
		}
		v = hz;
	}

	assert(v >= (100*HZ));

	a->hz = v;
	a->div = hz/a->hz;
	a->max = a->hz/HZ;
	a->min = a->hz/(100*HZ);

	splx(s);

	v = (v+500000LL)/1000000LL;
	DP("cpu%d: lapic clock at %lludMHz\n", m->machno, v);
}

void
lapicinit(Apic* apic)
{
	ulong dfr, ldr, lvt;

	if(lapicbase == 0)
		lapicbase = apic->addr;

	/*
	 * These don't really matter in Physical mode;
	 * set the defaults anyway.
	 */
	if(strncmp(m->cpuidid, "AuthenticAMD", 12) == 0)
		dfr = 0xf0000000;
	else
		dfr = 0xffffffff;
	ldr = 0x00000000;

	DP("lapicinit LapicID ID 0x%lux apic->machno 0x%d\n", lapicr(LapicID), apic->machno);
	lapicw(LapicDFR, dfr);
	lapicw(LapicLDR, ldr);
	lapicw(LapicTPR, 0xff);
	lapicw(LapicSVR, LapicENABLE|(VectorPIC+IrqSPURIOUS));

	lapictimerinit();

	/*
	 * Some Pentium revisions have a bug whereby spurious
	 * interrupts are generated in the through-local mode.
	 */
	switch(m->cpuidax & 0xFFF){
	case 0x526:				/* stepping cB1 */
	case 0x52B:				/* stepping E0 */
	case 0x52C:				/* stepping cC0 */
		wrmsr(0x0E, 1<<14);		/* TR12 */
		break;
	}

	/*
	 * Set the local interrupts. It's likely these should just be
	 * masked off for SMP mode as some Pentium Pros have problems if
	 * LINT[01] are set to ExtINT.
	 * Acknowledge any outstanding interrupts.
	lapicw(LapicLINT0, apic->lintr[0]);
	lapicw(LapicLINT1, apic->lintr[1]);
	 */
	lapiceoi(0);

	lvt = (lapicr(LapicVER)>>16) & 0xFF;
	DP("lapicinit LapicVER Version 0x%lux\n", lvt);
	if(lvt >= 4)
		lapicw(LapicPCINT, ApicIMASK);
	lapicw(LapicERROR, VectorPIC+IrqERROR);
	lapicw(LapicESR, 0);
	lapicr(LapicESR);

	/*
	 * Issue an INIT Level De-Assert to synchronise arbitration ID's.
	 */
	lapicw(LapicICRHI, 0);
	lapicw(LapicICRLO, LapicALLINC|ApicLEVEL|LapicDEASSERT|ApicINIT);
	while(lapicr(LapicICRLO) & ApicDELIVS)
		;

	/*
	 * Do not allow acceptance of interrupts until all initialisation
	 * for this processor is done. For the bootstrap processor this can be
	 * early duing initialisation. For the application processors this should
	 * be after the bootstrap processor has lowered priority and is accepting
	 * interrupts.
	lapicw(LapicTPR, 0);
	 */
}

void
lapicstartap(Apic* apic, int v)
{
	int i;
	ulong crhi;

	/* showlapicregisters(); */
	/* make apic's processor do a warm reset */
	crhi = apic->apicno<<24;
	lapicw(LapicICRHI, crhi);
	lapicw(LapicICRLO, LapicFIELD|ApicLEVEL|LapicASSERT|ApicINIT);
	DP("lapicstartap LapicFIELD|ApicLEVEL|LapicASSERT|ApicINIT 0x%ux\n",
		LapicFIELD|ApicLEVEL|LapicASSERT|ApicINIT);
	microdelay(200);
	lapicw(LapicICRLO, LapicFIELD|ApicLEVEL|LapicDEASSERT|ApicINIT);
	delay(10);

	DP("lapicstartap LapicID ID 0x%lux apic->machno 0x%d\n", lapicr(LapicID), apic->machno);
	/* assumes apic is not an 82489dx */
	for(i = 0; i < 2; i++){
		lapicw(LapicICRHI, crhi);
		/* make apic's processor start at v in real mode */
		lapicw(LapicICRLO, LapicFIELD|ApicEDGE|ApicSTARTUP|(v/BY2PG));
		DP("lapicstartap LapicFIELD|ApicEDGE|ApicSTARTUP|(v/BY2PG) 0x%zux\n",
			LapicFIELD|ApicEDGE|ApicSTARTUP|(v/BY2PG));
		microdelay(200);
	}
	DP("lapicstartap apic->machno %d after the for loop\n", apic->machno);
	/* showlapicregisters(); */
}

void
lapicerror(Ureg*, void*)
{
	ulong esr;

	lapicw(LapicESR, 0);
	esr = lapicr(LapicESR);
	switch(m->cpuidax & 0xFFF){
	case 0x526:				/* stepping cB1 */
	case 0x52B:				/* stepping E0 */
	case 0x52C:				/* stepping cC0 */
		return;
	}
	print("cpu%d: lapicerror: 0x%8.8luX\n", m->machno, esr);
}

void
lapicspurious(Ureg*, void*)
{
	print("cpu%d: lapicspurious\n", m->machno);
}

int
lapicisr(int v)
{
	ulong isr;

	isr = lapicr(LapicISR + (v/32));

	return isr & (1<<(v%32));
}

int
lapiceoi(int v)
{
	lapicw(LapicEOI, 0);

	return v;
}

void
lapicicrw(ulong hi, ulong lo)
{
	lapicw(LapicICRHI, hi);
	lapicw(LapicICRLO, lo);
}

void
ioapicrdtr(Apic* apic, int sel, int* hi, int* lo)
{
	ulong *iowin;

	iowin = apic->addr+(0x10/sizeof(ulong));
	sel = IoapicRDT + 2*sel;

	lock(apic);
	*apic->addr = sel+1;
	if(hi)
		*hi = *iowin;
	*apic->addr = sel;
	if(lo)
		*lo = *iowin;
	unlock(apic);
}

void
ioapicrdtw(Apic* apic, int sel, int hi, int lo)
{
	ulong *iowin;

	iowin = apic->addr+(0x10/sizeof(ulong));
	sel = IoapicRDT + 2*sel;

	lock(apic);
	*apic->addr = sel+1;
	*iowin = hi;
	*apic->addr = sel;
	*iowin = lo;
	unlock(apic);
}

void
ioapicinit(Apic* apic, int apicno)
{
	int hi, lo, v;
	ulong *iowin;

	/*
	 * Initialise the I/O APIC.
	 * The MultiProcessor Specification says it is the responsibility
	 * of the O/S to set the APIC id.
	 * Make sure interrupts are all masked off for now.
	 */
	iowin = apic->addr+(0x10/sizeof(ulong));
	DP("ioapicinit apic->addr 0x%p iowin 0x%p\n", apic->addr, iowin);
	lock(apic);
	*apic->addr = IoapicVER;
	apic->mre = (*iowin>>16) & 0xFF;

	*apic->addr = IoapicID;
	*iowin = apicno<<24;
	unlock(apic);

	DP("ioapicinit *iowin 0x%ux\n", *iowin);
	hi = 0;
	lo = ApicIMASK;
	for(v = 0; v <= apic->mre; v++)
		ioapicrdtw(apic, v, hi, lo);
}

void
lapictimerset(uvlong next)
{
	vlong period;
	Apictimer *a;

	a = &lapictimer[m->machno];
	period = next - fastticks(nil);
	period /= a->div;
	if(period < a->min)
		period = a->min;
	else if(period > a->max - a->min)
		period = a->max;
	lapicw(LapicTICR, period);
}

void
lapicclock(Ureg *u, void*)
{
	/*
	 * since the MTRR updates need to be synchronized across processors,
	 * we want to do this within the clock tick.
	 */
	mtrrclock();
	timerintr(u, 0);
}

void
lapicintron(void)
{
	lapicw(LapicTPR, 0);
}

void
lapicintroff(void)
{
	lapicw(LapicTPR, 0xFF);
}

void
lapicnmienable(void)
{
	lapicw(LapicPCINT, ApicNMI);
}

void
lapicnmidisable(void)
{
	lapicw(LapicPCINT, ApicIMASK);
}
