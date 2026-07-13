#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdlib>

int main() {
  uint32_t reg32 = 0x12345678;
  uint16_t* high = (uint16_t*)&reg32;
  *high = 0xABCD;
  std::cout << "high: " << std::hex << high[0] << std::endl; 
  std::cout << reg32 << std::endl;  
  system("pause");
  return 0;
}