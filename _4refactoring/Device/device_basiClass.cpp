#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <io.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <fluidsynth.h>
#include <stdexcept>
#include <SDL.h>
#include <CKhash.h>




namespace CK
{

    static const uint8_t info2devIObuffer_maxize = 16;
    static const uint16_t screen_maxWidth = 1280;
    static const uint16_t screen_minWidth = 640;
    static const uint16_t screen_maxHeight = 960;
    static const uint16_t screen_minHeight = 480;
    static const uint16_t CLImode_BytesMaxSize = 2048;


    class Screen
    {
    private:
        uint16_t Width;
        uint16_t Height;
        uint16_t CLImod_maxBytes;
        uint32_t* outBufferGUI;
        uint32_t* pageBufferGUI;
        uint32_t* outBufferCLI;
        uint32_t* pageBufferCLI;
        mutable bool isGUImod = false;
        mutable bool powerStatus = false;

    public:
        Screen(uint16_t width=screen_maxWidth, uint16_t height=screen_maxHeight, uint16_t CLImod_BytesMaxize=CLImode_BytesMaxSize)
        : Width(std::min(screen_maxWidth, std::max(width, screen_minWidth))), Height(std::min(screen_maxHeight, std::max(height, screen_minHeight))),
        outBufferGUI(new uint32_t[width * height]()), pageBufferGUI(new uint32_t[width * height]()), 
        CLImod_maxBytes(CLImod_BytesMaxize), outBufferCLI(new uint32_t[CLImod_BytesMaxize]()), pageBufferCLI(new uint32_t[CLImod_BytesMaxize]()) {}

        uint16_t get_Width() const {return Width; }
        uint16_t get_Height() const {return Height; }
        void _setGUImod() const {isGUImod = true; }
        void _setCLImod() const {isGUImod = false; }
        void power() const {powerStatus = !powerStatus; }
        bool get_powerState() const {return powerStatus; }

        ~Screen()
        {
            delete[] outBufferGUI;
            delete[] pageBufferGUI;
            delete[] outBufferCLI;
            delete[] pageBufferCLI;
        }

    };


    /*
    This base class provides a general IO interface for the Center class.
    Subclasses that publicly inherit from it will handle the incoming and saved information internally according to their own definitions
    */
    class devIO
   {
    private:
        uint32_t UEID;
        uint32_t UKUEUD;
        uint8_t* infoBuffer;
        uint8_t* outBuffer;
        uint8_t infoBufferHeadPointer = 0;
        uint8_t outBufferHeadPointer = 0;
        std::mutex infoMutexC;
        bool IOstate = false;
        bool selfRunningState = false;
        
        uint8_t IOphase = 0;

    public:
        std::mutex devMutexC;
        uint8_t* buffer;

        virtual void loop() = 0;

        devIO(uint32_t ID, uint32_t UKED)
        : UEID(ID), UKUEUD(UKED), buffer(new uint8_t[info2devIObuffer_maxize]()), infoBuffer(new uint8_t[info2devIObuffer_maxize]()),
            outBuffer(new uint8_t[info2devIObuffer_maxize]()) {}

        uint32_t get_UEID() const {return UEID; }
        uint32_t get_UKED() const {return UKUEUD; }

        bool get_selfRunningState() const {return selfRunningState; }
        void start() {selfRunningState = true; }
        void stop() {selfRunningState = false; }
        void startIO() {IOstate = true; }
        void stopIO() {IOstate = false; }

