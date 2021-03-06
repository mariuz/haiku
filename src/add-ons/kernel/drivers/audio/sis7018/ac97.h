/*
 * Copyright (c) 1999 Cameron Grant <gandalf@vilnya.demon.co.uk>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/sound/pcm/ac97.h,v 1.3.2.5 2001/08/01 03:41:03 cg Exp $
 */

/*
 * This file is a part of SiS 7018 BeOS driver project.
 * Copyright permissions see above.
 *
 * Portion copyright (c) 2002-2003 by Siarzuk Zharski <imker@gmx.li>
 *
 * $Source: /cvsroot/sis4be/sis7018/ac97.h,v $
 * $Author: zharik $
 * $Revision: 1.4 $
 * $Date: 2003/04/08 19:18:37 $
 */
 
#define AC97_MUTE  0x8000

#define AC97_REG_RESET  0x00
#define AC97_MIX_MASTER  0x02
#define AC97_MIX_PHONES  0x04
#define AC97_MIX_MONO   0x06
#define AC97_MIX_TONE  0x08
#define AC97_MIX_BEEP  0x0a
#define AC97_MIX_PHONE  0x0c
#define AC97_MIX_MIC  0x0e
#define AC97_MIX_LINE  0x10
#define AC97_MIX_CD  0x12
#define AC97_MIX_VIDEO  0x14
#define AC97_MIX_AUX  0x16
#define AC97_MIX_PCM  0x18
#define AC97_REG_RECSEL  0x1a
#define AC97_MIX_RGAIN  0x1c
#define AC97_MIX_MGAIN  0x1e
#define AC97_REG_GEN  0x20
#define AC97_REG_3D  0x22
#define AC97_REG_POWER  0x26
#define AC97_REGEXT_ID    0x28
#define   AC97_EXTCAP_VRA    (1 << 0)
#define   AC97_EXTCAP_DRA    (1 << 1)
#define   AC97_EXTCAP_VRM    (1 << 3)
#define    AC97_EXTCAPS (AC97_EXTCAP_VRA | AC97_EXTCAP_DRA | AC97_EXTCAP_VRM)
#define AC97_REGEXT_STAT  0x2a
#define AC97_REGEXT_FDACRATE  0x2c
#define AC97_REGEXT_SDACRATE  0x2e
#define AC97_REGEXT_LDACRATE  0x30
#define AC97_REGEXT_LADCRATE  0x32
#define AC97_REGEXT_MADCRATE  0x34
#define AC97_MIXEXT_CLFE  0x36
#define AC97_MIXEXT_SURROUND  0x38
#define AC97_REG_ID1  0x7c
#define AC97_REG_ID2  0x7e

/* Added by S.Zharski*/

status_t ac97init(sis7018_dev *dev);

int      ac97read(sis7018_dev *dev, int regno);
status_t ac97write(sis7018_dev *dev, int regno, uint32 data);

status_t ac97get_params(sis7018_dev *dev, sound_setup *sound);
status_t ac97set_params(sis7018_dev *dev, sound_setup *sound);

void ac97dump(int card_id);

/*
Siarzhuk Zharski: Following not needed in BeOS build. =-)
*/
/*#define  AC97_F_EAPD_INV    0x00000001

#define AC97_DECLARE(name) static DEFINE_CLASS(name, name ## _methods, sizeof(struct kobj))
#define AC97_CREATE(dev, devinfo, cls) ac97_create(dev, devinfo, &cls ## _class)

struct ac97_info;

#include "ac97_if.h"

extern kobj_class_t ac97_getmixerclass(void);

struct ac97_info *ac97_create(device_t dev, void *devinfo, kobj_class_t cls);
void ac97_destroy(struct ac97_info *codec);
void ac97_setflags(struct ac97_info *codec, u_int32_t val);
u_int32_t ac97_getflags(struct ac97_info *codec);
int ac97_setrate(struct ac97_info *codec, int which, int rate);
int ac97_setextmode(struct ac97_info *codec, u_int16_t mode);
u_int16_t ac97_getextmode(struct ac97_info *codec);
u_int16_t ac97_getextcaps(struct ac97_info *codec);
*/
