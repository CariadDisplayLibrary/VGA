#include <VGA.h>


#define VSYNC_OFF vsync_port->lat.set = vsync_pin;
#define VSYNC_ON  vsync_port->lat.clr = vsync_pin;

#define HSYNC_OFF hsync_port->lat.set = hsync_pin;
#define HSYNC_ON  hsync_port->lat.clr = hsync_pin;

VGA *VGA::_unit;

p32_ioport *hsync_port;
p32_ioport *vsync_port;
uint32_t hsync_pin;
uint32_t vsync_pin;

static const uint8_t blank[100] = {0}; // A blank line

static volatile uint8_t *vgaBuffer;
static volatile uint8_t pulsePhase = 0;

static volatile uint32_t scanLine = 0;

#define PULSEPOS 2340
#define PULSEWIDTH 10

void __USER_ISR horizPulse() {
    IFS0bits.T5IF = 0;
    HSYNC_ON
    T5CONbits.ON = 0;
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    HSYNC_OFF;
}

void __USER_ISR vgaProcess() {


    static uint32_t _ramPos = 0;

    DCH0INTbits.CHSDIF = 0;
    IFS1bits.DMA0IF = 0;

    pulsePhase = 0;
    TMR5 = 0;
    PR5 = PULSEPOS;
    T5CONbits.ON=1;


    if (scanLine == 0) _ramPos = 0;
    if (scanLine < 480) {
        DCH0SSA = ((uint32_t)&vgaBuffer[_ramPos]) & 0x1FFFFFFF;
    } else {
        DCH0SSA = ((uint32_t)&blank[0]) & 0x1FFFFFFF;
    }

    DCH0SSIZ = VGA::vgaHTOT >> 3;
    DCH0ECONbits.CFORCE = 1;
    DCH0CONbits.CHEN = 1;

    if (scanLine & 1) _ramPos += VGA::vgaHTOT >> 3;
    scanLine++;

    if (scanLine == 490) {
        VSYNC_ON
    } else if (scanLine == 492) {
        VSYNC_OFF
    } else if (scanLine == 525) {
        scanLine = 0;
    }
}

uint32_t VGA::millis() {
    return millis();
}

void VGA::initializeDevice() {
    if (_hsync_pin == 0 || _vsync_pin == 0) {
        return;
    }

    memset((void *)_buffer0, 0, (vgaHTOT >> 3) * (vgaVTOT >> 1));
    vgaBuffer = _buffer0;

    _hsync_port->tris.clr = _hsync_pin;
    _vsync_port->tris.clr = _vsync_pin;

    hsync_port = _hsync_port;
    vsync_port = _vsync_port;
    hsync_pin = _hsync_pin;
    vsync_pin = _vsync_pin;

    // Now congfigure SPI4 for display data transmission.
    SPI4CON = 0;
    SPI4CONbits.MSTEN = 1;
    SPI4CONbits.STXISEL = 0; //0b11;
    SPI4BRG = 2;
    SPI4CONbits.ON = 1;

    // And now a DMA channel for transferring the data
    DCH0DSA = ((unsigned int)&SPI4BUF) & 0x1FFFFFFF;
    DCH0DSIZ = 1;
    DCH0CSIZ = 1;
    DCH0ECONbits.SIRQEN = 1;
    DCH0ECONbits.CHSIRQ = _SPI4_TX_IRQ;
    DCH0CONbits.CHAEN = 0;
    DCH0CONbits.CHEN = 0;
    DCH0CONbits.CHPRI = 3;
    DCH0INTbits.CHSDIF = 0;
    DCH0INTbits.CHSDIE = 1;

    setIntVector(_DMA_0_VECTOR, vgaProcess);
    setIntPriority(_DMA_0_VECTOR, 6, 0);
    clearIntFlag(_DMA0_IRQ);
    setIntEnable(_DMA0_IRQ);

    VSYNC_OFF
    HSYNC_OFF

    DMACONbits.ON = 1;

    DCH0SSA = ((uint32_t)&blank[0]) & 0x1FFFFFFF;
    DCH0SSIZ = vgaHBP;
    DCH0ECONbits.CFORCE = 1;
    DCH0CONbits.CHEN = 1;

    T5CON = 0;
    T5CONbits.TCKPS = 0;
    PR5=0xFFFF;
    setIntVector(_TIMER_5_VECTOR, horizPulse);
    setIntPriority(_TIMER_5_VECTOR, 6, 0);
    clearIntFlag(_TIMER_5_IRQ);
    setIntEnable(_TIMER_5_IRQ);

    // Unfortunately the core timer really plays havoc with the VGA timing.
    // So we have to disable it. That means no millis() and no delay().
    // Makes things a little more interesting...
    clearIntEnable(_CORE_TIMER_IRQ);
}

VGA::VGA(uint8_t hsync, uint8_t vsync) {
    _unit = this;
    uint32_t port = 0;

    _scanPhase = 0;
    scanLine = 0;
    _ramPos = 0;

    _hsync_pin = 0;
    _vsync_pin = 0;

    if (hsync >= NUM_DIGITAL_PINS_EXTENDED) { return; }
    if (vsync >= NUM_DIGITAL_PINS_EXTENDED) { return; }

    port = digitalPinToPort(hsync);
    if (port == NOT_A_PIN) { return; }
    _hsync_port = (p32_ioport *)portRegisters(port);
    _hsync_pin = digitalPinToBitMask(hsync);

    port = digitalPinToPort(vsync);
    if (port == NOT_A_PIN) { return; }
    _vsync_port = (p32_ioport *)portRegisters(port);
    _vsync_pin = digitalPinToBitMask(vsync);
}

void VGA::setPixel(int x, int y, color_t c) {
    if (x < 0 || y < 0 || x >= Width || y >= Height) {
        return;
    }
    uint32_t poff = x/8 + y * (vgaHTOT/8);
    uint8_t ppos = x % 8;
    poff += 1;
    if (c == Color::Black) {
        _buffer0[poff] &= ~(0x80>>ppos);
    } else {
        _buffer0[poff] |= (0x80>>ppos);
    }
}

void VGA::vblank() {
    while (scanLine != 480);
}

void VGA::flip() {
}

void VGA::fillScreen(color_t c) {
    if (c) {
        for (int i = 0; i < (vgaVDP >> 1); i++) {
            memset((void *)&_buffer0[(vgaHTOT >> 3) * i + 1], 255, vgaHDP>>3);
        }
    } else {
        memset((void *)_buffer0, 0, (vgaHTOT >> 3) * (vgaVTOT >> 1));
    }
}

