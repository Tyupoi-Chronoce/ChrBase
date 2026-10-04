#ifndef CKHASH_H
#define CKHASH_H

#include <cstdint>

/*
 * CKhash —— 静态库公共头文件
 * 与 _4algor.cpp（原 DLL 版）算法、命名规范完全一致，仅链接格式不同：
 *   - 符号为 extern "C"（C 链接，不被 name-mangle），故 C / C++ 均可直接链接；
 *   - 不再有 dllexport / DllMain，纯静态归档（libCKhash.a）。
 *
 * 默认参数仅在 C++ 下提供；纯 C 调用须显式传全参（见 #ifdef __cplusplus 守卫）。
 * mul_seeDefault 的默认值为 0xfcfbfaf9u，与原实现中的常量同名同值。
 *
 * CKhash — public header for the static library.
 * The algorithm and naming conventions are exactly the same as _4algor.cpp
 * (the original DLL build); only the linkage format differs:
 *   - Symbols use extern "C" (C linkage, no name mangling), so both C and C++
 *     can link directly.
 *   - No dllexport / DllMain; it is a plain static archive (libCKhash.a).
 *
 * Default arguments are provided under C++ only; pure-C callers must pass all
 * arguments explicitly (see the #ifdef __cplusplus guard).
 * The default value of mul_seeDefault is 0xfcfbfaf9u, identical in name and
 * value to the constant used in the original implementation.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* 自检测试：首次调用打印就绪信息，仅用于手测 */
/* Self-check / smoke test: prints readiness info on first call. For manual use only. */
void test(void);

/* 直接哈希：输入 32 位，经 times 轮乘法-移位混合，返回 32 位哈希。
 * 等价于 main.cpp 中 Hasher::hash_uint32 的算法（雪崩 / 分布质量最佳）。 */
/* Direct hash: takes a 32-bit input, mixes it through `times` rounds of
 * multiply-shift, and returns a 32-bit hash.
 * Equivalent to Hasher::hash_uint32 in main.cpp; best avalanche / distribution. */
uint32_t hash_uint32(uint32_t num
#ifdef __cplusplus
    , int times = 3
    , uint32_t seed_4mul = 0xfcfbfaf9u   /* = mul_seeDefault */
#endif
);

/* 建一层随机数表 randNum_arrToT（times=2..15 共 14 层，每层 65536 项）。
 * 调用 get_randNum / get_RandNumTable_value 前必须先建表。 */
/* Builds the first-level random table randNum_arrToT (14 layers for
 * times=2..15, each layer holds 65536 entries).
 * Must be called before get_randNum / get_RandNumTable_value. */
void create_randNumTable(uint32_t seed_4mul
#ifdef __cplusplus
    = 0xfcfbfaf9u   /* = mul_seeDefault */
#endif
);

/* 建二层表：先建一层表，再对一层表每项做二次哈希得到 randNum_arrHR2。
 * 调用 get_randNumHR2 前必须先调用本函数。 */
/* Builds the second-level table: first builds the first-level table, then
 * applies a second hash pass over every first-level entry to produce
 * randNum_arrHR2. Must be called before get_randNumHR2. */
void create_randNumTableHR2(uint32_t seed_4mul
#ifdef __cplusplus
    = 0xfcfbfaf9u   /* = mul_seeDefault */
#endif
);

/* 一层表取随机数（RNG 风格步进取数）：以 index 为起点，查 index+1 处表值作步长，
 * 前进步长后取值返回。先调 create_randNumTable。 */
/* First-level table random number (RNG-style stepped fetch): starting at
 * `index`, reads the table value at index+1 as a step, advances by that step,
 * and returns the value there. Call create_randNumTable first. */
uint32_t get_randNum(uint16_t index, int timesD
#ifdef __cplusplus
    = 0
#endif
);

/* 二层表取随机数：直接索引二层表（已二次哈希，16 位域上近似排列，无 step 折叠）。
 * 先调 create_randNumTableHR2。 */
/* Second-level table random number: directly indexes the second-level table
 * (already re-hashed, an approximate permutation over the 16-bit domain, no
 * step folding). Call create_randNumTableHR2 first. */
uint32_t get_randNumHR2(uint16_t index, int timesD
#ifdef __cplusplus
    = 0
#endif
);

/* 直接按层 / 下标取表值（无边界检查）。index1∈[0,13] 对应 times=2..15。 */
/* Directly fetch a table value by layer / subscript (no bounds checking).
 * index1 in [0,13] corresponds to times=2..15. */
uint32_t get_RandNumTable_value(uint32_t index1, uint32_t index2);

/*
 * ---- 以下为遗留接口，签名与原 DLL 完全一致，但已知存在实现问题，谨慎使用 ----
 *   copyRNT_2arr / copyRNTHR2_2arr：参数 `uint32_t* arr[][0x10000]` 结合内部
 *       `*arr[index1][index2] = ...` 的解引用方式，要求调用方传入二维指针数组，
 *       否则为未定义行为。
 *   arr2Hash_uint32：以 `numT = hash_uint32(index1, 0xf)` 的低位作掩码读取 arrIn，
 *       存在越界风险；且 res_arr 形状假设为 [length][length]。
 *
 * ---- The following are legacy interfaces whose signatures match the original
 *      DLL exactly, but are known to have implementation issues; use with care ----
 *   copyRNT_2arr / copyRNTHR2_2arr: the parameter `uint32_t* arr[][0x10000]`
 *       combined with the internal dereference `*arr[index1][index2] = ...`
 *       requires the caller to pass a 2D array of pointers, otherwise it is
 *       undefined behavior.
 *   arr2Hash_uint32: uses the low bits of `numT = hash_uint32(index1, 0xf)` as
 *       a mask to read arrIn, which risks out-of-bounds access; and assumes
 *       res_arr has shape [length][length].
 */
void copyRNT_2arr(uint32_t* arr[][0x10000]);
void copyRNTHR2_2arr(uint32_t* arr[][0x10000]);
void arr2Hash_uint32(const uint32_t* arrIn, uint32_t length, uint32_t* res_arr
#ifdef __cplusplus
    , int times = 3
    , uint32_t seed_4mul = 0xfcfbfaf9u   /* = mul_seeDefault */
#endif
);

#ifdef __cplusplus
}
#endif

#endif /* CKHASH_H */
