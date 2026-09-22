#define ECS_SPECIFIC
#include <hardware/dmabits.h>
#include <hardware/intbits.h>
#include <hardware/cia.h>
#include <hardware/blit.h>
#include <hardware/custom.h>
#include <graphics/display.h>
#include "AmigaHardware.h"
#include "CopperList.h"
#include "Sprite.h"
#include "Palette.h"
#include "Palette24Bit.h"

bool AmigaHardware::hasAGAChipSet = false;
bool AmigaHardware::hasQueuedBlits = false;
uint16_t AmigaHardware::bplcon3BaseValue = 0x0c00 | BPLCON3_BRDNBLNK | BPLCON3_BRDNTRAN;

uint16_t AmigaHardware::octants[4] = {
    OCTANT2 | LINEMODE,
    OCTANT1 | LINEMODE,
    OCTANT3 | LINEMODE,
    OCTANT4 | LINEMODE
};

uint16_t* AmigaHardware::blitterQueueBuffer = new uint16_t[BLITTER_QUEUE_SIZE + 1];
uint16_t* AmigaHardware::blitterQueueBufferEnd = blitterQueueBuffer + BLITTER_QUEUE_SIZE;
uint16_t* AmigaHardware::blitterQueueToBeBlitted = blitterQueueBuffer;
uint16_t* AmigaHardware::blitterQueueAddPosition = blitterQueueBuffer;

void AmigaHardware::setCopperList(const CopperList& copperList, bool immediate)
{
    *cop1lcPointer = copperList.data();
    if (immediate) {
        *copjmp1Pointer = 0;
    }
}

void AmigaHardware::setPalette(const Palette& palette, uint16_t colorIndex, int16_t fromIndex, int16_t toIndex)
{
    if (toIndex < 0) {
        toIndex = (int16_t)(palette.colorCount() - 1);
    }

    *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 7));
    uint16_t* colorRegister = (uint16_t*)color00Pointer + colorIndex;
    for (uint32_t i = fromIndex, j = colorIndex; i <= toIndex; i++) {
        *colorRegister++ = palette[i];

        if ((++j) & 31 == 0) {
            *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((j & 0x00e0) << 7));
            colorRegister = (uint16_t*)color00Pointer;
        }
    }
}

void AmigaHardware::setPalette24Bit(const Palette24Bit& palette, uint16_t colorIndex, int16_t fromIndex, int16_t toIndex)
{
    uint32_t i, j;
    if (toIndex < 0) {
        toIndex = (int16_t)(palette.colorCount() - 1);
    }

    *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 8));
    uint16_t* colorRegister = (uint16_t*)color00Pointer + (colorIndex & 31);
    for (i = fromIndex, j = colorIndex; i <= toIndex; i++) {
        uint32_t color = palette[i];
        *colorRegister++ = (uint16_t)(((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4));

        if (((++j) & 31) == 0) {
            *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((j & 0x00e0) << 8));
            colorRegister = (uint16_t*)color00Pointer;
        }
    }

    *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 8) | 0x0200);
    colorRegister = (uint16_t*)color00Pointer + (colorIndex & 31);
    for (i = fromIndex, j = colorIndex; i <= toIndex; i++) {
        uint32_t color = palette[i];
        *colorRegister++ = (uint16_t)(((color & 0x0f0000) >> 8) | ((color & 0x000f00) >> 4) | (color & 0x00000f));

        if (((++j) & 31) == 0) {
            *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((j & 0x00e0) << 8) | 0x200);
            colorRegister = (uint16_t*)color00Pointer;
        }
    }
}

void AmigaHardware::setColor(uint16_t colorIndex, uint16_t color, bool changeBank, bool lowBits)
{
    if (changeBank) {
        *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 7) | (lowBits ? 0x0200 : 0));
    }
    uint16_t* colorRegister = (uint16_t*)color00Pointer + colorIndex;
    *colorRegister = color;
}

