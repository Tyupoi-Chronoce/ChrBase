#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <ctime>

int main() {
  time_t t = time(0);
  time_t t2 = time(0);
  std::cout << "time: " << t << std::endl; 
  std::cout << "time2: " << t2 << std::endl;
  uint32_t reg32 = 0x12345678;
  uint16_t* high = (uint16_t*)&reg32;
  uint8_t* test = (uint8_t*)&reg32;
  *high = 0xABCD;
  std::cout << "high: " << std::hex << *high << std::endl; 
  std::cout << reg32 << std::endl;  
  for (int i =0; i <= 60; ++i)
  {
    printf("test -- index<%d> - %x\n", i, test[i]);
  }
  time_t t3 = time(0);
  std::cout << "time3 hex:" << std::hex << t3 << std::endl;
  std::cout << "time3: " << std::dec << t3 << std::endl;
  std::cout << "reg32 address: " << std::hex << &reg32 << std::endl;
  system("pause");
  return 0;
}