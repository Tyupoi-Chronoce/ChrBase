#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>



//const uint32_t deadNum = 0xdeadc01d; // dead cold


class stack4B
{
private:
    uint32_t maxSize;
    uint32_t headIndex;
    uint8_t* stack;

public:
    stack4B(uint32_t stackSize_unitB)
     : maxSize(std::max(stackSize_unitB, 32u)), headIndex(maxSize), stack(new uint8_t[maxSize +1])
    {
        memset(stack, 0xcc, maxSize +1);
    }

    typedef struct
    {
        uint32_t data;
        bool isDead;
    } resData;

    resData push(uint32_t srcData)
    {
        resData res;
        res.isDead = (headIndex < 4);
        if (res.isDead) {return res; }
        headIndex -= 4;
        stack[headIndex] = srcData;
        stack[headIndex + 1] = srcData >> 8;
        stack[headIndex + 2] = srcData >> 16;
        stack[headIndex + 3] = srcData >> 24;
        return res;
    }
    resData pop()
    {
        resData res;
        res.isDead = (headIndex >= maxSize);
        if (res.isDead) {return res; }
        uint32_t tempNum = ((uint32_t)stack[headIndex]) | ((uint32_t)stack[headIndex + 1] << 8) 
            | ((uint32_t)stack[headIndex + 2] << 16) | ((uint32_t)stack[headIndex + 3] << 24);
        res.data = tempNum;
        headIndex += 4;
        return res;
    }
    resData checkHead() const
    {
        resData res;
        res.isDead = (headIndex >= maxSize);
        if (res.isDead) {return res; }
        uint32_t tempNum = ((uint32_t)stack[headIndex]) | ((uint32_t)stack[headIndex + 1] << 8) 
            | ((uint32_t)stack[headIndex + 2] << 16) | ((uint32_t)stack[headIndex + 3] << 24);
        res.data = tempNum;
        return res;
    }
    void clearStack() 
    {
        memset(stack, 0xcc, maxSize +1);
        headIndex = maxSize;
    }

    ~stack4B()
    {
        delete[] stack;
    }
    stack4B(const stack4B&) = delete;
    stack4B& operator = (const stack4B&) = delete;

};