void AmigaHardware::setColor24Bit(uint16_t colorIndex, uint32_t color)
{
    uint16_t* colorRegister = (uint16_t*)color00Pointer + colorIndex;
    *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 7));
    *colorRegister = (uint16_t)(((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4));
    *bplcon3Pointer = (uint16_t)(bplcon3BaseValue | ((colorIndex & 0x00e0) << 7) | 0x200);
    *colorRegister = (uint16_t)(((color & 0x0f0000) >> 8) | ((color & 0x000f00) >> 4) | (color & 0x00000f));
}

void AmigaHardware::setPlayfield(uint16_t width, uint16_t height, uint8_t bitplaneCount, bool interleaved, bool hires, bool interlace, bool dualPlayfield, bool holdAndModify, bool killEHB, uint16_t centerY)
{
    uint16_t halfHeight = height >> 1;
    uint16_t bitplaneWidth = width >> 3;
    *fmodePointer = (uint16_t)(hasAGAChipSet ? 3 : 0);
    *bplcon3Pointer = bplcon3BaseValue;
    *bplcon2Pointer = (uint16_t)(killEHB ? 0x0224 : 0x0024);
    *bplcon1Pointer = 0;
    *bplcon0Pointer = (uint16_t)(((bitplaneCount & 8) << 1) | ((bitplaneCount & 7) << PLNCNTSHFT) | (hires ? MODE_640 : 0) | (dualPlayfield ? DBLPF : 0) | (holdAndModify ? HOLDNMODIFY : 0) | USE_BPLCON3);
    *diwstrtPointer = (uint16_t)(((centerY - halfHeight) << 8) | 0x81);
    *diwstopPointer = (uint16_t)(((centerY + halfHeight) << 8) | 0xc1);
    *diwhighPointer = 0x2100;
    if (hasAGAChipSet) {
        if (hires) {
            *ddfstrtPointer = (uint16_t)(0x88 - bitplaneWidth);
            *ddfstopPointer = (uint16_t)(0x94 + (bitplaneWidth >> 1));
        } else {
            *ddfstrtPointer = (uint16_t)(0x88 - (bitplaneWidth << 1));
            *ddfstopPointer = (uint16_t)(0x90 + bitplaneWidth);
        }
    } else {
        if (hires) {
            *ddfstrtPointer = (uint16_t)(0x88 - bitplaneWidth);
            *ddfstopPointer = (uint16_t)(0x80 + bitplaneWidth);
        } else {
            *ddfstrtPointer = (uint16_t)(0x88 - (bitplaneWidth << 1));
            *ddfstopPointer = (uint16_t)(0x80 + (bitplaneWidth << 1));
        }
    }
    *bpl1modPointer = (uint16_t)(interleaved ? ((bitplaneCount - 1) * bitplaneWidth) : 0);
    *bpl2modPointer = (uint16_t)(interleaved ? ((bitplaneCount - 1) * bitplaneWidth) : 0);
}

void AmigaHardware::setSpritesEnabled(bool enabled)
{
    if (enabled) {
        *dmaconPointer = DMAF_SETCLR | DMAF_SPRITE;
    } else {
        *dmaconPointer = DMAF_SPRITE;
    }
}

void AmigaHardware::setBlitterNasty(bool enabled)
{
    if (enabled) {
        *dmaconPointer = DMAF_SETCLR | DMAF_BLITHOG;
    } else {
        *dmaconPointer = DMAF_BLITHOG;
    }
}

void AmigaHardware::setDMAChannels(uint16_t dmaChannels, bool enabled)
{
    if (enabled) {
        *dmaconPointer = (uint16_t)(dmaChannels | DMAF_SETCLR);
    } else {
        *dmaconPointer = dmaChannels;
    }
}

void AmigaHardware::setInterrupts(uint16_t interrupts, bool enabled)
{
    if (enabled) {
        *intenaPointer = (uint16_t)(interrupts | INTF_SETCLR);
    } else {
        *intenaPointer = interrupts;
    }
}

