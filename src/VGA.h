#ifndef _VGA_H
#define _VGA_H

#include <Cariad.h>

#ifndef VGA_USE_DOUBLE_BUFFER
#define VGA_USE_DOUBLE_BUFFER 0
#endif

#ifndef VGA_USE_H_RES
#define VGA_USE_H_RES 0
#endif

#ifndef VGA_USE_V_RES
#define VGA_USE_V_RES 0
#endif

#ifdef __PIC32MZ__
#define _TIMER_5_IRQ _TIMER_5_VECTOR
#define _SPI4_TX_IRQ _SPI4_TX_VECTOR
#define _CORE_TIMER_IRQ _CORE_TIMER_VECTOR
#endif

class VGA : public Cariad {
    public:
        static const int Width = 320;
        static const int Height = 240;

        static VGA *_unit;


        // VGA timings specify:
        static const uint32_t vgaHFP = 16/2;
        static const uint32_t vgaHSP = 96/2;
        static const uint32_t vgaHBP = 48/2;
        static const uint32_t vgaHDP = 640/2;

        static const uint32_t vgaHTOT = vgaHFP + vgaHSP + vgaHBP + vgaHDP;

        static const uint32_t vgaHoriz[];

        // These are all in lines.
        static const uint32_t vgaVFP = 10;
        static const uint32_t vgaVSP = 2;
        static const uint32_t vgaVBP = 33;
        static const uint32_t vgaVDP = 480;
        static const uint32_t vgaVTOT = vgaVFP + vgaVSP + vgaVBP + vgaVDP;

    private:
        uint32_t _scanPhase;
        uint32_t _scanLine;
        uint32_t _ramPos;


        static const uint32_t vgaVert[];

        volatile uint8_t _buffer0[(vgaHTOT/8) * (vgaVTOT >> 1)] __attribute__((aligned(4)));

        volatile uint8_t *activeBuffer;

        uint32_t bufferNumber;

        p32_ioport *_hsync_port;
        p32_ioport *_vsync_port;
        uint32_t _hsync_pin;
        uint32_t _vsync_pin;

    
    public:
        VGA(uint8_t hsync, uint8_t vsync);
        void initializeDevice();
        void setPixel(int x, int y, color_t c);
        void setRotation(int __attribute__((unused)) r) {}
        void displayOn() {}
        void displayOff() {}
        void invertDisplay(bool __attribute__((unused)) i) {}

        int getWidth() { return Width; }
        int getHeight() { return Height; }

        void vblank();
        void flip();

        void fillScreen(color_t c);

        uint32_t millis();

        static void processInterrupt() { if (_unit) _unit->runInterrupt(); }
        void runInterrupt();

};

#endif
