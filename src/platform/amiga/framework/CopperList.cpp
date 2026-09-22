#ifdef DEBUG
#include <proto/dos.h>
#endif
#define ECS_SPECIFIC
#include <hardware/custom.h>
#include <graphics/display.h>
#include "Copperlist.h"
#include "Palette.h"
#include "Palette24Bit.h"
#include "AmigaHardware.h"
#include "Sprite.h"
#include "Bitmap.h"

CopperList::CopperList() :
    data_(0),
    length(0)
{
}

CopperList::CopperList(uint32_t* data, uint32_t length) :
    data_(data),
    length(length)
{
    if (length > 0) {
        data_[0] = copperWait(16, 0);
        data_[length - 1] = copperWait(255, 254);
    }
}

CopperList::~CopperList()
{
#ifdef DEBUG
    Printf("Copperlist length %ld\n", length);
    for (int i = 0; i < length; i++) {
        Printf("%02lx %08lx\n", i, data_[i]);
    }
#endif
}

uint32_t* CopperList::data() const
{
    return data_;
}

CopperList* CopperList::allocate(uint32_t length)
{
    uint32_t* data = (uint32_t*)Util::getFreeMemoryPoolSegment(length << 2);
    return data ? new CopperList(data, length) : 0;
}

#ifndef ASSEMBLER
void CopperList::showSprite(uint32_t listIndex, uint16_t spriteNumber, const Sprite& sprite)
{
    uint32_t spriteData = (uint32_t)sprite.data();
    data_[listIndex++] = copperMove(spr1pth + (spriteNumber << 2), (uint16_t)(spriteData >> 16));
    data_[listIndex] = copperMove(spr1ptl + (spriteNumber << 2), (uint16_t)spriteData);
}

void CopperList::showBitmap(uint32_t listIndex, const Bitmap& bitmap, uint16_t firstBitplane, uint16_t bitplaneNumberDelta, int16_t xOffset, int16_t yOffset, uint16_t bitplaneCount)
{
    uint32_t bitplane = (uint32_t)bitmap.data;
    if (xOffset) {
        bitplane += xOffset >> 3;
    }
    if (yOffset) {
        bitplane += yOffset * bitmap.rowSizeInBytes;
    }
    if (bitplaneCount == 0) {
        bitplaneCount = bitmap.bitplanes;
    }
    uint16_t bitplanePointerRegister = bpl1pth + ((firstBitplane - 1) << 2);
    for (uint16_t i = 0; i < bitplaneCount; i++, bitplanePointerRegister += (bitplaneNumberDelta << 2)) {
        data_[listIndex++] = copperMove(bitplanePointerRegister, (uint16_t)(bitplane >> 16));
        data_[listIndex++] = copperMove(bitplanePointerRegister + 2, (uint16_t)bitplane);
        bitplane += bitmap.interleaved ? bitmap.widthInBytes : bitmap.bitplaneSizeInBytes;
    }
}

#endif

uint32_t CopperList::setPalette(uint32_t listIndex, const Palette& palette, uint16_t colorIndex, int16_t fromIndex, int16_t toIndex, bool forceWrite)
{
    if (toIndex < 0) {
        toIndex = (int16_t)(palette.colorCount() - 1);
    }

    if (const_cast<Palette&>(palette).update() || forceWrite) {
        data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((colorIndex & 0x00e0) << 8)));

        uint16_t colorRegister = color00 + ((colorIndex & 31) << 1);
        for (uint32_t i = fromIndex, j = colorIndex; i <= toIndex; i++, j++, colorRegister += 2) {
            if (j != colorIndex && (j & 31) == 0) {
                data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((j & 0x00e0) << 8)));
                colorRegister = color00;
            }

            data_[listIndex++] = copperMove(colorRegister, palette[i]);
        }
    } else {
        listIndex++;

        for (uint32_t i = fromIndex, j = colorIndex; i <= toIndex; i++, j++) {
            if (j != colorIndex && (j & 31) == 0) {
                listIndex++;
            }

            listIndex++;
        }
    }

    return listIndex;
}

void CopperList::setColor(uint32_t listIndex, uint16_t color, uint16_t count)
{
    uint16_t* wordData = (uint16_t*)data_;
    listIndex += listIndex + 1;

    for (uint16_t i = 0; i < count; i++, listIndex += 2) {
        wordData[listIndex] = color;
    }
}