uint16_t AmigaHardware::enabledDMAChannels()
{
    return *dmaconrPointer;
}

uint16_t AmigaHardware::enabledInterrupts()
{
    return *intenarPointer;
}

void AmigaHardware::clearInterruptRequests(uint16_t interrupts)
{
    *intreqPointer = interrupts;
    *intreqPointer = interrupts;
}

bool AmigaHardware::isLeftMouseButtonPressed()
{
    return !(*ciaapraPointer & CIAF_GAMEPORT0);
}

bool AmigaHardware::isRightMouseButtonPressed()
{
    return !(*potinpPointer & (1 << 10));
}

#ifdef USE_BLITTER_QUEUE
#ifndef ASSEMBLER
void AmigaHardware::blitterClear(uint16_t* data, uint16_t width, uint16_t height, int16_t modulo)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltawmPointer = 0xffffffff;
        *bltcon1Pointer = 0;
        *bltcon0Pointer = BC0F_DEST;
        *bltdmodPointer = modulo;
        *bltdptPointer = data;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = 8;
        if (blitterQueueAddPosition + 16 >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = 0;
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = BC0F_DEST;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = modulo;
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(data) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)data);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::blitterCopy(uint16_t* source, uint16_t* destination, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t shift, uint16_t firstWordMask, uint16_t lastWordMask, uint16_t mask)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltafwmPointer = firstWordMask;
        *bltalwmPointer = lastWordMask;
        *bltcon1Pointer = (uint16_t)(shift < 0 ? BLITREVERSE : 0);
        *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_DEST | ABC | ABNC | ((shift < 0 ? -shift : shift) << 12));
        *bltdmodPointer = destinationModulo;
        if (source) {
            *bltamodPointer = sourceModulo;
            *bltaptPointer = source;
        } else {
            *bltadatPointer = 0;
        }
        *bltbdatPointer = mask;
        *bltdptPointer = destination;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = (uint16_t)(source ? 12 : 10);
        if (blitterQueueAddPosition + (source ? 24 : 20) >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = firstWordMask;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = lastWordMask;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = (uint16_t)(shift < 0 ? BLITREVERSE : 0);
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_DEST | ABC | ABNC | ((shift < 0 ? -shift : shift) << 12));
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = destinationModulo;
        if (source) {
            *blitterQueueAddPosition++ = bltamod;
            *blitterQueueAddPosition++ = sourceModulo;
            *blitterQueueAddPosition++ = bltapth;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(source) >> 16);
            *blitterQueueAddPosition++ = bltaptl;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)source);
        } else {
            *blitterQueueAddPosition++ = bltadat;
            *blitterQueueAddPosition++ = 0;
        }
        *blitterQueueAddPosition++ = bltbdat;
        *blitterQueueAddPosition++ = mask;
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::blitterCopyWithMask(uint16_t* source, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltafwmPointer = firstWordMask;
        *bltalwmPointer = lastWordMask;
        *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *bltbmodPointer = maskModulo;
        *bltcmodPointer = destinationModulo;
        *bltdmodPointer = destinationModulo;
        if (source) {
            *bltamodPointer = sourceModulo;
            *bltaptPointer = source;
        } else {
            *bltadatPointer = 0;
        }
        *bltbptPointer = mask;
        *bltcptPointer = destination;
        *bltdptPointer = destination;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = (uint16_t)(source ? 17 : 15);
        if (blitterQueueAddPosition + (source ? 34 : 30) >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = firstWordMask;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = lastWordMask;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *blitterQueueAddPosition++ = bltbmod;
        *blitterQueueAddPosition++ = maskModulo;
        *blitterQueueAddPosition++ = bltcmod;
        *blitterQueueAddPosition++ = destinationModulo;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = destinationModulo;
        if (source) {
            *blitterQueueAddPosition++ = bltamod;
            *blitterQueueAddPosition++ = sourceModulo;
            *blitterQueueAddPosition++ = bltapth;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(source) >> 16);
            *blitterQueueAddPosition++ = bltaptl;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)source);
        } else {
            *blitterQueueAddPosition++ = bltadat;
            *blitterQueueAddPosition++ = 0;
        }
        *blitterQueueAddPosition++ = bltbpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(mask) >> 16);
        *blitterQueueAddPosition++ = bltbptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)mask);
        *blitterQueueAddPosition++ = bltcpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltcptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::blitterLine(uint16_t* data, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t bytesPerRow, bool singleBitPerRow)
{
    if (singleBitPerRow && y1 == y2) {
        return;
    }

    // Make sure the line is drawn from top to bottom
    if (y2 < y1) {
        uint16_t temp = x2;
        x2 = x1;
        x1 = temp;

        temp = y2;
        y2 = y1;
        y1 = temp;
    }

    uint16_t octant = 0;
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        return;
    }
    if (dx < 0) {
        octant += 2;
        dx = (int16_t)-dx;
    }
    if (dx >= (dy + dy)) {
        dy--;
    }

    uint16_t* firstWord = data + y1 * (bytesPerRow >> 1) + (x1 >> 4);
    if (dy < dx) {
        int16_t signedTemp = dy;
        dy = dx;
        dx = signedTemp;
        octant++;
    }
    uint16_t bltcon0Value = ((x1 & 15) << 12) | BC0F_SRCA | BC0F_SRCC | BC0F_DEST;
    uint16_t bltcon1Value = octants[octant];
    if (singleBitPerRow) {
        bltcon0Value |= A_XOR_C;
        bltcon1Value |= ONEDOT;
    } else {
        bltcon0Value |= A_OR_C;
    }
    int16_t v = dx + dx;

    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltawmPointer = 0xffffffff;
        *bltadatPointer = 0x8000;
        *bltbdatPointer = 0xffff;
        *bltcmodPointer = (int16_t)bytesPerRow;
        *bltdmodPointer = (int16_t)bytesPerRow;
        *bltbmodPointer = v;
        v -= dy;
        if (v < 0) {
            bltcon1Value |= SIGNFLAG;
        }
        *bltaptlPointer = v;
        v -= dy;
        *bltamodPointer = v;
        *bltcon0Pointer = bltcon0Value;
        *bltcon1Pointer = bltcon1Value;
        *bltcptPointer = firstWord;
        *bltdptPointer = firstWord;
        *bltsizePointer = (uint16_t)((dy << 6) | 2);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = 16;
        if (blitterQueueAddPosition + 32 >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltadat;
        *blitterQueueAddPosition++ = 0x8000;
        *blitterQueueAddPosition++ = bltbdat;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltcmod;
        *blitterQueueAddPosition++ = (int16_t)bytesPerRow;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = (int16_t)bytesPerRow;
        *blitterQueueAddPosition++ = bltbmod;
        *blitterQueueAddPosition++ = v;
        v -= dy;
        if (v < 0) {
            bltcon1Value |= SIGNFLAG;
        }
        *blitterQueueAddPosition++ = bltaptl;
        *blitterQueueAddPosition++ = v;
        v -= dy;
        *blitterQueueAddPosition++ = bltamod;
        *blitterQueueAddPosition++ = v;
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = bltcon0Value;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = bltcon1Value;
        *blitterQueueAddPosition++ = bltcpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(firstWord) >> 16);
        *blitterQueueAddPosition++ = bltcptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)firstWord);
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(firstWord) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)firstWord);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((dy << 6) | 2);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::blitterFill(uint16_t* data, uint16_t width, uint16_t height, int16_t modulo)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltawmPointer = 0xffffffff;
        *bltcon0Pointer = BC0F_SRCA | BC0F_DEST | A_TO_D;
        *bltcon1Pointer = BLITREVERSE | FILL_OR;
        *bltamodPointer = modulo;
        *bltdmodPointer = modulo;
        *bltaptPointer = data;
        *bltdptPointer = data;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = 11;
        if (blitterQueueAddPosition + 22 >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = 0xffff;
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = BC0F_SRCA | BC0F_DEST | A_TO_D;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = BLITREVERSE | FILL_OR;
        *blitterQueueAddPosition++ = bltamod;
        *blitterQueueAddPosition++ = modulo;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = modulo;
        *blitterQueueAddPosition++ = bltapth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(data) >> 16);
        *blitterQueueAddPosition++ = bltaptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)data);
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(data) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)data);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::processBlitterQueue()
{
    if (isBlitterBusy() || !hasQueuedBlits) {
        return;
    }

    uint16_t registerCount = *blitterQueueToBeBlitted++;
    if (blitterQueueToBeBlitted + registerCount + registerCount >= blitterQueueBufferEnd) {
        blitterQueueToBeBlitted = blitterQueueBuffer;
    }
    for (uint16_t i = 0; i < registerCount; i++) {
        uint32_t destination = 0xdff000 + *blitterQueueToBeBlitted++;
        *((uint16_t*)destination) = *blitterQueueToBeBlitted++;
    }
    hasQueuedBlits = (bool)(blitterQueueToBeBlitted != blitterQueueAddPosition ? true : false);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}
