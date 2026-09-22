#include "AmigaHardware.h"
#include "Bitmap.h"

Bitmap::Bitmap(void* data, uint16_t width, uint16_t height, uint16_t bitplanes, bool interleaved, uint16_t bitmapDataWidth) :
    data(data),
    width(width),
    height(height),
    bitplanes(bitplanes),
    dataWidth(bitmapDataWidth ? bitmapDataWidth : width),
    widthInBytes(((dataWidth + 15) >> 4) << 1),
    rowSizeInBytes(interleaved ? (bitplanes * widthInBytes) : widthInBytes),
    bitplaneSizeInBytes(widthInBytes * height),
    interleaved(interleaved),
    blittable((uint32_t)data < 0x200000 ? true : false)
{
}

uint16_t Bitmap::widthInWords() const
{
    return widthInBytes >> 1;
}

uint16_t Bitmap::rowSizeInWords() const
{
    return rowSizeInBytes >> 1;
}

uint16_t Bitmap::bitplaneSizeInWords() const
{
    return bitplaneSizeInBytes >> 1;
}

uint32_t Bitmap::dataSize() const
{
    return (dataWidth >> 3) * height * bitplanes;
}

Bitmap* Bitmap::allocate(uint16_t width, uint16_t height, uint16_t bitplanes, bool interleaved, uint16_t dataWidth)
{
    if (dataWidth == 0) {
        dataWidth = width;
    }

    uint32_t bitmapSize = (dataWidth >> 3) * height * bitplanes;
    void* data = Util::getFreeMemoryPoolSegment(bitmapSize);
    return data ? new Bitmap(data, width, height, bitplanes, interleaved, dataWidth) : 0;
}

Bitmap* Bitmap::generateMask(const Bitmap& source, void* data, bool singleBitplane)
{
    uint16_t width = source.width;
    uint16_t height = source.height;
    uint16_t sourceBitplanes = source.bitplanes;
    uint16_t maskBitplanes = singleBitplane ? 1 : sourceBitplanes;
    bool interleaved = source.interleaved;
    uint16_t dataWidth = source.dataWidth;

    Bitmap* mask = data ? new Bitmap(data, width, height, maskBitplanes, interleaved, dataWidth) : allocate(width, height, maskBitplanes, interleaved);

    uint16_t widthWords = dataWidth >> 4;
    uint16_t* sourceData = (uint16_t*)source.data;
    uint16_t* maskData = (uint16_t*)mask->data;
    uint16_t bitplane;
    uint16_t sourceRowModulo = interleaved ? ((sourceBitplanes - 1) * widthWords) : 0;
    uint16_t destRowModulo = interleaved ? ((maskBitplanes - 1) * widthWords) : 0;
    uint16_t bitplaneModulo = widthWords * (interleaved ? 1 : height);
    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < widthWords; x++) {
            uint16_t word = 0;
            uint16_t* data = sourceData++;
            for (bitplane = 0; bitplane < sourceBitplanes; bitplane++, data += bitplaneModulo) {
                word |= *data;
            }
            data = maskData++;
            for (bitplane = 0; bitplane < maskBitplanes; bitplane++, data += bitplaneModulo) {
                *data = word;
            }
        }
        sourceData += sourceRowModulo;
        maskData += destRowModulo;
    }

    return mask;
}

#ifndef ASSEMBLER
void Bitmap::clear(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (width == 0) {
        width = this->width;
    }
    if (height == 0) {
        height = this->height;
    }

    // Calculate blit width in words
    uint16_t destFirstWord = x >> 4;
    uint16_t destLastWord = (x + width - 1) >> 4;
    uint16_t widthWords = destLastWord - destFirstWord + 1;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;
    uint16_t* destData = (uint16_t*)data + y * this->rowSizeInWords() + destFirstWord;

    if (blittable) {
        if (interleaved) {
            AmigaHardware::blitterClear(destData, widthWords, bitplanes * height, destRowModulo << 1);
        } else {
            for (uint16_t i = 0; i < bitplanes; i++) {
                AmigaHardware::blitterClear(destData, widthWords, height, destRowModulo << 1);
                destData += this->bitplaneSizeInWords();
            }
        }
    } else {
        int16_t destBitplaneModulo = interleaved ? 0 : ((this->height - height) * destWidthWords);

        for (uint16_t i = 0; i < bitplanes; i++) {
            for (uint16_t j = 0; j < height; j++) {
                for (uint16_t k = 0; k < widthWords; k++) {
                    *destData++ = 0;
                }
                destData += destRowModulo;
            }
            destData += destBitplaneModulo;
        }
    }
}

