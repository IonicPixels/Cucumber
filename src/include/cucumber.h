#pragma once

#include <revolution/types.h>

#define cuVersion 1

// create patch sections
#pragma section ".cucumber"

// patch types, further evaluation in Cucumbuilder
#define cuPatchWriteAddr32 1
#define cuPatchWriteAddr16Lo 4
#define cuPatchWriteAddr16Hi 5
#define cuPatchWriteAddr16Ha 6
#define cuPatchWriteRel24 10
#define cuPatchWrite32 40
#define cuPatchWrite16 41
#define cuPatchWrite8 42
#define cuPatchBranch 43
#define cuPatchCall 44

// _cu_"name"
#define cuNameGen(prefix, name) \
    _cu_##prefix##name
#define cuNameVar(count) \
    cuNameGen(var, count)
#define cuNameFunc(count) \
    cuNameGen(func, count)
#ifndef _VSCODE
#define cuData3(name) \
    __declspec(section ".cucumber") static const u32 cuNameVar(name)
#else
#define cuData3(name) \
    static const u32 cuNameVar(name)
#endif

// write value, supports both static and pointer values
#define cuWrite32(adr, val) \
    cuData3(__COUNTER__)[3] = { cuPatchWriteAddr32, (u32)(adr), (u32)(val) }
#define cuWrite16(adr, val) \
    cuData3(__COUNTER__)[3] = { cuPatchWriteAddr16Lo, (u32)(adr), (u32)(val) }
#define cuWrite16HI(adr, val) \
    cuData3(__COUNTER__)[3] = { cuPatchWriteAddr16Hi, (u32)(adr), (u32)(val) }
#define cuWrite16HA(adr, val) \
    cuData3(__COUNTER__)[3] = { cuPatchWriteAddr16Ha, (u32)(adr), (u32)(val) }
#define cuWrite8(adr, val) \
    cuData3(__COUNTER__)[3] = { cuPatchWrite8, (u32)(adr), (u32)(val) }

    // branches, supports both static and pointer values
#define cuBranch(adr, ptr) \
    cuData3(__COUNTER__)[3] = { cuPatchBranch, (u32)(adr), (u32)(ptr) }
#define cuCall(adr, ptr) \
    cuData3(__COUNTER__)[3] = { cuPatchCall, (u32)(adr), (u32)(ptr) }


// instructions

// ASM
#define cuNop                   0x60000000
#define cuBlr                   0x4E800020
#define cuCmplwi(RA, I)         (0x28000000 | ((RA & 0x1F) << 16) | (I & 0xFFFF))
#define cuCmpwi(RA, I)          (0x2C000000 | ((RA & 0x1F) << 16) | (I & 0xFFFF))
#define cuAddi(RT, RA, I)       (0x38000000 | ((RT & 0x1F) << 21) | ((RA & 0x1F) << 16) | (I & 0xFFFF))
#define cuAddis(RT, RA, I)      (0x3C000000 | ((RT & 0x1F) << 21) | ((RA & 0x1F) << 16) | (I & 0xFFFF))
#define cuOr(RA, RS, RB)        (0x7C000378 | ((RS & 0x1F) << 21) | ((RA & 0x1F) << 16) | ((RB & 0x1F) << 11))
#define cuLwz(RT, D, RA)        (0x80000000 | ((RT & 0x1F) << 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuLbz(RT, D, RA)        (0x88000000 | ((RT & 0x1F) << 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuStw(RS, D, RA)        (0x90000000 | ((RS & 0x1F) << 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuLhz(RT, D, RA)        (0xA0000000 | ((RT & 0x1F) << 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuSth(RS, D, RA)        (0xB0000000 | ((RS & 0x1F) << 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuLfs(FRT, D, RA)       (0xC0000000 | ((FRT & 0x1F)<< 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuStfs(FRS, D, RA)      (0xD0000000 | ((FRS & 0x1F)<< 21) | ((RA & 0x1F) << 16) | (D & 0xFFFF))
#define cuFmr(FRT, FRB)         (0xFC000090 | ((FRT & 0x1F)<< 21) | ((FRB & 0x1F)<< 11))

#define cuLi(RT, I)             cuAddi(RT, 0, I)
#define cuLis(RT, I)            cuAddis(RT, 0, I)
#define cuMr(RA, RS)            cuOr(RA, RS, RS)

