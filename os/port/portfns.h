Timer*		addclock0link(void (*)(void), int);
Cname*		addelem(Cname*, char*);
void		addprog(Proc*);
void		addrootfile(char*, uchar*, ulong);
Block*		adjustblock(Block*, int);
Block*		allocb(int);
int	anyhigher(void);
int	anyready(void);
void	_assert(char*);
Block*		bl2mem(uchar*, Block*, int);
int		blocklen(Block*);
int	breakhit(Ureg *ur, Proc*);
void		callwithureg(void(*)(Ureg*));
char*		channame(Chan*);
int		canlock(Lock*);
int		canqlock(QLock*);
void		cclose(Chan*);
int		canrlock(RWlock*);
void		chandevinit(void);
void		chandevreset(void);
void		chandevshutdown(void);
Dir*		chandirstat(Chan*);
void		chanfree(Chan*);
void		chanrec(Mnt*);
void		checkalarms(void);
void		checkb(Block*, char*);
void		cinit(void);
Chan*		cclone(Chan*);
void		cclose(Chan*);
void		closeegrp(Egrp*);
void		closefgrp(Fgrp*);
void		closemount(Mount*);
void		closepgrp(Pgrp*);
void		closesigs(Skeyset*);
void		cmderror(Cmdbuf*, char*);
int		cmount(Chan*, Chan*, int, char*);
void		cnameclose(Cname*);
Block*		concatblock(Block*);
void		confinit(void);
void		copen(Chan*);
Block*		copyblock(Block*, int);
int		cread(Chan*, uchar*, int, vlong);
Chan*	cunique(Chan*);
Chan*		createdir(Chan*, Mhead*);
void		cunmount(Chan*, Chan*);
void		cupdate(Chan*, uchar*, int, vlong);
void		cursorenable(void);
void		cursordisable(void);
void		cursoron(void);
void		cursoroff(void);
void		cwrite(Chan*, uchar*, int, vlong);
void		debugkey(Rune, char *, void(*)(), int);
int		decref(Ref*);
Chan*		devattach(int, char*);
Block*		devbread(Chan*, s32, u32);
s32		devbwrite(Chan*, Block*, u32);
Chan*		devclone(Chan*);
void		devcreate(Chan*, char*, u32, u32);
void		devdir(Chan*, Qid, char*, vlong, char*, long, Dir*);
long		devdirread(Chan*, char*, long, Dirtab*, int, Devgen*);
Devgen		devgen;
void		devinit(void);
int		devno(int, int);
void	devpower(int);
Dev*	devbyname(char*);
Chan*		devopen(Chan*, int, Dirtab*, int, Devgen*);
void		devpermcheck(char*, u32, int);
void		devremove(Chan*);
void		devreset(void);
void		devshutdown(void);
int		devstat(Chan*, uchar*, int, Dirtab*, int, Devgen*);
Walkqid*	devwalk(Chan*, Chan*, char**, int, Dirtab*, int, Devgen*);
int		devwstat(Chan*, uchar*, int);
void		disinit(void*);
void		disfault(void*, char*);
int		domount(Chan**, Mhead**);
void		drawactive(int);
void		drawcmap(void);
void		dumpstack(void);
Fgrp*		dupfgrp(Fgrp*);
void		egrpcpy(Egrp*, Egrp*);
int		emptystr(char*);
int		eqchan(Chan*, Chan*, int);
int		eqqid(Qid, Qid);
void		eqlock(QLock*);
void		error(char*);
void		errorf(char*, ...);
#pragma varargck argpos errorf 1
void		errstr(char*, int);
void		excinit(void);
void		exhausted(char*);
void		exit(int);
void		reboot(void);
void		halt(void);
int		export(int, char*, int);
uvlong		fastticks(uvlong*);
uvlong		fastticks2ns(uvlong);
void		fdclose(Fgrp*, int);
Chan*		fdtochan(Fgrp*, int, int, int, int);
int		findmount(Chan**, Mhead**, int, int, Qid);
void		free(void*);
void		freeb(Block*);
void		freeblist(Block*);
void		freeskey(Signerkey*);
void		getcolor(u32, u32*, u32*, u32*);
uintptr	getmalloctag(void*);
uintptr	getrealloctag(void*);
void		gotolabel(Label*);
char*		getconfenv(void);
void 		(*hwrandbuf)(void*, u32);
void		hnputl(void*, ulong);
void		hnputs(void*, ushort);
Block*		iallocb(int);
void		iallocsummary(void);
void		ilock(Lock*);
int		incref(Ref*);
void		iomapinit(u32);
s32		ioreservewin(u32, u32, u32, u32, char*);
int		iprint(char*, ...);
#pragma varargck argpos iprint 1
void		isdir(Chan*);
int		iseve(void);
int		islo(void);
void		iunlock(Lock*);
void		ixsummary(void);
void		kbdclock(void);		/* will go away with kbdfs */
int		kbdcr2nl(Queue*, int);	/* will go away with kbdfs */
void		kbdprocesschar(int);	/* will go away with kbdfs */
int		kbdputc(Queue*, int);	/* will go away with kbdfs */
void		kbdrepeat(int);		/* will go away with kbdfs */
void		kproc(char*, void(*)(void*), void*, int);
int		kfgrpclose(Fgrp*, int);
void		kprocchild(Proc*, void (*)(void*), void*);
int		kprint(char*, ...);
void	(*kproftick)(ulong);
void		ksetenv(char*, char*, int);
void		kstrcpy(char*, char*, int);
void		kstrdup(char**, char*);
long		latin1(Rune*, int);
void		lock(Lock*);
void		logopen(Log*);
void		logclose(Log*);
char*		logctl(Log*, int, char**, Logflag*);
void		logn(Log*, int, void*, int);
long		logread(Log*, void*, ulong, long);
void		logb(Log*, int, char*, ...);
#define	pragma varargck argpos logb 3
Cmdtab*		lookupcmd(Cmdbuf*, Cmdtab*, int);
void		machinit(void);
extern void	machbreakinit(void);
extern Instr	machinstr(ulong addr);
extern void	machbreakset(ulong addr);
extern void	machbreakclear(ulong addr, Instr i);
extern ulong	machnextaddr(Ureg *ur);
void*		malloc(ulong);
void*		mallocalign(uintptr, u32, s32, u32);
void*		mallocz(ulong, int);
Block*		mem2bl(uchar*, int);
void		memmapdump(void);
uvlong		memmapnext(uvlong, ulong);
uvlong		memmapsize(uvlong, uvlong);
void		memmapadd(uvlong, uvlong, ulong);
uvlong		memmapalloc(uvlong, uvlong, uvlong, u32);
void		memmapfree(uvlong, uvlong, ulong);
int			memusehigh(void);
void		microdelay(int);
uvlong		mk64fract(uvlong, uvlong);
void		mkqid(Qid*, vlong, ulong, int);
void		modinit(void);
Chan*		mntauth(Chan*, char*);
long		mntversion(Chan*, char*, int, int);
void		mountfree(Mount*);
void		mousetrack(int, int, int, int);
uvlong		ms2fastticks(ulong);
ulong		msize(void*);
void		mul64fract(uvlong*, uvlong, uvlong);
void		muxclose(Mnt*);
Chan*		namec(char*, int, int, ulong);
Chan*		newchan(void);
Egrp*		newegrp(void);
Fgrp*		newfgrp(Fgrp*);
Mount*		newmount(Mhead*, Chan*, int, char*);
Pgrp*		newpgrp(void);
Proc*		newproc(void);
char*		nextelem(char*, char*);
void		nexterror(void);
Cname*		newcname(char*);
int		notify(Ureg*);
void	notkilled(void);
int		nrand(int);
uvlong		ns2fastticks(uvlong);
int		okaddr(ulong, ulong, int);
int		openmode(ulong);
Block*		packblock(Block*);
Block*		padblock(Block*, int);
void		panic(char*, ...);
Cmdbuf*		parsecmd(char*, int);
void		pexit(char*, int);
void		pgrpcpy(Pgrp*, Pgrp*);
#define		poperror()		up->nerrlab--
int		poolread(char*, int, u64);
void		poolsize(Pool *, u64, int);
int		postnote(Proc *, int, char *, int);
int		pprint(char*, ...);
int		preemption(int);
void		printinit(void);
void		procctl(Proc*);
void		procdump(void);
void		procinit(void);
Proc*		proctab(int);
void	(*proctrace)(Proc*, int, vlong); 
int		progfdprint(Chan*, int, int, char*, int);
int		pullblock(Block**, int);
Block*		pullupblock(Block*, int);
Block*		pullupqueue(Queue*, int);
void		putmhead(Mhead*);
void		putstrn(char*, int);
void		qaddlist(Queue*, Block*);
Block*		qbread(Queue*, int);
long		qbwrite(Queue*, Block*);
Queue*	qbypass(void (*)(void*, Block*), void*);
int		qcanread(Queue*);
void		qclose(Queue*);
int		qconsume(Queue*, void*, int);
Block*		qcopy(Queue*, int, ulong);
int		qdiscard(Queue*, int);
void		qflush(Queue*);
void		qfree(Queue*);
int		qfull(Queue*);
Block*		qget(Queue*);
void		qhangup(Queue*, char*);
int		qisclosed(Queue*);
int		qiwrite(Queue*, void*, int);
int		qlen(Queue*);
void		qlock(QLock*);
void		qnoblock(Queue*, int);
Queue*		qopen(int, int, void (*)(void*), void*);
int		qpass(Queue*, Block*);
int		qpassnolim(Queue*, Block*);
int		qproduce(Queue*, void*, int);
void		qputback(Queue*, Block*);
long		qread(Queue*, void*, int);
Block*		qremove(Queue*);
void		qreopen(Queue*);
void		qsetlimit(Queue*, int);
void		qunlock(QLock*);
int		qwindow(Queue*);
int		qwrite(Queue*, void*, int);
void		randominit(void);
ulong	randomread(void*, ulong);
void*	realloc(void*, ulong);
int		readnum(ulong, char*, ulong, ulong, int);
int		readnum_vlong(ulong, char*, ulong, vlong, int);
int		readstr(ulong, char*, ulong, char*);
void		ready(Proc*);
void		renameproguser(char*, char*);
void		renameuser(char*, char*);
void		resrcwait(char*);
int		return0(void*);
void		rlock(RWlock*);
void		runlock(RWlock*);
Proc*		runproc(void);
void		sched(void);
void		schedinit(void);
long		seconds(void);
void		(*serwrite)(char*, int);
int		setcolor(u32, u32, u32, u32);
int		setlabel(Label*);
void		setmalloctag(void*, uintptr);
int		setpri(int);
void		setrealloctag(void*, uintptr);
char*		skipslash(char*);
void		sleep(Rendez*, int(*)(void*), void*);
void*		smalloc(uintptr);
int		splhi(void);
int		spllo(void);
void		splx(int);
void	splxpc(int);
void		swiproc(Proc*, int);
int		_tas(int*);
void		timeradd(Timer*);
void		timerdel(Timer*);
void		timersinit(void);
void		timerintr(Ureg*, uvlong);
void		timerset(s64);
ulong	tk2ms(ulong);
#define		TK2MS(x) ((x)*(1000/HZ))
uvlong		tod2fastticks(vlong);
vlong		todget(vlong*);
void		todfix(void);
void		todsetfreq(vlong);
void		todinit(void);
void		todset(vlong, vlong, int);
int		tready(void*);
Block*		trimblock(Block*, int, int);
void		tsleep(Rendez*, int (*)(void*), void*, int);
int		uartctl(Uart*, char*);
int		uartgetc(void);
void		uartkick(void*);
void		uartmouse(char*, int (*)(Queue*, int), int);
void		uartsetmouseputc(char*, int (*)(Queue*, int));
void		uartputc(int);
void		uartputs(char*, int);
void		uartrecv(Uart*, char);
int		uartstageoutput(Uart*);
long		unionread(Chan*, void*, long);
void		unlock(Lock*);
void		userinit(void);
ulong		userpc(void);
void		validname(char*, int);
void		validstat(uchar*, int);
void		validwstatname(char*);
int		wakeup(Rendez*);
int		walk(Chan**, char**, int, int, int*);
void		werrstr(char*, ...);
void		wlock(RWlock*);
void		wunlock(RWlock*);
void*		xalloc(uintptr);
void*		xallocz(uintptr, s32);
void		xfree(void*);
void		xhole(uintptr, uintptr);
void		xinit(void);
int		xmerge(void*, void*);
void*		xspanalloc(uintptr, s32, uintptr);
void		xsummary(void);
 
void		validaddr(void*, ulong, int);
void*	vmemchr(void*, int, int);
void		hnputv(void*, vlong);
void		hnputl(void*, ulong);
void		hnputs(void*, ushort);
vlong		nhgetv(void*);
ulong		nhgetl(void*);
ushort		nhgets(void*);