#endif

void AmigaHardware::blitterCombineWithMask(uint16_t* background, uint16_t* source, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t backgroundModulo, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltafwmPointer = firstWordMask;
        *bltalwmPointer = lastWordMask;
        *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *bltbmodPointer = maskModulo;
        *bltcmodPointer = backgroundModulo;
        *bltdmodPointer = destinationModulo;
        if (source) {
            *bltamodPointer = sourceModulo;
            *bltaptPointer = source;
        } else {
            *bltadatPointer = 0;
        }
        *bltbptPointer = mask;
        *bltcptPointer = background;
        *bltdptPointer = destination;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = (uint16_t)(source ? 17 : 15);
        if (blitterQueueAddPosition + (source ? 34 : 30) >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = firstWordMask;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = lastWordMask;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *blitterQueueAddPosition++ = bltbmod;
        *blitterQueueAddPosition++ = maskModulo;
        *blitterQueueAddPosition++ = bltcmod;
        *blitterQueueAddPosition++ = backgroundModulo;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = destinationModulo;
        if (source) {
            *blitterQueueAddPosition++ = bltamod;
            *blitterQueueAddPosition++ = sourceModulo;
            *blitterQueueAddPosition++ = bltapth;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(source) >> 16);
            *blitterQueueAddPosition++ = bltaptl;
            *blitterQueueAddPosition++ = (uint16_t)((uint32_t)source);
        } else {
            *blitterQueueAddPosition++ = bltadat;
            *blitterQueueAddPosition++ = 0;
        }
        *blitterQueueAddPosition++ = bltbpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(mask) >> 16);
        *blitterQueueAddPosition++ = bltbptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)mask);
        *blitterQueueAddPosition++ = bltcpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(background) >> 16);
        *blitterQueueAddPosition++ = bltcptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)background);
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

