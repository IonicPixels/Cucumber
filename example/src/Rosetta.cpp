#include "Rosetta.h"
#include "JSystem/JKernel/JKRDecomp.h"
#include <revolution.h>
#include "c_stdlib.h"

cuBranch(roReg3(0x804A19D0), Cucumber::loadDynamicSimple);

void* allocMemoryFromMemArena(u32 s, bool temporary) {
    if (!temporary)
        return OSAllocFromMEM1ArenaLo(s, 4);
    else
        return (void*)(((u32)OSGetMEM1ArenaLo() + 3) & ~3);
}

void* loadCodeFileToMainRAMSimple(const char* pFileName, bool loadTemporary) {
    DVDFileInfo file;

    if (!DVDOpen(pFileName, &file)) {
        char msg[128];
        u32 color = 0xFFFFFFFF, bg = 0x000000FF;

        snprintf(msg, 128, "FATAL ERROR!\nCan not load code!\nMissing file:\n%s", pFileName);
        OSFatal(&color, &bg, msg);
    }

    u8* pData = (u8*)(((u32)OSGetMEM1ArenaHi() - file.length - 31) & ~31);
    u8* pOut;
    
    DVDReadPrio(&file, pData, file.length, 0, 2);
    DVDClose(&file);

    if (JKRDecomp::checkCompressed(pData) > 0) {
        u32 outSize = JKRDecompExpandSize(pData);
        pOut = (u8*)allocMemoryFromMemArena(outSize, loadTemporary);
        JKRDecomp::decode(pData, pOut, outSize, 0);
    }
    else {
        pOut = (u8*)allocMemoryFromMemArena(file.length, loadTemporary);
        memcpy(pOut, pData, file.length);
    }

    return pOut;
}

void* Cucumber::loadCodeSimple() {
    return loadCodeFileToMainRAMSimple("/CodeData/CustomCode_Main.bin", false);
}

void* Cucumber::loadCodeRelSimple() {
    return loadCodeFileToMainRAMSimple("/CodeData/CustomCode_Main.rel", true);
}

void Cucumber::fatalErrorReport(const char* pMsg) {
    u32 color = 0xFFFFFFFF, bg = 0x000000FF;
    OSFatal(&color, &bg, pMsg);
}