        bool _infoBHPcheck(uint8_t lengthNeed=1)
        {
            lengthNeed = std::min((uint8_t)(info2devIObuffer_maxize -1), lengthNeed);
            if (infoBufferHeadPointer >= info2devIObuffer_maxize) 
            {
                infoBufferHeadPointer = info2devIObuffer_maxize -1 - lengthNeed; 
                return true;
            }
            else if ((int)infoBufferHeadPointer - (int)lengthNeed < 0)
            {
                infoBufferHeadPointer = 0;
                return true;
            }
            return false;
        }
        bool _outBHPcheck(uint8_t lengthNeed=1)
        {
            lengthNeed = std::min((uint8_t)(info2devIObuffer_maxize -1), lengthNeed);
            if (outBufferHeadPointer >= info2devIObuffer_maxize) 
            {
                outBufferHeadPointer = info2devIObuffer_maxize -1 - lengthNeed; 
                return true;
            }
            else if ((int)outBufferHeadPointer - (int)lengthNeed < 0)
            {
                outBufferHeadPointer = 0;
                return true;
            }
            return false;
        }

        void info2dev(uint8_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _infoBHPcheck(1);
            infoBuffer[infoBufferHeadPointer] = data;
            infoBufferHeadPointer++;
        }
        void info2dev(uint16_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _infoBHPcheck(2);
            infoBuffer[infoBufferHeadPointer] = (uint8_t)data;
            infoBuffer[infoBufferHeadPointer +1] = (uint8_t)(data >> 8);
            infoBufferHeadPointer += 2;
        }
        void info2dev(uint32_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _infoBHPcheck(4);
            infoBuffer[infoBufferHeadPointer] = (uint8_t)data;
            infoBuffer[infoBufferHeadPointer +1] = (uint8_t)(data >> 8);
            infoBuffer[infoBufferHeadPointer +2] = (uint8_t)(data >> 16);
            infoBuffer[infoBufferHeadPointer +3] = (uint8_t)(data >> 24);
            infoBufferHeadPointer += 4;
        }
        uint8_t info2center8()
        {
            uint8_t res;
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(1);
            res = outBuffer[outBufferHeadPointer];
            outBufferHeadPointer--;
            return res;
        }
        uint16_t info2center16()
        {
            uint16_t res;
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(2);
            res = (uint16_t)outBuffer[outBufferHeadPointer] | (uint16_t)(outBuffer[outBufferHeadPointer +1] << 8);
            outBufferHeadPointer -= 2;
            return res;
        }
        uint32_t info2center32()
        {
            uint32_t res;
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(4);
            res = (uint32_t)outBuffer[outBufferHeadPointer] | (uint32_t)(outBuffer[outBufferHeadPointer +1] << 8) |
                (uint32_t)(outBuffer[outBufferHeadPointer +2] << 16) | (uint32_t)(outBuffer[outBufferHeadPointer +3] << 24);
            outBufferHeadPointer -= 4;
            return res;
        }
        void _savInfo() {memcpy(buffer, infoBuffer, info2devIObuffer_maxize); }
        void _infoBHPset0() {infoBufferHeadPointer = 0; }
        void _outBHPset0() {outBufferHeadPointer = 0; }
        void _push2outBuffer(uint8_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(1);
            outBuffer[outBufferHeadPointer] = data;
            outBufferHeadPointer++;
        }
        void _push2outBuffer(uint16_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(2);
            outBuffer[outBufferHeadPointer] = (uint8_t)data;
            outBuffer[outBufferHeadPointer +1] = (uint8_t)(data >> 8);
            outBufferHeadPointer += 2;
        }
        void _push2outBuffer(uint32_t data)
        {
            std::unique_lock<std::mutex> infoLock(infoMutexC);
            _outBHPcheck(4);
            outBuffer[outBufferHeadPointer] = (uint8_t)data;
            outBuffer[outBufferHeadPointer +1] = (uint8_t)(data >> 8);
            outBuffer[outBufferHeadPointer +2] = (uint8_t)(data >> 16);
            outBuffer[outBufferHeadPointer +3] = (uint8_t)(data >> 24);
            outBufferHeadPointer += 4;
        }
        void _phaseInc() {IOphase++; }
        void _phaseDec() {IOphase--; }
        void _phaseClr() {IOphase = 0; }
        uint8_t get_IOphase() const {return IOphase; }

        virtual ~devIO()
        {
            delete[] infoBuffer;
            delete[] outBuffer;
            delete[] buffer;
        }

    };


};






