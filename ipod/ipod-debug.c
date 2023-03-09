// compile with gcc ipod-debug.c -g -o ipod-debug -m32

#include <stdint.h>

typedef struct AND_Data {
    uint32_t signature;
    uint16_t pageSizeFactor;
    uint16_t pagesPerBlock;
    uint32_t nandType;
    uint16_t oob;
    uint8_t unused;
    uint8_t fil_ecc_refresh_threshold;
    uint16_t bankCnt;
    uint16_t userBlockPerBank;
    uint32_t initialBBType;
    uint32_t field_18;
    uint16_t blocksPerBank;
    uint16_t hyperBlockCnt;
    uint32_t pagesPerHyperblock;
    uint32_t field_24;
    uint32_t field_28;
    uint16_t hyperBlockCnt_;
    uint16_t userHyperBlockCnt;
    uint32_t userPageCnt;
    uint16_t pageSize;
    uint16_t pagesPerBlockExponent;
    uint16_t blocksPerHyperBlock;
    uint16_t blocksPerHyperBlockAndBank;
} AND_Data;

AND_Data** and_data;

typedef struct FIL_funcTbl {
    void (*resetAndVerifyIds)();
    int (*readSinglePage)(uint16_t bank, uint32_t page, void* data, void* arg3, uint8_t* arg4);
    int (*readNoECC)(uint16_t bank, uint32_t block, void* arg2, void* arg3);
    int (*readSequentialPages)(uint32_t* arg0, void* arg1, void* arg2, uint16_t arg3, uint8_t* arg4);
    int (*readScatteredPages)(uint16_t* bank, uint32_t* arg1, void* arg2, void* arg3, uint32_t arg4, uint8_t* arg5);
    int (*readSinglePageNoMetadata)(uint16_t bank, uint32_t page, void* data);
    int (*writeScatteredPages)(uint16_t* bank, uint32_t* arg1, void* arg2, void* arg3, void* arg4);
    int (*writeSequentialPages)(uint32_t* arg0, void* arg1, void* arg2, uint16_t arg3, uint32_t arg4, uint32_t arg5);
    int (*writeSinglePage)(uint16_t bank, void* arg1, void* arg2, void* arg3);
    int (*eraseSingleBlock)(uint16_t bank, uint16_t block);
    int (*eraseSequentialBlocks)(uint16_t* arg0, uint32_t arg1, uint32_t arg2);
    int (*writeSinglePageNoMetadata)(uint16_t bank, uint32_t arg1, void* arg2);
    void (*convert_P2C_fn)(uint32_t vBlock_in_hBlock, void* arg1, uint32_t* arg2, uint32_t* arg3);
    void (*convert_C2P_fn)(void* arg0, void* arg1, uint32_t* arg2, uint32_t* arg3);
} FIL_funcTbl;

FIL_funcTbl* fil_func_tbl;

int main() {
    return 0;
}