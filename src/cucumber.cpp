#include "cucumber.h"
#include "c_stdlib.h"
#include <revolution.h>

extern u8 __cu_dynamicCodeStart;
extern u32 __cu_dynamicNum;
extern u8* __cu_dynamicLoc[];
static u8** __cu_patchBuffer = 0;

inline void invalidateCacheAtAdr(void* adr) {
    register void* r_adr = adr;
    __asm {
        dcbst 0, r_adr
        sync
        icbi 0, r_adr
    }
}



Cucumber(Cucumber::sCucumber);

Cucumber::Cucumber() {
    __cu_dynamicLoc[0] = &__cu_dynamicCodeStart;
}

void Cucumber::loadDynamicCore(u8* pCodeBlob, u32 code) {
    char fatalMsg[128];
    u32 maxNum = __cu_dynamicNum;
    u8** pDynamics = __cu_dynamicLoc;

    cuRelocHeader* pHeader = (cuRelocHeader*)loadCodeRelTemporary(code);
    if (pHeader->mMagic1 != 0x63755265 || pHeader->mMagic2 != (0x6C6F6300 | cuVersion)) {
        snprintf(fatalMsg, 128, "ERROR!\nFile %d is not a cucumber reloc file\nor is corrupted!", code);
        fatalErrorReport(fatalMsg);
        return;
    }

    cuShortHeader* pGlobals = (cuShortHeader*)loadCodeGloTemporary();
    if (pGlobals->mMagic1 != 0x6375476C || pGlobals->mMagic2 != (0x6F626C00 | cuVersion)) {
        fatalErrorReport("ERROR!\nFile is not a cucumber global file\nor is corrupted!");
        return;
    }


    BOOL interpt = OSDisableInterrupts();
    
    pDynamics[code] = pCodeBlob;
    u8* pCodeBlobEnd = pCodeBlob + pHeader->mCodeSize;

    for (u8* i = pCodeBlob; i < pCodeBlobEnd; i++)
        invalidateCacheAtAdr(i);

    if (pHeader->mPatchSize > 0) {
        if (!__cu_patchBuffer) {
            __cu_patchBuffer = new u8* [maxNum - 1];
            for (u32 i = 0; i < maxNum - 1; i++)
                __cu_patchBuffer[i] = 0;
        }

        u8* pPatchBuff = new u8 [pHeader->mPatchSize];
        __cu_patchBuffer[code - 1] = pPatchBuff;
        doReloc((u8*)pHeader, pHeader->mPatchEntries, pPatchBuff, pCodeBlob, 0);
    }

    doReloc((u8*)pHeader, pHeader->mStaticExternalEntries, pCodeBlob, 0, 1);
    doReloc((u8*)pHeader, pHeader->mRelocEntries, pCodeBlob, pCodeBlob, 2);

    // ctors
    typedef void (*Func)();
    Func* pCtorLoc = (Func*)(pCodeBlob + pHeader->mCtorsLoc);
    Func* pCtorEnd = (Func*)(pCodeBlob + pHeader->mCtorsEnd);


    // link globals
    for (u32 i = 0; i < maxNum; i++) {
        if (i != code && pDynamics[i]) {
            // link self
            doReloc((u8*)pGlobals, pGlobals->mRelocEntries[maxNum * code + i], pCodeBlob, pDynamics[i], 2);

            // link external
            doReloc((u8*)pGlobals, pGlobals->mRelocEntries[maxNum * i + code], pDynamics[i], pCodeBlob, 2);
        }
    }

    __sync();
    __isync();

    OSRestoreInterrupts(interpt);

    freeCodeRelTemporary(pHeader);
    freeCodeGloTemporary(pGlobals);
    

    // do Ctors
    for (Func* f = pCtorLoc; f < pCtorEnd; f++)
        (*f)();
}

void Cucumber::unloadAllCore() {
    u32 maxNum = __cu_dynamicNum;
    u8** pDynamicCode = __cu_dynamicLoc;
    u8** pPatchBuffer = __cu_patchBuffer;

    cuShortHeader* pReset = (cuShortHeader*)loadCodeResTemporary();
    if (pReset->mMagic1 != 0x63755265 || pReset->mMagic2 != (0x73657400 | cuVersion)) {
        fatalErrorReport("ERROR!\nFile is not a cucumber reset file\nor is corrupted!");
        return;
    }

    BOOL interpt = OSDisableInterrupts();
    doReloc((u8*)pReset, pReset->mRelocEntries[0], pDynamicCode[0], pDynamicCode[0], 2);
    OSRestoreInterrupts(interpt);

    freeCodeResTemporary(pReset);

    // undo patches
    if (pPatchBuffer) {
        for (u32 i = 1; i < maxNum; i++) {
            if (pPatchBuffer[i - 1]) {
                cuRelocHeader* pHeader = (cuRelocHeader*)loadCodeRelTemporary(i);

                interpt = OSDisableInterrupts();
                undoPatches((const u8*)pHeader, pHeader->mPatchEntries, pPatchBuffer[i - 1]);
                OSRestoreInterrupts(interpt);

                freeCodeRelTemporary(pHeader);
            }
        }
        __cu_patchBuffer = 0;
    }

    __sync();
    __isync();
    
    for (u32 i = 1; i < maxNum; i++)
        pDynamicCode[i] = 0;
}

