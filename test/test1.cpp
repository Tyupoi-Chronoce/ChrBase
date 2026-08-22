#include <iostream>
#include <cstdint>



void test(uint8_t* arr, int times)
{
    for (int index =0; index < times; ++index)
    {
        std::cout << (uint16_t)arr[index] << std::endl;
    }
}
void test()
{
    uint8_t* arr;
    std::cout << (uint16_t)arr[256] << std::endl;
}

<<<<<<< HEAD
class Test
{
private:
    uint16_t test;

public:
    void static bimbimbambam()
    {}

};
=======
>>>>>>> af8bf87f2f8b5a5dab09168c3123711069c7115e