void AmigaHardware::blitterPatternWithMask(uint16_t pattern, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    AmigaHardware::setInterrupts(INTF_BLIT, false);
    if (!isBlitterBusy() && !hasQueuedBlits) {
        *bltafwmPointer = firstWordMask;
        *bltalwmPointer = lastWordMask;
        *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *bltcon0Pointer = (uint16_t)(BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *bltbmodPointer = maskModulo;
        *bltcmodPointer = destinationModulo;
        *bltdmodPointer = destinationModulo;
        *bltadatPointer = pattern;
        *bltbptPointer = mask;
        *bltcptPointer = destination;
        *bltdptPointer = destination;
        *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
        *color00Pointer = 0xf00;
#endif
    } else {
        *blitterQueueAddPosition++ = 15;
        if (blitterQueueAddPosition + 30 >= blitterQueueBufferEnd) {
            blitterQueueAddPosition = blitterQueueBuffer;
        }
        *blitterQueueAddPosition++ = bltafwm;
        *blitterQueueAddPosition++ = firstWordMask;
        *blitterQueueAddPosition++ = bltalwm;
        *blitterQueueAddPosition++ = lastWordMask;
        *blitterQueueAddPosition++ = bltcon1;
        *blitterQueueAddPosition++ = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
        *blitterQueueAddPosition++ = bltcon0;
        *blitterQueueAddPosition++ = (uint16_t)(BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
        *blitterQueueAddPosition++ = bltbmod;
        *blitterQueueAddPosition++ = maskModulo;
        *blitterQueueAddPosition++ = bltcmod;
        *blitterQueueAddPosition++ = destinationModulo;
        *blitterQueueAddPosition++ = bltdmod;
        *blitterQueueAddPosition++ = destinationModulo;
        *blitterQueueAddPosition++ = bltadat;
        *blitterQueueAddPosition++ = pattern;
        *blitterQueueAddPosition++ = bltbpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(mask) >> 16);
        *blitterQueueAddPosition++ = bltbptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)mask);
        *blitterQueueAddPosition++ = bltcpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltcptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltdpth;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)(destination) >> 16);
        *blitterQueueAddPosition++ = bltdptl;
        *blitterQueueAddPosition++ = (uint16_t)((uint32_t)destination);
        *blitterQueueAddPosition++ = bltsize;
        *blitterQueueAddPosition++ = (uint16_t)((height << 6) | width);
        hasQueuedBlits = true;
        processBlitterQueue();
    }
    AmigaHardware::setInterrupts(INTF_BLIT, true);
}