// ASM patch macros
#define cuWriteNop(adr)                     cuWrite32(adr, cuNop)
#define cuWriteBlr(adr)                     cuWrite32(adr, cuBlr)
#define cuWriteCmplwi(adr, RA, val)         cuWrite32(adr, cuCmplwi(RA, val))
#define cuWriteCmpwi(adr, RA, val)          cuWrite32(adr, cuCmpwi(RA, val))
#define cuWriteAddi(adr, RT, RA, val)       cuWrite32(adr, cuAddi(RT, RA, val))
#define cuWriteAddis(adr, RT, RA, val)      cuWrite32(adr, cuAddis(RT, RA, val))
#define cuWriteLi(adr, RT, val)             cuWrite32(adr, cuLi(RT, val))
#define cuWriteLis(adr, RT, val)            cuWrite32(adr, cuLis(RT, val))
#define cuWriteMr(adr, RT, RS)              cuWrite32(adr, cuMr(RT, RS))
#define cuWriteLwz(adr, RT, val, RA)        cuWrite32(adr, cuLwz(RT, val, RA))
#define cuWriteLbz(adr, RT, val, RA)        cuWrite32(adr, cuLbz(RT, val, RA))
#define cuWriteStw(adr, RS, val, RA)        cuWrite32(adr, cuStw(RS, val, RA))
#define cuWriteLhz(adr, RT, val, RA)        cuWrite32(adr, cuLhz(RT, val, RA))
#define cuWriteSth(adr, RS, val, RA)        cuWrite32(adr, cuSth(RS, val, RA))
#define cuWriteLfs(adr, FRT, val, RA)       cuWrite32(adr, cuLfs(FRT, val, RA))
#define cuWriteStfs(adr, FRS, val, RA)      cuWrite32(adr, cuStfs(FRS, val, RA))
#define cuWriteFmr(adr, FRT, FRB)           cuWrite32(adr, cuFmr(FRT, FRB))

    // use with pointer values, can be pointers to externals or inside custom code, but also static values
#define cuWriteAddiPtr(adr, RT, RA, ptr)    cuWrite16(adr, cuAddi(RT, RA, 0) >> 16); cuWrite16((u32)(adr) + 2, ptr)
#define cuWriteAddisPtr(adr, RT, RA, ptr)   cuWrite16(adr, cuAddis(RT, RA, 0) >> 16); cuWrite16((u32)(adr) + 2, ptr)
#define cuWriteLiPtr(adr, RT, ptr)          cuWrite16(adr, cuLi(RT, 0) >> 16); cuWrite16((u32)(adr) + 2, ptr)
#define cuWriteLisPtrHi(adr, RT, ptr)       cuWrite16(adr, cuLis(RT, 0) >> 16); cuWrite16HI((u32)(adr) + 2, ptr)
#define cuWriteLisPtrHa(adr, RT, ptr)       cuWrite16(adr, cuLis(RT, 0) >> 16); cuWrite16HA((u32)(adr) + 2, ptr)
#define cuWriteLwzPtr(adr, RT, ptr, RA)     cuWrite16(adr, cuLwz(RT, 0, RA) >> 16);  cuWrite16((u32)(adr) + 2, ptr)


class Cucumber {
private:
    Cucumber();

    struct cuRelocEntry {
        u32 mOffset;
        u32 mLength;
    };
    struct cuShortHeader {
        u32 mMagic1; //_0
        u32 mMagic2; //_4
        cuRelocEntry mRelocEntries[]; //_8
    };
    struct cuRelocHeader {
        u32 mMagic1; //_0
        u32 mMagic2; //_4
        u32 mCodeSize; //_8
        u32 mPatchSize; //_C
        u32 mCtorsLoc; //_10
        u32 mCtorsEnd; //_14
        cuRelocEntry mPatchEntries; //_18
        cuRelocEntry mStaticExternalEntries; //_20
        cuRelocEntry mRelocEntries; //_28
    };
    union cuAddressReader {
        const u8* mU8;
        const u16* mU16;
        const u32* mU32;
    };
    union cuAddressWriter {
        u8* mU8;
        u16* mU16;
        u32* mU32;
    };

    static void loadDynamicCore(u8*, u32);
    static void unloadAllCore();
    static void fatalErrorReport(const char*);
    static void doReloc(const u8* relocDat, const cuRelocEntry&, u8* target, const u8* dest, u32 type);
    static void undoPatches(const u8* relocDat, const cuRelocEntry&, const u8*);

    static void* loadCodeSimple();
    static void* loadCodeRelSimple();
    static void doRelocSimple(const u8* relocDat, const cuRelocEntry&, u8* target, u32 type);
    static Cucumber sCucumber; // for initializing the custom code

    // custom user defined functions
    static void* loadCodeBlobToMainMem(u32);
    static void* loadCodeBlobToMainMemTemporary(u32);
    static void* loadCodeRelTemporary(u32);
    static void* loadCodeGloTemporary();
    static void* loadCodeResTemporary();
    static void freeCodeRelTemporary(void*);
    static void freeCodeGloTemporary(void*);
    static void freeCodeResTemporary(void*);

public:
    static void loadDynamicSimple();
    static void loadDynamic(u32);
    static void loadTemporaryDynamic(u32);
    static void unloadAll();
    #ifdef cuDebug
    static bool fallbackOSReport(const char*);
    #else
    inline static bool fallbackOSReport(const char*) { return false; }
    #endif

    // custom user defined functions
    static void freeCodeBlobTemporary(void*);
};

#ifdef __cplusplus
#define cuFallbackFunction \
extern "C" { bool __cu_fallbackFunc() { return Cucumber::fallbackOSReport(__FILE__); } }
#else
#define cuFallbackFunction \
bool __cu_fallbackFunc() { return Cucumber::fallbackOSReport(__FILE__); }
#endif