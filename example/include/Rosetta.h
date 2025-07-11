#pragma once

#include "cucumber.h"

// reg difference 1: 8022441c 8022469c
// reg difference 1: JAP: 8039EFB8, USA: 8039EFBC, PAL: 8039EFD4
// default regional difference 2 usually somewhere after 0x803A0000 and somewhere before 0x804A0000
// default regional difference 3 usually somewhere after 0x804A0000
#ifdef USA
#define roRegDiff1 0
#define roRegDiff2 0
#define roRegDiff3 0
#define roRegDiff4 0
#endif

#ifdef EUR
#define roRegDiff1 0x1C
#define roRegDiff2 0x1C
#define roRegDiff3 0
#define roRegDiff4 0x10
#endif

#ifdef JAP
#define roRegDiff1 0
#define roRegDiff2 -0x4
#define roRegDiff3 -0x20
#define roRegDiff4 0
#endif

#define roReg1(adr) (u32)(adr) + roRegDiff1
#define roReg2(adr) (u32)(adr) + roRegDiff2
#define roReg3(adr) (u32)(adr) + roRegDiff3
#define roReg4(adr) (u32)(adr) + roRegDiff4