#else

void AmigaHardware::blitterClear(uint16_t* data, uint16_t width, uint16_t height, int16_t modulo)
{
    blitterWait();
    *bltawmPointer = 0xffffffff;
    *bltcon1Pointer = 0;
    *bltcon0Pointer = BC0F_DEST;
    *bltdmodPointer = modulo;
    *bltdptPointer = data;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterCopy(uint16_t* source, uint16_t* destination, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t shift, uint16_t firstWordMask, uint16_t lastWordMask, uint16_t mask)
{
    blitterWait();
    *bltafwmPointer = firstWordMask;
    *bltalwmPointer = lastWordMask;
    *bltcon1Pointer = (uint16_t)(shift < 0 ? BLITREVERSE : 0);
    *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_DEST | ABC | ABNC | ((shift < 0 ? -shift : shift) << 12));
    *bltdmodPointer = destinationModulo;
    if (source) {
        *bltamodPointer = sourceModulo;
        *bltaptPointer = source;
    } else {
        *bltadatPointer = 0;
    }
    *bltbdatPointer = mask;
    *bltdptPointer = destination;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterCopyWithMask(uint16_t* source, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    blitterWait();
    *bltafwmPointer = firstWordMask;
    *bltalwmPointer = lastWordMask;
    *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
    *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
    *bltbmodPointer = maskModulo;
    *bltcmodPointer = destinationModulo;
    *bltdmodPointer = destinationModulo;
    if (source) {
        *bltamodPointer = sourceModulo;
        *bltaptPointer = source;
    } else {
        *bltadatPointer = 0;
    }
    *bltbptPointer = mask;
    *bltcptPointer = destination;
    *bltdptPointer = destination;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterCombineWithMask(uint16_t* background, uint16_t* source, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t backgroundModulo, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    blitterWait();
    *bltafwmPointer = firstWordMask;
    *bltalwmPointer = lastWordMask;
    *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
    *bltcon0Pointer = (uint16_t)((source ? BC0F_SRCA : 0) | BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
    *bltbmodPointer = maskModulo;
    *bltcmodPointer = backgroundModulo;
    *bltdmodPointer = destinationModulo;
    if (source) {
        *bltamodPointer = sourceModulo;
        *bltaptPointer = source;
    } else {
        *bltadatPointer = 0;
    }
    *bltbptPointer = mask;
    *bltcptPointer = background;
    *bltdptPointer = destination;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterPatternWithMask(uint16_t pattern, uint16_t* destination, uint16_t* mask, uint16_t width, uint16_t height, int16_t sourceModulo, int16_t destinationModulo, int16_t maskModulo, int16_t sourceShift, int16_t maskShift, uint16_t firstWordMask, uint16_t lastWordMask, bool clearMasked)
{
    blitterWait();
    *bltafwmPointer = firstWordMask;
    *bltalwmPointer = lastWordMask;
    *bltcon1Pointer = (uint16_t)((sourceShift < 0 ? BLITREVERSE : 0) | ((maskShift < 0 ? -maskShift : maskShift) << 12));
    *bltcon0Pointer = (uint16_t)(BC0F_SRCB | BC0F_SRCC | BC0F_DEST | ABC | ABNC | (clearMasked ? 0 : (NANBC | ANBC)) | ((sourceShift < 0 ? -sourceShift : sourceShift) << 12));
    *bltbmodPointer = maskModulo;
    *bltcmodPointer = destinationModulo;
    *bltdmodPointer = destinationModulo;
    *bltadatPointer = pattern;
    *bltbptPointer = mask;
    *bltcptPointer = destination;
    *bltdptPointer = destination;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterLine(uint16_t* data, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t bytesPerRow, bool singleBitPerRow)
{
    if (singleBitPerRow && y1 == y2) {
        return;
    }

    // Make sure the line is drawn from top to bottom
    if (y2 < y1) {
        uint16_t temp = x2;
        x2 = x1;
        x1 = temp;

        temp = y2;
        y2 = y1;
        y1 = temp;
    }

    uint16_t octant = 0;
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    if (dx == 0 && dy == 0) {
        return;
    }
    if (dx < 0) {
        octant += 2;
        dx = (int16_t)-dx;
    }
    if (dx >= (dy + dy)) {
        dy--;
    }

    uint16_t* firstWord = data + y1 * (bytesPerRow >> 1) + (x1 >> 4);
    if (dy < dx) {
        int16_t signedTemp = dy;
        dy = dx;
        dx = signedTemp;
        octant++;
    }
    uint16_t bltcon0Value = ((x1 & 15) << 12) | BC0F_SRCA | BC0F_SRCC | BC0F_DEST;
    uint16_t bltcon1Value = octants[octant];
    if (singleBitPerRow) {
        bltcon0Value |= A_XOR_C;
        bltcon1Value |= ONEDOT;
    } else {
        bltcon0Value |= A_OR_C;
    }
    int16_t v = dx + dx;

    blitterWait();
    *bltawmPointer = 0xffffffff;
    *bltadatPointer = 0x8000;
    *bltbdatPointer = 0xffff;
    *bltcmodPointer = (int16_t)bytesPerRow;
    *bltdmodPointer = (int16_t)bytesPerRow;
    *bltbmodPointer = v;
    v -= dy;
    if (v < 0) {
        bltcon1Value |= SIGNFLAG;
    }
    *bltaptlPointer = v;
    v -= dy;
    *bltamodPointer = v;
    *bltcon0Pointer = bltcon0Value;
    *bltcon1Pointer = bltcon1Value;
    *bltcptPointer = firstWord;
    *bltdptPointer = firstWord;
    *bltsizePointer = (uint16_t)((dy << 6) | 2);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::blitterFill(uint16_t* data, uint16_t width, uint16_t height, int16_t modulo)
{
    blitterWait();
    *bltawmPointer = 0xffffffff;
    *bltcon0Pointer = BC0F_SRCA | BC0F_DEST | A_TO_D;
    *bltcon1Pointer = BLITREVERSE | FILL_OR;
    *bltamodPointer = modulo;
    *bltdmodPointer = modulo;
    *bltaptPointer = data;
    *bltdptPointer = data;
    *bltsizePointer = (uint16_t)((height << 6) | width);
#ifdef DEBUG
    *color00Pointer = 0xf00;
#endif
}

void AmigaHardware::processBlitterQueue()
{
}
#endif
