/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		François Revol, revol@free.fr.
 */

#ifndef _TOSCALLS_H
#define _TOSCALLS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLER__
#include <OS.h>

/* TOS calls use 16 bit param alignment, so we must generate the calls ourselves.
 * We then use asm macros, one for each possible arg type and count.
 * cf. how mint does it in sys/mint/arch/asm_misc.h
 */
//#if __GNUC__ >= 3
#define TOS_CLOBBER_LIST "d1", "d2", "a0", "a1", "a2"
//#else
//#error fixme
//#endif

/* void (no) arg */
#define toscallV(trapnr, callnr)				\
({												\
	register int32 retvalue __asm__("d0");		\
												\
	__asm__ volatile							\
	("/* toscall(" #trapnr ", " #callnr ") */\n"	\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	trap	%[trapn]\n"						\
	"	add.l	#2,%%sp\n"						\
	: "=r"(retvalue)	/* output */			\
	: [trapn]"i"(trapnr),[calln]"i"(callnr) 	\
									/* input */	\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);											\
	retvalue;									\
})

#define toscallW(trapnr, callnr, p1)			\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
												\
	__asm__ volatile							\
	("/* toscall(" #trapnr ", " #callnr ") */\n"	\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	trap	%[trapn]\n"						\
	"	add.l	#4,%%sp \n"						\
	: "=r"(retvalue)			/* output */	\
	: "r"(_p1),						/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);											\
	retvalue;									\
})

