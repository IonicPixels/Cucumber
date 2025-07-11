#include "cucumber.h"

inline void invalidateCacheAtAdr(void* adr) {
    register void* r_adr = adr;
    __asm {
        dcbst 0, r_adr
        sync
        icbi 0, r_adr
    }
}

void Cucumber::loadDynamicSimple() {
    u8* pCodeBlob = (u8*)loadCodeSimple();
    cuRelocHeader* pHeader = (cuRelocHeader*)loadCodeRelSimple();

    if (pHeader->mMagic1 != 0x63755265 || pHeader->mMagic2 != (0x6C6F6300 | cuVersion)) {
        fatalErrorReport("ERROR!\nFile is not a cucumber reloc file\nor is corrupted!");
        return;
    }
    
    u8* pCodeBlobEnd = pCodeBlob + pHeader->mCodeSize;

    for (u8* i = pCodeBlob; i < pCodeBlobEnd; i++)
        invalidateCacheAtAdr(i);

    if (pHeader->mPatchSize > 0)
        doRelocSimple((u8*)pHeader, pHeader->mPatchEntries, pCodeBlob, 0);

    doRelocSimple((u8*)pHeader, pHeader->mStaticExternalEntries, pCodeBlob, 1);
    doRelocSimple((u8*)pHeader, pHeader->mRelocEntries, pCodeBlob, 2);

    __sync();
    __isync();
    
    // do Ctors
    typedef void (*Func)();
    Func* pCtorLoc = (Func*)(pCodeBlob + pHeader->mCtorsLoc);
    Func* pCtorEnd = (Func*)(pCodeBlob + pHeader->mCtorsEnd);
    for (Func* f = pCtorLoc; f < pCtorEnd; f++)
        (*f)();
}

void Cucumber::doRelocSimple(const u8* relocDat, const cuRelocEntry& rReloc, u8* target, u32 relocType) {
    cuAddressReader pReloc;
    pReloc.mU8 = relocDat + rReloc.mOffset;
    const u8* pRelocEnd = pReloc.mU8 + rReloc.mLength;

    while (pReloc.mU8 < pRelocEnd) {
        u32 type;
        cuAddressWriter adr;
        u32 data = 0;

        switch(relocType) {
            case 2: {
                data = (u32)target;
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

                switch (type) {
                    case cuPatchWriteAddr32:
                    case cuPatchWriteAddr16Lo:
                    case cuPatchWriteAddr16Hi:
                    case cuPatchWriteAddr16Ha:
                    case cuPatchBranch:
                    case cuPatchCall: {
                        data += (u32)target;
                    }
                    case cuPatchWrite32:
                    case cuPatchWrite16:
                    case cuPatchWrite8:
                        break;

                    default: {
                        fatalErrorReport("ERROR!\nInvalid or Unknown patch type\nwhile buffering original code data.");
                        return;
                    }
                }
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