uint32_t CopperList::setPalette24Bit(uint32_t listIndex, const Palette24Bit& palette, uint16_t colorIndex, int16_t fromIndex, int16_t toIndex, bool forceWrite)
{
    uint32_t i, j;
    if (toIndex < 0) {
        toIndex = (int16_t)(palette.colorCount() - 1);
    }

    if (const_cast<Palette24Bit&>(palette).update() || forceWrite) {
        data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((colorIndex & 0x00e0) << 8)));

        uint16_t colorRegister = color00 + ((colorIndex & 31) << 1);
        for (i = fromIndex, j = colorIndex; i <= toIndex; i++, j++, colorRegister += 2) {
            if (j != colorIndex && (j & 31) == 0) {
                data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((j & 0x00e0) << 8)));
                colorRegister = color00;
            }

            uint32_t color = palette[i];
            data_[listIndex++] = copperMove(colorRegister, (uint16_t)(((color & 0xf00000) >> 12) | ((color & 0x00f000) >> 8) | ((color & 0x0000f0) >> 4)));
        }

        data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((colorIndex & 0x00e0) << 8) | 0x0200));

        colorRegister = (uint16_t)(color00 + ((colorIndex & 31) << 1));
        for (i = fromIndex, j = colorIndex; i <= toIndex; i++, j++, colorRegister += 2) {
            if (j != colorIndex && (j & 31) == 0) {
                data_[listIndex++] = copperMove(bplcon3, (uint16_t)(AmigaHardware::bplcon3BaseValue | ((j & 0x00e0) << 8) | 0x200));
                colorRegister = color00;
            }

            uint32_t color = palette[i];
            data_[listIndex++] = copperMove(colorRegister, (uint16_t)(((color & 0x0f0000) >> 8) | ((color & 0x000f00) >> 4) | (color & 0x00000f)));
        }
    } else {
        listIndex++;

        for (i = fromIndex, j = colorIndex; i <= toIndex; i++, j++) {
            if (j != colorIndex && (j & 31) == 0) {
                listIndex++;
            }
            listIndex++;
        }

        listIndex++;

        for (i = fromIndex, j = colorIndex; i <= toIndex; i++, j++) {
            if (j != colorIndex && (j & 31) == 0) {
                listIndex++;
            }
            listIndex++;
        }
    }

    return listIndex;
}

uint32_t CopperList::setPlayfield(uint32_t listIndex, uint16_t width, uint16_t height, uint8_t bitplaneCount, bool interleaved, bool hires, bool interlace, bool dualPlayfield, bool holdAndModify, bool killEHB, uint16_t centerY)
{
    uint16_t halfHeight = height >> (interlace ? 2 : 1);
    uint16_t bitplaneWidth = width >> 3;
    uint16_t halfFetchWidth = hires ? (bitplaneWidth >> 1) : bitplaneWidth;

    data_[listIndex++] = copperMove(fmode, AmigaHardware::hasAGAChipSet ? 3 : 0);
    data_[listIndex++] = copperMove(bplcon3, AmigaHardware::bplcon3BaseValue);
    data_[listIndex++] = copperMove(bplcon2, killEHB ? 0x0224 : 0x0024);
    data_[listIndex++] = copperMove(bplcon1, (AmigaHardware::hasAGAChipSet && halfFetchWidth > 40) ? 0x7777 : 0);
    data_[listIndex++] = copperMove(bplcon0, (uint16_t)(((bitplaneCount & 8) << 1) | ((bitplaneCount & 7) << PLNCNTSHFT) | (hires ? MODE_640 : 0) | (interlace ? INTERLACE : 0) | (dualPlayfield ? DBLPF : 0) | (holdAndModify ? HOLDNMODIFY : 0) | USE_BPLCON3));
    data_[listIndex++] = copperMove(diwstrt, (uint16_t)(((centerY - halfHeight) << 8) | (0x121 - (width >> (hires ? 2 : 1)))));
    data_[listIndex++] = copperMove(diwstop, (uint16_t)(((centerY + halfHeight) << 8) | (0x21 + (width >> (hires ? 2 : 1)))));
    data_[listIndex++] = copperMove(diwhigh, 0x2100);
    if (AmigaHardware::hasAGAChipSet) {
        data_[listIndex++] = copperMove(ddfstrt, (uint16_t)(0x48 - halfFetchWidth + (hires ? 16 : 0)));
        data_[listIndex++] = copperMove(ddfstop, (uint16_t)(0x70 + halfFetchWidth + (hires ? 32 : 0)));
    } else {
        // This isn't quite right
        data_[listIndex++] = copperMove(ddfstrt, (uint16_t)(0x58 - halfFetchWidth));
        data_[listIndex++] = copperMove(ddfstop, (uint16_t)(0xb0 + halfFetchWidth));
    }
    data_[listIndex++] = copperMove(bpl1mod, (uint16_t)(interleaved ? ((bitplaneCount - 1) * bitplaneWidth) : 0));
    data_[listIndex++] = copperMove(bpl2mod, (uint16_t)(interleaved ? ((bitplaneCount - 1) * bitplaneWidth) : 0));

    return listIndex;
}