#define toscallL(trapnr, callnr, p1)			\
({							\
	register int32 retvalue __asm__("d0");		\
	int32 _p1 = (int32)(p1);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.l	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#6,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1),			/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallWW(trapnr, callnr, p1, p2)		\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
	int16 _p2 = (int16)(p2);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.w	%2,-(%%sp) \n			\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#6,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),		/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

#define toscallWLWWWL(trapnr, callnr, p1, p2, p3, p4, p5, p6)	\
({							\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);			\
	int32 _p2 = (int32)(p2);			\
	int16 _p3 = (int16)(p3);			\
	int16 _p4 = (int16)(p4);			\
	int16 _p5 = (int16)(p5);			\
	int32 _p6 = (int32)(p6);			\
							\
	__asm__ volatile				\
	(/*"; toscall(" #trapnr ", " #callnr ")"*/"\n	\
		move.l	%6,-(%%sp) \n			\
		move.w	%5,-(%%sp) \n			\
		move.w	%4,-(%%sp) \n			\
		move.w	%3,-(%%sp) \n			\
		move.l	%2,-(%%sp) \n			\
		move.w	%1,-(%%sp) \n			\
		move.w	%[calln],-(%%sp)\n		\
		trap	%[trapn]\n			\
		add.l	#18,%%sp \n "			\
	: "=r"(retvalue)	/* output */		\
	: "r"(_p1), "r"(_p2),				\
	  "r"(_p3), "r"(_p4),				\
	  "r"(_p5), "r"(_p6),		/* input */	\
	  [trapn]"i"(trapnr),[calln]"i"(callnr)		\
	: TOS_CLOBBER_LIST /* clobbered regs */		\
	);						\
	retvalue;					\
})

/* pointer versions */
#define toscallP(trapnr, callnr, a) toscallL(trapnr, callnr, (int32)a)
#define toscallWPWWWL(trapnr, callnr, p1, p2, p3, p4, p5, p6) \
	toscallWLWWWL(trapnr, callnr, p1, (int32)p2, p3, p4, p5, p6)




#endif /* __ASSEMBLER__ */

#define BIOS_TRAP	13
#define XBIOS_TRAP	14
#define GEMDOS_TRAP	1

/* 
 * Atari BIOS calls
 */

/* those are used by asm code too */

#define DEV_PRINTER	0
#define DEV_AUX	1
#define DEV_CON	2
#define DEV_CONSOLE	2
#define DEV_MIDI	3
#define DEV_IKBD	4
#define DEV_RAW	5

#define K_RSHIFT	0x01
#define K_LSHIFT	0x02
#define K_CTRL	0x04
#define K_ALT	0x08
#define K_CAPSLOCK	0x10
#define K_CLRHOME	0x20
#define K_INSERT	0x40

#define RW_READ			0x00
#define RW_WRITE		0x01
#define RW_NOMEDIACH	0x02
#define RW_NORETRY		0x04
#define RW_NOTRANSLATE	0x08

#ifndef __ASSEMBLER__

//extern int32 bios(uint16 nr, ...);

// cf. http://www.fortunecity.com/skyscraper/apple/308/html/bios.htm

struct tos_bpb {
	int16 recsiz;
	int16 clsiz;
	int16 clsizb;
	int16 rdlen;
	int16 fsiz;
	int16 fatrec;
	int16 datrec;
	int16 numcl;
	int16 bflags;
};

struct tos_pun_info {
	int16 puns;
	uint8 pun[16];
	int32 part_start[16]; // unsigned ??
	uint32 p_cookie;
	struct tos_pun_info *p_cooptr; // points to itself
	uint16 p_version;
	uint16 p_max_sector;
	int32 reserved[16];
};
#define PUN_INFO ((struct tos_pun_info *)0x516L)


//#define Getmpb() toscallV(BIOS_TRAP, 0)
#define Bconstat(dev) toscallW(BIOS_TRAP, 1, (uint16)dev)
#define Bconin(dev) toscallW(BIOS_TRAP, 2, (uint16)dev)
#define Bconout(dev, chr) toscallWW(BIOS_TRAP, 3, (uint16)dev, (uint16)chr)
#define Rwabs(mode, buf, count, recno, dev, lrecno) toscallWPWWWL(BIOS_TRAP, 4, (int16)mode, (void *)buf, (int16)count, (int16)recno, (uint16)dev, (int32)lrecno)
//#define Setexc() toscallV(BIOS_TRAP, 5, )
#define Tickcal() toscallV(BIOS_TRAP, 6)
#define Getbpb(dev) (struct tos_bpb *)toscallW(BIOS_TRAP, 7, (uint16)dev)
#define Bcostat(dev) toscallW(BIOS_TRAP, 8, (uint16)dev)
#define Mediach(dev) toscallW(BIOS_TRAP, 9, (int16)dev)
#define Drvmap() (uint32)toscallV(BIOS_TRAP, 10)
#define Kbshift(mode) toscallW(BIOS_TRAP, 11, (uint16)mode)

/* handy shortcut */
static inline int Bconput(int16 handle, const char *string)
{
	int i, err;
	for (i = 0; string[i]; i++) {
		if (string[i] == '\n')
			Bconout(handle, '\r');
		err = Bconout(handle, string[i]);
		if (err < 0)
			break;
	}
	return i;
}

static inline int Bconputs(int16 handle, const char *string)
{
	int err = Bconput(handle, string);
	Bconout(handle, '\r');
	Bconout(handle, '\n');
	return err;
}

#endif /* __ASSEMBLER__ */

/* 
 * Atari XBIOS calls
 */

#define IM_DISABLE	0
#define IM_RELATIVE	1
#define IM_ABSOLUTE	2
#define IM_KEYCODE	4

#define NVM_READ	0
#define NVM_WRITE	1
#define NVM_RESET	2
// unofficial
#define NVM_R_SEC	0
#define NVM_R_MIN	2
#define NVM_R_HOUR	4
#define NVM_R_MDAY	7
#define NVM_R_MON	8	/*+- 1*/
#define NVM_R_YEAR	9
#define NVM_R_VIDEO	29


#ifndef __ASSEMBLER__

//extern int32 xbios(uint16 nr, ...);


#define Initmous(mode, param, vec) toscallWPP(XBIOS_TRAP, 0, (int16)mode, (void *)param, (void *)vec)
#define Physbase() (void *)toscallV(XBIOS_TRAP, 2)
#define Logbase() (void *)toscallV(XBIOS_TRAP, 3)
//#define Getrez() toscallV(XBIOS_TRAP, 4)
#define Setscreen(log, phys, mode) toscallPPW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode)
#define VsetScreen(log, phys, mode, modecode) toscallPPW(XBIOS_TRAP, 5, (void *)log, (void *)phys, (int16)mode)
//#define Mfpint() toscallV(XBIOS_TRAP, 13, )
#define Rsconf(speed, flow, ucr, rsr, tsr, scr) toscallWWWWWW(XBIOS_TRAP, 15, (int16)speed, (int16)flow, (int16)ucr, (int16)rsr, (int16)tsr, (int16)scr)
//#define Keytbl(unshift, shift, caps) (KEYTAB *)toscallPPP(XBIOS_TRAP, 16, (char *)unshift, (char *)shift, (char *)caps)
#define Random() toscallV(XBIOS_TRAP, 17)
#define Gettime() (uint32)toscallV(XBIOS_TRAP, 23)
#define Jdisint(intno) toscallW(XBIOS_TRAP, 26, (int16)intno)
#define Jenabint(intno) toscallW(XBIOS_TRAP, 27, (int16)intno)
#define Supexec(func) toscallP(XBIOS_TRAP, 38, (void *)func)
//#define Puntaes() toscallV(XBIOS_TRAP, 39)
#define DMAread(sect, count, buf, dev) toscallLWPW(XBIOS_TRAP, 42, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define DMAwrite(sect, count, buf, dev) toscallWPLW(XBIOS_TRAP, 43, (int32)sect, (int16)count, (void *)buf, (int16)dev)
#define NVMaccess(op, start, count, buffer) toscallWWWP(XBIOS_TRAP, 46, (int16)op, (int16)start, (int16)count, (char *)buffer)
#define VsetMode(mode) toscallW(XBIOS_TRAP, 88, (int16)mode)
#define VgetMonitor() toscallV(XBIOS_TRAP, 89)
#define Locksnd() toscallV(XBIOS_TRAP, 128)
#define Unlocksnd() toscallV(XBIOS_TRAP, 129)

#endif /* __ASSEMBLER__ */

/* 
 * Atari GEMDOS calls
 */

#define SUP_USER		0
#define SUP_SUPER		1


#ifdef __ASSEMBLER__
#define SUP_SET			0
#define SUP_INQUIRE		1
#else

//extern int32 gemdos(uint16 nr, ...);

#define SUP_SET			(void *)0
#define SUP_INQUIRE		(void *)1

// official names
#define Pterm0() toscallV(GEMDOS_TRAP, 0)
#define Cconin() toscallV(GEMDOS_TRAP, 1)
#define Super(s) toscallP(GEMDOS_TRAP, 0x20, s)
#define Pterm(retcode) toscallW(GEMDOS_TRAP, 76, (int16)retcode)

#endif /* __ASSEMBLER__ */

#ifndef __ASSEMBLER__

/* 
 * MetaDOS XBIOS calls
 */

//#define _DISABLE	0

#ifndef __ASSEMBLER__

#define Metainit(buf) toscallP(XBIOS_TRAP, 48, (void *)buf)
#define Metaopen(drive, p) toscall>P(XBIOS_TRAP, 48, (void *)buf)
#define Metaclose(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
#define Metagettoc(drive, flag, p) toscallW(XBIOS_TRAP, 62, (int16)drive, (int16)flag, (void *)p)
#define Metadiscinfo(drive, p) toscallWP(XBIOS_TRAP, 63, (int16)drive, (void *)p)
#define Metaioctl(drive, magic, op, buf) toscallWLWP(XBIOS_TRAP, 55, (int16)drive, (int32)magic, (int16)op, )
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)
//#define Meta(drive) toscallW(XBIOS_TRAP, 50, (int16)drive)