void Bitmap::copy(const Bitmap& source, uint16_t destX, uint16_t destY, uint16_t sourceX, uint16_t sourceY, uint16_t width, uint16_t height, uint16_t mask)
{
    if (width == 0) {
        width = source.width;
    }
    if (height == 0) {
        height = source.height;
    }

    // Calculate blit width in words
    uint16_t sourceFirstWord = sourceX >> 4;
    uint16_t sourceLastWord = (sourceX + width - 1) >> 4;
    uint16_t destFirstWord = destX >> 4;
    uint16_t destLastWord = (destX + width - 1) >> 4;
    uint16_t sourceWords = sourceLastWord - sourceFirstWord;
    uint16_t destWords = destLastWord - destFirstWord;
    uint16_t widthWords = destWords;
    if (sourceWords > destWords) {
        widthWords = sourceWords;
        sourceLastWord = (uint16_t)(sourceFirstWord + widthWords);
        destLastWord = (uint16_t)(destFirstWord + widthWords);
    }
    widthWords++;

    // Calculate modulos
    uint16_t sourceWidthWords = source.widthInWords();
    int16_t sourceRowModulo = sourceWidthWords - widthWords;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;

    // Calculate shift, masks and data starting address
    uint16_t sourceLeftShift = sourceX & 15;
    uint16_t destRightShift = destX & 15;
    int16_t shift = destRightShift - sourceLeftShift;
    uint16_t firstWordMask = 0xffff;
    uint16_t lastWordMask = 0xffff;
    uint16_t* sourceData = (uint16_t*)source.data;
    uint16_t* destData = (uint16_t*)data;
    if (shift >= 0) {
        // Shift is to the right: blit forward starting from the first word
        sourceData += sourceY * source.rowSizeInWords() + sourceFirstWord;
        destData += destY * this->rowSizeInWords() + destFirstWord;
        lastWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        firstWordMask >>= sourceLeftShift;
    } else {
        // Shift is to the left: blit reverse starting from the last word
        sourceData += (sourceY + height) * source.rowSizeInWords() - sourceWidthWords + sourceLastWord;
        destData += (destY + height) * this->rowSizeInWords() - destWidthWords + destLastWord;
        firstWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        lastWordMask >>= sourceLeftShift;
    }

    if (blittable && source.blittable) {
        if (interleaved) {
            AmigaHardware::blitterCopy(sourceData, destData, widthWords, bitplanes * height, sourceRowModulo << 1, destRowModulo << 1, shift, firstWordMask, lastWordMask, mask);
        } else {
            for (uint16_t i = 0; i < this->bitplanes; i++) {
                AmigaHardware::blitterCopy(i < source.bitplanes ? sourceData : 0, destData, widthWords, height, sourceRowModulo << 1, destRowModulo << 1, shift, firstWordMask, lastWordMask, mask);
                sourceData += source.bitplaneSizeInWords();
                destData += this->bitplaneSizeInWords();
            }
        }
    } else {
        int16_t sourceBitplaneModulo = source.interleaved ? 0 : ((source.height - height) * sourceWidthWords);
        int16_t destBitplaneModulo = interleaved ? 0 : ((this->height - height) * destWidthWords);
        uint16_t bitplanes = this->bitplanes < source.bitplanes ? this->bitplanes : source.bitplanes;

        for (uint16_t i = 0; i < bitplanes; i++) {
            for (uint16_t j = 0; j < height; j++) {
                for (uint16_t k = 0; k < widthWords; k++) {
                    *destData++ = (uint16_t)(*sourceData++ & mask);
                }
                sourceData += sourceRowModulo;
                destData += destRowModulo;
            }
            sourceData += sourceBitplaneModulo;
            destData += destBitplaneModulo;
        }
    }
}

