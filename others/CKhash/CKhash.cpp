
#include <cstdio>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <algorithm>

// 静态库格式：原 DLL 版的 EXPT 展开为 `extern "C" __declspec(dllexport)`，
// 静态库不需要 dllexport，这里只保留 extern "C" 以维持 C 链接（符号不被 name-mangle），
// 使头文件 CKhash.h 在 C / C++ 下都能直接链接。算法、命名规范与 _4algor.cpp 完全一致。
#ifdef __cplusplus
#define EXPT extern "C"
#else
#define EXPT
#endif


static char test_state = 0;
const static int max4timesArg = 0xf;
const static int min4timesArg = 0x2;
const static uint32_t length_sizeof_uint16 = 0xffff +1;
static uint32_t randNum_arrToT[max4timesArg -1][length_sizeof_uint16] = {0};      // Total randnumbers table
static uint32_t randNum_arrHR2[max4timesArg -1][length_sizeof_uint16] = {0};      // Tavle of randnumbers from hash randNumTable

const static uint32_t xor_seed = 0xdeadc01d;        // Dead Cold
const static uint32_t mul_seeDefault = 0xfcfbfaf9;  // Odd num as first mul seed

const static uint32_t num1 = 0x9e3779b1;
const static uint32_t num2 = 0x85ebca6b;

EXPT void test()
{
    if (test_state ==0) {std::cout << "<<-Ready for Action->>" << std::endl; test_state = 1;}
    else {std::cout << ">>-Next time-<<";}
}

/*//
EXPT void creat(const char* arr, int length, const unsigned char* passwd, unsigned char* res_arr)
{
}//*/

EXPT uint32_t get_RandNumTable_value(uint32_t index1, uint32_t index2)
{
    return randNum_arrToT[index1][index2];
}

EXPT void copyRNT_2arr(uint32_t* arr[][length_sizeof_uint16])
{
    int times = min4timesArg;
    for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            *arr[index1][index2] = randNum_arrToT[index1][index2];
        }

    }
}

EXPT void copyRNTHR2_2arr(uint32_t* arr[][length_sizeof_uint16])
{
    int times = min4timesArg;
    for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            *arr[index1][index2] = randNum_arrHR2[index1][index2];
        }

    }
}

EXPT uint32_t hash_uint32(uint32_t num, int times=3, uint32_t seed_4mul=mul_seeDefault)
{
    uint32_t numT = seed_4mul | 0x01;
    times = std::min(0xf, std::max(2, times));
    num ^= xor_seed;
    num *= num1;
    for (int i =0; i < times; ++i)
    {
        num ^= (num >> 16) | (num2 << 4);
        num *= numT;
        numT ^= (((numT >> 9) | ((num ^ num1) >> 21)) & (numT << 12));
        numT |= 0x01;
    }
    if (num == 0 || num == 0xffffffff) {return hash_uint32(num ^ (seed_4mul & numT), times ^ 0b1011, seed_4mul);}
    return num;
}

EXPT void create_randNumTable(uint32_t seed_4mul=mul_seeDefault)
{
    int times = min4timesArg;
    for (uint32_t index1 =0; times <= max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            randNum_arrToT[index1][index2] = hash_uint32(index2, times, seed_4mul);
        }
    }
}

EXPT void create_randNumTableHR2(uint32_t seed_4mul=mul_seeDefault)
{
    create_randNumTable(seed_4mul);
    int times = min4timesArg;
    for (uint32_t index1 =0; times <= max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            uint32_t src_num = randNum_arrToT[index1][index2];
            randNum_arrHR2[index1][index2] = hash_uint32(src_num, times, seed_4mul);
        }
    }
}

EXPT uint32_t get_randNum(uint16_t index, int timesD=0)
{
    timesD = std::min(max4timesArg -2, std::max(0, timesD));
    uint16_t step = randNum_arrToT[timesD][(uint16_t)(index +1)];
    uint16_t indexT = index +1 + step;
    return randNum_arrToT[timesD][(uint16_t)indexT];
}

EXPT uint32_t get_randNumHR2(uint16_t index, int timesD=0)
{
    timesD = std::min(max4timesArg -2, std::max(0, timesD));
    //uint16_t step = randNum_arrHR2[timesD][(uint16_t)(index +1)];
    //uint16_t indexT = index +1 + step;
    return randNum_arrHR2[timesD][index];
}

EXPT void arr2Hash_uint32(const uint32_t* arrIn, uint32_t length, uint32_t* res_arr, int times=3, uint32_t seed_4mul=mul_seeDefault)
{                                                               // arrIn[length]    res_arr[length][length]
    for (uint32_t index1 = 0; index1 < length; ++index1)
    {
        uint32_t numT = hash_uint32(index1, 0xf);
        for (uint32_t index2 = 0; index2 < length; ++index2)
        {
            res_arr[index1 * length + index2] = hash_uint32(arrIn[index2 & numT], times, seed_4mul);
        }
    }
}