#endif /* __ASSEMBLER__ */

/*
 * XHDI support
 * see http://toshyp.atari.org/010008.htm
 */

#define XHDI_COOKIE 'XHDI'
#define XHDI_MAGIC 0x27011992
#define XHDI_CLOBBER_LIST /* only d0 */

#define XH_TARGET_STOPPABLE		0x00000001L
#define XH_TARGET_REMOVABLE		0x00000002L
#define XH_TARGET_LOCKABLE		0x00000004L
#define XH_TARGET_EJECTABLE		0x00000008L
#define XH_TARGET_LOCKED		0x20000000L
#define XH_TARGET_STOPPED		0x40000000L
#define XH_TARGET_RESERVED		0x80000000L

#ifndef __ASSEMBLER__

/* pointer to the XHDI dispatch function */
extern void *gXHDIEntryPoint;

/* void (no) arg */
#define xhdicallV(callnr)						\
({												\
	register int32 retvalue __asm__("d0");		\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	call	(%[entry])\n"						\
	"	add.l	#2,%%sp\n"						\
	: "=r"(retvalue)	/* output */			\
	:							/* input */		\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})

#define xhdicallW(callnr, p1)					\
({												\
	register int32 retvalue __asm__("d0");		\
	int16 _p1 = (int16)(p1);					\
												\
	__asm__ volatile							\
	("/* xhdicall(" #callnr ") */\n"			\
	"	move.w	%1,-(%%sp) \n"					\
	"	move.w	%[calln],-(%%sp)\n"				\
	"	call	(%[entry])\n"						\
	"	add.l	#4,%%sp \n"						\
	: "=r"(retvalue)	/* output */			\
	: "r"(_p1),			/* input */				\
	  [entry]"a"(gXHDIEntryPoint),				\
	  [calln]"i"(callnr)						\
	: XHDI_CLOBBER_LIST /* clobbered regs */	\
	);											\
	retvalue;									\
})