void Bitmap::copyWithMask(const Bitmap& source, const Bitmap& mask, uint16_t destX, uint16_t destY, uint16_t sourceX, uint16_t sourceY, uint16_t maskX, uint16_t maskY, uint16_t width, uint16_t height, bool clearMasked)
{
    if (width == 0) {
        width = source.width;
    }
    if (height == 0) {
        height = source.height;
    }

    // Calculate blit width in words
    uint16_t sourceFirstWord = sourceX >> 4;
    uint16_t sourceLastWord = (sourceX + width - 1) >> 4;
    uint16_t sourceWords = sourceLastWord - sourceFirstWord;
    uint16_t destFirstWord = destX >> 4;
    uint16_t destLastWord = (destX + width - 1) >> 4;
    uint16_t destWords = destLastWord - destFirstWord;
    uint16_t widthWords = destWords;
    if (sourceWords > destWords) {
        widthWords = sourceWords;
        sourceLastWord = (uint16_t)(sourceFirstWord + widthWords);
        destLastWord = (uint16_t)(destFirstWord + widthWords);
    }
    uint16_t maskFirstWord = maskX >> 4;
    uint16_t maskLastWord = (uint16_t)(maskFirstWord + widthWords);
    widthWords++;

    // Calculate modulos
    uint16_t sourceWidthWords = source.widthInWords();
    int16_t sourceRowModulo = sourceWidthWords - widthWords;
    uint16_t maskWidthWords = mask.widthInWords();
    int16_t maskRowModulo = maskWidthWords - widthWords;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;

    // Calculate shift, masks and data starting address
    uint16_t sourceLeftShift = sourceX & 15;
    uint16_t maskLeftShift = maskX & 15;
    uint16_t destRightShift = destX & 15;
    int16_t sourceShift = destRightShift - sourceLeftShift;
    int16_t maskShift = destRightShift - maskLeftShift;
    uint16_t firstWordMask = 0xffff;
    uint16_t lastWordMask = 0xffff;
    uint16_t* sourceData = (uint16_t*)source.data;
    uint16_t* maskData = (uint16_t*)mask.data;
    uint16_t* destData = (uint16_t*)data;
    if (sourceShift >= 0) {
        // Shift is to the right: blit forward starting from the first word
        sourceData += sourceY * source.rowSizeInWords() + sourceFirstWord;
        maskData += maskY * mask.rowSizeInWords() + maskFirstWord;
        destData += destY * this->rowSizeInWords() + destFirstWord;
        lastWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        firstWordMask >>= sourceLeftShift;

        if (maskShift < 0) {
            maskShift += 16;
        }
    } else {
        // Shift is to the left: blit reverse starting from the last word
        sourceData += (sourceY + height) * source.rowSizeInWords() - sourceWidthWords + sourceLastWord;
        maskData += (maskY + height) * mask.rowSizeInWords() - maskWidthWords + maskLastWord;
        destData += (destY + height) * this->rowSizeInWords() - destWidthWords + destLastWord;
        firstWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        lastWordMask >>= sourceLeftShift;

        if (maskShift > 0) {
            maskShift -= 16;
            maskData--;
        }
    }

    if (blittable && source.blittable) {
        if (interleaved) {
            if (mask.bitplanes == 1) {
                uint16_t bitplanes = this->bitplanes < source.bitplanes ? this->bitplanes : source.bitplanes;
                sourceRowModulo += source.rowSizeInWords() - sourceWidthWords;
                destRowModulo += this->rowSizeInWords() - destWidthWords;
                for (uint16_t i = 0; i < bitplanes; i++) {
                    AmigaHardware::blitterCopyWithMask(sourceData, destData, maskData, widthWords, height, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
                    sourceData += sourceShift >= 0 ? sourceWidthWords : -sourceWidthWords;
                    destData += sourceShift >= 0 ? destWidthWords : -destWidthWords;
                }
            } else {
                AmigaHardware::blitterCopyWithMask(sourceData, destData, maskData, widthWords, bitplanes * height, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
            }
        } else {
            for (uint16_t i = 0; i < this->bitplanes; i++) {
                AmigaHardware::blitterCopyWithMask(i < source.bitplanes ? sourceData : 0, destData, maskData, widthWords, height, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
                sourceData += source.bitplaneSizeInWords();
                if (mask.bitplanes > 1) {
                    maskData += mask.bitplaneSizeInWords();
                }
                destData += this->bitplaneSizeInWords();
            }
        }
    } else {
        int16_t sourceBitplaneModulo = source.interleaved ? 0 : ((source.height - height) * sourceWidthWords);
        int16_t destBitplaneModulo = interleaved ? 0 : ((this->height - height) * destWidthWords);
        int16_t maskBitplaneModulo = mask.bitplanes == 1 ?
            (-height * maskWidthWords) :
            (mask.interleaved ? 0 : ((mask.height - height) * maskWidthWords));

        for (uint16_t i = 0; i < this->bitplanes; i++) {
            if (i < source.bitplanes) {
                for (uint16_t j = 0; j < height; j++) {
                    for (uint16_t k = 0; k < widthWords; k++, sourceData++, maskData++, destData++) {
                        *destData = (uint16_t)((*destData & ~*maskData) | (*sourceData & *maskData));
                    }
                    sourceData += sourceRowModulo;
                    maskData += maskRowModulo;
                    destData += destRowModulo;
                }
                sourceData += sourceBitplaneModulo;
                maskData += maskBitplaneModulo;
                destData += destBitplaneModulo;
            } else {
                for (uint16_t j = 0; j < height; j++) {
                    for (uint16_t k = 0; k < widthWords; k++, maskData++, destData++) {
                        *destData = (uint16_t)(*destData & ~*maskData);
                    }
                    maskData += maskRowModulo;
                    destData += destRowModulo;
                }
                maskData += maskBitplaneModulo;
                destData += destBitplaneModulo;
            }
        }
    }
}

void Bitmap::line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color, bool fillMode)
{
    uint16_t* destData = (uint16_t*)data;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destBitplaneModulo = interleaved ? destWidthWords : (height * destWidthWords);

    for (uint16_t i = 0; i < bitplanes && color != 0; i++, color >>= 1, destData += destBitplaneModulo) {
        if (color & 1) {
            AmigaHardware::blitterLine(destData, x1, y1, x2, y2, rowSizeInBytes, fillMode);
        }
    }
}

void Bitmap::fill(uint16_t x, uint16_t y, uint16_t fillWidth, uint16_t fillHeight)
{
    if (fillWidth == 0) {
        fillWidth = width;
    }
    if (fillHeight == 0) {
        fillHeight = height;
    }

    // Calculate blit width in words
    uint16_t destFirstWord = x >> 4;
    uint16_t destLastWord = (x + fillWidth - 1) >> 4;
    uint16_t widthWords = destLastWord - destFirstWord + 1;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;
    uint16_t* destData = (uint16_t*)data + (y + fillHeight - 1) * this->rowSizeInWords() + destLastWord;

    if (interleaved) {
        AmigaHardware::blitterFill(destData, widthWords, bitplanes * fillHeight, destRowModulo << 1);
    } else {
        for (uint16_t i = 0; i < bitplanes; i++) {
            AmigaHardware::blitterFill(destData, widthWords, fillHeight, destRowModulo << 1);
            destData -= this->bitplaneSizeInWords();
        }
    }
}
#endif

void Bitmap::polygon(const Polygon& polygon, uint16_t color, bool fillMode)
{
    int point = 0;
    for (point = 0; point < polygon.size - 1; point++) {
        line(polygon.points[point].x, polygon.points[point].y, polygon.points[point + 1].x, polygon.points[point + 1].y, color, fillMode);
    }
    line(polygon.points[point].x, polygon.points[point].y, polygon.points[0].x, polygon.points[0].y, color, fillMode);

    if (fillMode) {
        Rect rect = polygon.boundingRect();
        fill(rect.topLeft.x, rect.topLeft.y, rect.width, rect.height);
    }
}

void Bitmap::combineWithMask(const Bitmap& background, const Bitmap& source, const Bitmap& mask, uint16_t destX, uint16_t destY, uint16_t backgroundX, uint16_t backgroundY, uint16_t sourceX, uint16_t sourceY, uint16_t maskX, uint16_t maskY, uint16_t width, uint16_t height, bool clearMasked)
{
    if (width == 0) {
        width = source.width;
    }
    if (height == 0) {
        height = source.height;
    }

    // Calculate blit width in words
    uint16_t backgroundFirstWord = backgroundX >> 4;
    uint16_t backgroundLastWord = (backgroundX + width - 1) >> 4;
    uint16_t sourceFirstWord = sourceX >> 4;
    uint16_t sourceLastWord = (sourceX + width - 1) >> 4;
    uint16_t sourceWords = sourceLastWord - sourceFirstWord;
    uint16_t destFirstWord = destX >> 4;
    uint16_t destLastWord = (destX + width - 1) >> 4;
    uint16_t destWords = destLastWord - destFirstWord;
    uint16_t widthWords = destWords;
    if (sourceWords > destWords) {
        widthWords = sourceWords;
        backgroundLastWord = (uint16_t)(backgroundFirstWord + widthWords);
        sourceLastWord = (uint16_t)(sourceFirstWord + widthWords);
        destLastWord = (uint16_t)(destFirstWord + widthWords);
    }
    uint16_t maskFirstWord = maskX >> 4;
    uint16_t maskLastWord = (uint16_t)(maskFirstWord + widthWords);
    widthWords++;

    // Calculate modulos
    uint16_t backgroundWidthWords = background.widthInWords();
    int16_t backgroundRowModulo = backgroundWidthWords - widthWords;
    uint16_t sourceWidthWords = source.widthInWords();
    int16_t sourceRowModulo = sourceWidthWords - widthWords;
    uint16_t maskWidthWords = mask.widthInWords();
    int16_t maskRowModulo = maskWidthWords - widthWords;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;

    // Calculate shift, masks and data starting address
    uint16_t sourceLeftShift = sourceX & 15;
    uint16_t maskLeftShift = maskX & 15;
    uint16_t destRightShift = destX & 15;
    int16_t sourceShift = destRightShift - sourceLeftShift;
    int16_t maskShift = destRightShift - maskLeftShift;
    uint16_t firstWordMask = 0xffff;
    uint16_t lastWordMask = 0xffff;
    uint16_t* backgroundData = (uint16_t*)background.data;
    uint16_t* sourceData = (uint16_t*)source.data;
    uint16_t* maskData = (uint16_t*)mask.data;
    uint16_t* destData = (uint16_t*)data;
    if (sourceShift >= 0) {
        // Shift is to the right: blit forward starting from the first word
        backgroundData += backgroundY * background.rowSizeInWords() + backgroundFirstWord;
        sourceData += sourceY * source.rowSizeInWords() + sourceFirstWord;
        maskData += maskY * mask.rowSizeInWords() + maskFirstWord;
        destData += destY * this->rowSizeInWords() + destFirstWord;
        lastWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        firstWordMask >>= sourceLeftShift;

        if (maskShift < 0) {
            maskShift += 16;
        }
    } else {
        // Shift is to the left: blit reverse starting from the last word
        backgroundData += (backgroundY + height) * background.rowSizeInWords() - backgroundWidthWords + backgroundLastWord;
        sourceData += (sourceY + height) * source.rowSizeInWords() - sourceWidthWords + sourceLastWord;
        maskData += (maskY + height) * mask.rowSizeInWords() - maskWidthWords + maskLastWord;
        destData += (destY + height) * this->rowSizeInWords() - destWidthWords + destLastWord;
        firstWordMask <<= ((sourceLastWord << 4) + 16 - sourceX - width);
        lastWordMask >>= sourceLeftShift;

        if (maskShift > 0) {
            maskShift -= 16;
            maskData--;
        }
    }

    if (blittable && source.blittable) {
        if (interleaved) {
            if (mask.bitplanes == 1) {
                uint16_t bitplanes = this->bitplanes < source.bitplanes ? this->bitplanes : source.bitplanes;
                backgroundRowModulo += background.rowSizeInWords() - backgroundWidthWords;
                sourceRowModulo += source.rowSizeInWords() - sourceWidthWords;
                destRowModulo += this->rowSizeInWords() - destWidthWords;
                for (uint16_t i = 0; i < bitplanes; i++) {
                    AmigaHardware::blitterCombineWithMask(backgroundData, sourceData, destData, maskData, widthWords, height, backgroundRowModulo << 1, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
                    backgroundData += sourceShift >= 0 ? backgroundWidthWords : -backgroundWidthWords;
                    sourceData += sourceShift >= 0 ? sourceWidthWords : -sourceWidthWords;
                    destData += sourceShift >= 0 ? destWidthWords : -destWidthWords;
                }
            } else {
                AmigaHardware::blitterCombineWithMask(backgroundData, sourceData, destData, maskData, widthWords, bitplanes * height, backgroundRowModulo << 1, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
            }
        } else {
            for (uint16_t i = 0; i < this->bitplanes; i++) {
                AmigaHardware::blitterCombineWithMask(backgroundData, i < source.bitplanes ? sourceData : 0, destData, maskData, widthWords, height, backgroundRowModulo << 1, sourceRowModulo << 1, destRowModulo << 1, maskRowModulo << 1, sourceShift, maskShift, firstWordMask, lastWordMask, clearMasked);
                backgroundData += background.bitplaneSizeInWords();
                sourceData += source.bitplaneSizeInWords();
                if (mask.bitplanes > 1) {
                    maskData += mask.bitplaneSizeInWords();
                }
                destData += this->bitplaneSizeInWords();
            }
        }
    } else {
        int16_t backgroundBitplaneModulo = background.interleaved ? 0 : ((background.height - height) * backgroundWidthWords);
        int16_t sourceBitplaneModulo = source.interleaved ? 0 : ((source.height - height) * sourceWidthWords);
        int16_t destBitplaneModulo = interleaved ? 0 : ((this->height - height) * destWidthWords);
        int16_t maskBitplaneModulo = mask.bitplanes == 1 ?
            (-height * maskWidthWords) :
            (mask.interleaved ? 0 : ((mask.height - height) * maskWidthWords));

        for (uint16_t i = 0; i < this->bitplanes; i++) {
            if (i < source.bitplanes) {
                for (uint16_t j = 0; j < height; j++) {
                    for (uint16_t k = 0; k < widthWords; k++, backgroundData++, sourceData++, maskData++, destData++) {
                        *destData = (uint16_t)((*backgroundData & ~*maskData) | (*sourceData & *maskData));
                    }
                    backgroundData += backgroundRowModulo;
                    sourceData += sourceRowModulo;
                    maskData += maskRowModulo;
                    destData += destRowModulo;
                }
                backgroundData += backgroundBitplaneModulo;
                sourceData += sourceBitplaneModulo;
                maskData += maskBitplaneModulo;
                destData += destBitplaneModulo;
            } else {
                for (uint16_t j = 0; j < height; j++) {
                    for (uint16_t k = 0; k < widthWords; k++, backgroundData++, maskData++, destData++) {
                        *destData = (uint16_t)(*backgroundData & ~*maskData);
                    }
                    backgroundData += backgroundRowModulo;
                    maskData += maskRowModulo;
                    destData += destRowModulo;
                }
                backgroundData += backgroundBitplaneModulo;
                maskData += maskBitplaneModulo;
                destData += destBitplaneModulo;
            }
        }
    }
}

void Bitmap::patternWithMask(const Bitmap& mask, uint16_t destX, uint16_t destY, uint16_t maskX, uint16_t maskY, uint16_t width, uint16_t height, uint16_t pattern)
{
    if (width == 0) {
        width = mask.width;
    }
    if (height == 0) {
        height = mask.height;
    }

    // Calculate blit width in words
    uint16_t maskFirstWord = maskX >> 4;
    uint16_t maskLastWord = (maskX + width - 1) >> 4;
    uint16_t destFirstWord = destX >> 4;
    uint16_t destLastWord = (destX + width - 1) >> 4;
    uint16_t maskWords = maskLastWord - maskFirstWord;
    uint16_t destWords = destLastWord - destFirstWord;
    uint16_t widthWords = destWords;
    if (maskWords > destWords) {
        widthWords = maskWords;
        maskLastWord = (uint16_t)(maskFirstWord + widthWords);
        destLastWord = (uint16_t)(destFirstWord + widthWords);
    }
    widthWords++;

    // Calculate modulos
    uint16_t maskWidthWords = mask.widthInWords();
    int16_t maskRowModulo = maskWidthWords - widthWords;
    uint16_t destWidthWords = this->widthInWords();
    int16_t destRowModulo = destWidthWords - widthWords;

    // Calculate shift, masks and data starting address
    uint16_t maskLeftShift = maskX & 15;
    uint16_t destRightShift = destX & 15;
    int16_t shift = destRightShift - maskLeftShift;
    uint16_t firstWordMask = 0xffff;
    uint16_t lastWordMask = 0xffff;
    uint16_t* maskData = (uint16_t*)mask.data;
    uint16_t* destData = (uint16_t*)data;
    if (shift >= 0) {
        // Shift is to the right: blit forward starting from the first word
        maskData += maskY * mask.rowSizeInWords() + maskFirstWord;
        destData += destY * this->rowSizeInWords() + destFirstWord;
        lastWordMask <<= ((maskLastWord << 4) + 16 - maskX - width);
        firstWordMask >>= maskLeftShift;
    } else {
        // Shift is to the left: blit reverse starting from the last word
        maskData += (maskY + height) * mask.rowSizeInWords() - maskWidthWords + maskLastWord;
        destData += (destY + height) * this->rowSizeInWords() - destWidthWords + destLastWord;
        firstWordMask <<= ((maskLastWord << 4) + 16 - maskX - width);
        lastWordMask >>= maskLeftShift;
    }

    if (blittable && mask.blittable) {
        if (interleaved) {
            AmigaHardware::blitterPatternWithMask(pattern, destData, maskData, widthWords, bitplanes * height, 0, destRowModulo << 1, maskRowModulo << 1, 0, shift, firstWordMask, lastWordMask, false);
        } else {
            for (uint16_t i = 0; i < this->bitplanes; i++) {
                AmigaHardware::blitterPatternWithMask(pattern, destData, maskData, widthWords, height, 0, destRowModulo << 1, maskRowModulo << 1, 0, shift, firstWordMask, lastWordMask, false);
                maskData += mask.bitplaneSizeInWords();
                destData += this->bitplaneSizeInWords();
            }
        }
    } else {
        int16_t maskBitplaneModulo = mask.interleaved ? 0 : ((mask.height - height) * maskWidthWords);
        int16_t destBitplaneModulo = interleaved ? 0 : ((this->height - height) * destWidthWords);
        uint16_t bitplanes = this->bitplanes < mask.bitplanes ? this->bitplanes : mask.bitplanes;

        for (uint16_t i = 0; i < bitplanes; i++) {
            for (uint16_t j = 0; j < height; j++) {
                for (uint16_t k = 0; k < widthWords; k++, destData++, maskData++) {
                    *destData = (uint16_t)((*destData & ~*maskData) | (pattern & *maskData));
                }
                maskData += maskRowModulo;
                destData += destRowModulo;
            }
            maskData += maskBitplaneModulo;
            destData += destBitplaneModulo;
        }
    }
}
