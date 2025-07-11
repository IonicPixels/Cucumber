#include <revolution/types.h>

u32 JKRDecompExpandSize(u8*);

class JKRDecomp {
public:
    static void decode(u8*, u8*, u32, u32);
    static s32 checkCompressed(u8*);
};