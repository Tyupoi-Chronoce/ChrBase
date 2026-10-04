#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>



const uint32_t deadNum = 0xdeadc01d; // dead cold



typedef struct LLnode
{
    uint32_t data = deadNum;
    LLnode* head = nullptr;
    LLnode* tail = nullptr;

};

typedef struct
{
    LLnode* topNode = nullptr;

} lnkStack;

void push(lnkStack* stack, LLnode* node)
{
    if (stack->topNode == nullptr) {stack->topNode = node; }

    else 
    {
        stack->topNode->tail = node;
        node->head = stack->topNode;
        stack->topNode = node;
    }
}
void push(lnkStack* stack, uint32_t data)
{
    LLnode* node = new LLnode;
    node->data = data;
    push(stack, node);

}
uint32_t pop(lnkStack* stack)
{
    if (stack->topNode == nullptr) {return deadNum; }
    uint32_t res = stack->topNode->data;
    LLnode* tempNode = stack->topNode;
    stack->topNode = stack->topNode->head;
    if (stack->topNode != nullptr) {stack->topNode->tail = nullptr; }
    delete tempNode;
    return res;
}


