#include "../__4algor.h"
#include <cstdio>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <windows.h>

static char test_state = 0;
static uint32_t randNum_arrToT[max4timesArg -1][length_sizeof_uint16] = {0};
static uint32_t randNum_arrHR2[max4timesArg -1][length_sizeof_uint16] = {0};

EXPT void test() 
{
    if (test_state ==0) {std::cout << "<<-Ready for Action->>" << std::endl; test_state = 1;}
    else {std::cout << ">>-Next time-<<";}
}

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

EXPT uint32_t hash_uint32(uint32_t num, int times, uint32_t seed_4mul)
{
    uint32_t numT = seed_4mul | 0x01;
    times = std::min(0xf, std::max(2, times));
    num ^= xor_seed;
    num *= num1;
    for (int i =0; i < times; ++i)
    {
        num ^= (num >> 16);
        num *= numT;
        numT ^= ((((numT >> 8) | ((num ^ num1) >> 24)) & (numT << 16)) | 0x01);
    }
    if (num == 0 || num == 0xffffffff) {return hash_uint32(num ^ (seed_4mul & numT), times ^ 0x0f, seed_4mul);}
    return num;
}

EXPT void create_randNumTable(uint32_t seed_4mul)
{
    int times = min4timesArg;
    for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            randNum_arrToT[index1][index2] = hash_uint32(index2, times, seed_4mul);
        }
    }
}

EXPT void create_randNumTableHR2(uint32_t seed_4mul)
{
    create_randNumTable(seed_4mul);
    int times = min4timesArg;
    for (uint32_t index1 =0; times < max4timesArg; (++times, ++index1))
    {
        for (uint32_t index2 =0; index2 < length_sizeof_uint16; ++index2)
        {
            uint32_t src_num = randNum_arrToT[index1][index2];
            randNum_arrHR2[index1][index2] = hash_uint32(src_num, times, seed_4mul);
        }
    }
}

EXPT uint32_t get_randNum(uint16_t index, int timesD)
{
    timesD = std::min(max4timesArg -2, std::max(0, timesD));
    uint16_t step = randNum_arrToT[timesD][(uint16_t)(index +1)];
    uint16_t indexT = index +1 + step;
    return randNum_arrToT[timesD][(uint16_t)indexT];
}

EXPT uint32_t get_randNumHR2(uint16_t index, int timesD)
{
    timesD = std::min(max4timesArg -2, std::max(0, timesD));
    uint16_t step = randNum_arrHR2[timesD][(uint16_t)(index +1)];
    uint16_t indexT = index +1 + step;
    return randNum_arrHR2[timesD][(uint16_t)indexT];
}

EXPT void arr2Hash_uint32(const uint32_t* arrIn, uint32_t length, uint32_t* res_arr, int times, uint32_t seed_4mul)
{
    for (uint32_t index1 = 0; index1 < length; ++index1)
    {
        uint32_t numT = hash_uint32(index1, 0xf);
        for (uint32_t index2 = 0; index2 < length; ++index2)
        {
            res_arr[index1 * length + index2] = hash_uint32(arrIn[index2 & numT], times, seed_4mul);
        }
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    test_state = 0;
    test();
    return TRUE;
}