void Cucumber::doReloc(const u8* relocDat, const cuRelocEntry& rReloc, u8* target, const u8* dest, u32 relocType) {
    if (rReloc.mLength == 0)
        return;

    cuAddressReader pReloc;
    pReloc.mU8 = relocDat + rReloc.mOffset;
    const u8* pRelocEnd = pReloc.mU8 + rReloc.mLength;

    while (pReloc.mU8 < pRelocEnd) {
        u32 type;
        cuAddressWriter adr;
        u32 data = 0;

        switch(relocType) {
            case 2: {
                data = (u32)dest;
            }

            case 1: {
                type = *pReloc.mU32++;
                adr.mU8 = target + (type & 0xFFFFFF);
                type >>= 24;
                data += *pReloc.mU32++;
                break;
            }

            default: {
                type = *pReloc.mU8++;
                adr.mU8 = (u8*)(*pReloc.mU32++);
                data = *pReloc.mU32++;

                // buffer original data
                cuAddressWriter patchBuffer;
                patchBuffer.mU8 = target;
                //*patchBuffer.mU32++ = (u32)adr.mU8;
                switch (type) {
                    case cuPatchWriteAddr32:
                    case cuPatchBranch:
                    case cuPatchCall: {
                        data += (u32)dest;
                    }
                    case cuPatchWrite32: {
                        *patchBuffer.mU32++ = *adr.mU32;
                        break;
                    }
                    case cuPatchWriteAddr16Lo:
                    case cuPatchWriteAddr16Hi:
                    case cuPatchWriteAddr16Ha: {
                        data += (u32)dest;
                    }
                    case cuPatchWrite16: {
                        *patchBuffer.mU16++ = *adr.mU16;
                        break;
                    }

                    case cuPatchWrite8: {
                        *patchBuffer.mU8++ = *adr.mU8;
                        break;
                    }

                    default: {
                        fatalErrorReport("ERROR!\nInvalid or Unknown patch type\nwhile buffering original code data.");
                        return;
                    }
                }

                target = patchBuffer.mU8;
                break;
            }
        }

        switch (type) {
            case cuPatchWriteAddr32:
            case cuPatchWrite32: {
                *adr.mU32 = data;
                break;
            }
            case cuPatchWriteAddr16Lo:
            case cuPatchWrite16: {
                *adr.mU16 = data;
                break;
            }
            case cuPatchWriteAddr16Hi: {
                *adr.mU16 = data >> 16;
                break;
            }
            case cuPatchWriteAddr16Ha: {
                *adr.mU16 = (data >> 16) + (data >> 15 & 1);
                break;
            }
            case cuPatchWriteRel24: {
                *adr.mU32 = (*adr.mU32 & 0xFC000003) | ((data - (u32)adr.mU8) & 0x3FFFFFC);
                break;
            }

            case cuPatchWrite8: {
                *adr.mU8 = data;
                break;
            }
            case cuPatchBranch: {
                *adr.mU32 = 0x48000000 | ((data - (u32)adr.mU8) & 0x3FFFFFC);
                break;
            }
            case cuPatchCall: {
                *adr.mU32 = 0x48000001 | ((data - (u32)adr.mU8) & 0x3FFFFFC);
                break;
            }

            default: {
                fatalErrorReport("ERROR!\nInvalid or Unknown patch type.");
                return;
            }
        }

        invalidateCacheAtAdr(adr.mU8);
    }
}

void Cucumber::undoPatches(const u8* relocDat, const cuRelocEntry& rReloc, const u8* patch) {
    cuAddressReader pReloc;
    cuAddressReader patchBuffer;
    cuAddressWriter adr;

    pReloc.mU8 = relocDat + rReloc.mOffset;
    const u8* pRelocEnd = pReloc.mU8 + rReloc.mLength;
    patchBuffer.mU8 = patch;

    while (pReloc.mU8 < pRelocEnd) {
        u32 type = *pReloc.mU8++;
        adr.mU8 = (u8*)(*pReloc.mU32++);
        pReloc.mU32++; // advance pointer

        // buffer original data
        switch (type) {
            case cuPatchWriteAddr32:
            case cuPatchBranch:
            case cuPatchCall:
            case cuPatchWrite32: {
                *adr.mU32 = *patchBuffer.mU32++;
                break;
            }
            case cuPatchWriteAddr16Lo:
            case cuPatchWriteAddr16Hi:
            case cuPatchWriteAddr16Ha:
            case cuPatchWrite16: {
                *adr.mU16 = *patchBuffer.mU16++;
                break;
            }

            case cuPatchWrite8: {
                *adr.mU8 = *patchBuffer.mU8++;
                break;
            }
        }

        invalidateCacheAtAdr(adr.mU8);
    }
}



void Cucumber::loadDynamic(u32 code) {
    if (code == 0 || code >= __cu_dynamicNum || __cu_dynamicLoc[code])
        return;
    
    loadDynamicCore((u8*)loadCodeBlobToMainMem(code), code);
}

void Cucumber::loadTemporaryDynamic(u32 code) {
    if (code == 0 || code >= __cu_dynamicNum || __cu_dynamicLoc[code])
        return;
    
    loadDynamicCore((u8*)loadCodeBlobToMainMemTemporary(code), code);
}

void Cucumber::unloadAll() {
    u32 maxNum = __cu_dynamicNum;
    u8** pDynamicCode = __cu_dynamicLoc;
    bool isNoDynamicsLoaded = true;

    for (u32 i = 1; i < maxNum; i++) {
        if (pDynamicCode[i]) {
            isNoDynamicsLoaded = false;
            break;
        }
    }

    if (!isNoDynamicsLoaded)
        unloadAllCore();
}

#ifdef cuDebug
bool Cucumber::fallbackOSReport(const char* pFile) {
    register u8* val;
    __asm {
        mflr val
    }
    OSReport("Unloaded function call at 0x%X in file %s\n", val - 4, pFile);
    return false;
}
#endif