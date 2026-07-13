#ifndef __4ALGOR_H__
#define __4ALGOR_H__

#include <cstdint>

constexpr int max4timesArg = 0xf;
constexpr int min4timesArg = 0x2;
constexpr uint32_t length_sizeof_uint16 = 0xffff + 1;

constexpr uint32_t xor_seed = 0xdeadc01d;
constexpr uint32_t mul_seeDefault = 0xfcfbfaf9;
constexpr uint32_t num1 = 0x9e3779b1;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __4ALGOR_EXPORTS
#define EXPT __declspec(dllexport)
#else
#define EXPT __declspec(dllimport)
#endif

EXPT void test();

EXPT uint32_t get_RandNumTable_value(uint32_t index1, uint32_t index2);

EXPT void copyRNT_2arr(uint32_t* arr[][length_sizeof_uint16]);

EXPT void copyRNTHR2_2arr(uint32_t* arr[][length_sizeof_uint16]);

EXPT uint32_t hash_uint32(uint32_t num, int times = 3, uint32_t seed_4mul = mul_seeDefault);

EXPT void create_randNumTable(uint32_t seed_4mul = mul_seeDefault);

EXPT void create_randNumTableHR2(uint32_t seed_4mul = mul_seeDefault);

EXPT uint32_t get_randNum(uint16_t index, int timesD = 0);

EXPT uint32_t get_randNumHR2(uint16_t index, int timesD = 0);

EXPT void arr2Hash_uint32(const uint32_t* arrIn, uint32_t length, uint32_t* res_arr, int times = 3, uint32_t seed_4mul = mul_seeDefault);

#ifdef __cplusplus
}
#endif

#endif