#define XHGetVersion() (uint16)xhdicallV(0)
#define XHInqTarget(major, minor, bsize, flags, pname) xhdicallWWPPP(1, (uint16)major, (uint16)minor, (uint32 *)bsize, (uint32 *)flags, (char *)pname)
//XHReserve 2
//#define XHLock() 3
//#define XHStop() 4
#define XHEject(major, minor, doeject, key) xhdicallWWWW(5, (uint16)major, (uint16)minor, (uint16)doeject, (uint16)key)
#define XHDrvMap() xhdicallV(6)
//#define XHInqDev(dev,) xhdicall(7,)
//XHInqDriver 8
//XHNewCookie 9
#define XHReadWrite(major, minor, rwflags, recno, count, buf) xhdicalWWWLWP(10, (uint16)major, (uint16)minor, (uint16)rwflags, (uint32)recno, (uint16)count, (void *)buf)
#define XHInqTarget2(major, minor, bsize, flags, pname, pnlen) xhdicallWWPPPW(11, (uint16)major, (uint16)minor, (uint32 *)bsize, (uint32 *)flags, (char *)pname, (uint16)pnlen)
//XHInqDev2 12
//XHDriverSpecial 13
#define XHGetCapacity(major, minor, blocks, blocksize) xhdicall(14, (uint16)major, (uint16)minor, (uint32 *)blocks, (uint32 *)blocksize)
//#define XHMediumChanged() 15
//XHMiNTInfo 16
//XHDosLimits 17
//XHLastAccess 18
//SHReaccess 19

#endif /* __ASSEMBLER__ */


/*
 * error mapping
 * in debug.c
 */

extern status_t toserror(int32 err);
extern status_t xhdierror(int32 err);
extern void dump_tos_cookies(void);

/*
 * Cookie Jar access
 */

typedef struct tos_cookie {
	uint32 cookie;
	union {
		int32 ivalue;
		void *pvalue;
	};
} tos_cookie;

#define COOKIE_JAR (*((const tos_cookie **)0x5A0))

static inline const tos_cookie *tos_find_cookie(uint32 what)
{
	const tos_cookie *c = COOKIE_JAR;
	while (c && (c->cookie)) {
		if (c->cookie == what)
			return c;
		c++;
	}
	return NULL;
}

/*
 * OSHEADER access
 */

typedef struct tos_osheader {
	uint16 os_entry;
	uint16 os_version;
	void *reseth;
	struct tos_osheader *os_beg;
	void *os_end;
	void *os_rsv1;
	void *os_magic;
	uint32 os_date;
	uint32 os_conf;
	//uint32/16? os_dosdate;
	// ... more stuff we don't care about
} tos_osheader;

#define tos_sysbase ((const struct tos_osheader **)0x4F2)

static inline const struct tos_osheader *tos_get_osheader()
{
	if (!(*tos_sysbase))
		return NULL;
	return (*tos_sysbase)->os_beg;
}

#endif /* __ASSEMBLER__ */

/*
 * ARAnyM Native Features
 */

#define NF_COOKIE	0x5f5f4e46L	//'__NF'
#define NF_MAGIC	0x20021021L

#ifndef __ASSEMBLER__

typedef struct {
	uint32 magic;
	int32 (*nfGetID) (const char *);
	int32 (*nfCall) (int32 ID, ...);
} NatFeatCookie;

extern NatFeatCookie *gNatFeatCookie;
extern int32 gDebugPrintfNatFeatID;

static inline NatFeatCookie *nat_features(void)
{
	const struct tos_cookie *c;
	if (gNatFeatCookie == (void *)-1)
		return NULL;
	if (gNatFeatCookie)
		return gNatFeatCookie;
	c = tos_find_cookie(NF_COOKIE);
	if (c) {
		gNatFeatCookie = (NatFeatCookie *)c->pvalue;
		if (gNatFeatCookie && gNatFeatCookie->magic == NF_MAGIC) {
			return gNatFeatCookie;
		}
	}
	gNatFeatCookie = (NatFeatCookie *)-1;
	return NULL;
}

extern status_t init_nat_features(void);

static inline int32 nat_feat_getid(const char *name)
{
	NatFeatCookie *c = nat_features();
	if (!c)
		return 0;
	return c->nfGetID(name);
}

#define nat_feat_call(id, code, a...) \
({						\
	int32 ret;				\
	NatFeatCookie *c = nat_features();	\
	if (!c)					\
		return -1;			\
	ret = c->nfCall(id | code, a##...);	\
	ret;					\
})

extern void nat_feat_debugprintf(const char *str);

#endif /* __ASSEMBLER__ */

#ifdef __cplusplus
}
#endif

#endif /* _TOSCALLS_H */
