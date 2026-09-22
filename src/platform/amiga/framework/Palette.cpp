#include "Palette.h"

uint8_t Palette::fadeTable[16][16] = { 0 };

void Palette::initialize()
{
    for (int fade = 1; fade < 16; fade++) {
        for (int value = 0; value < 16; value++) {
            fadeTable[fade][value] = (uint8_t)((fade * value) / 15);
        }
    }
}

Palette::Palette(const uint16_t* colors, uint16_t colorCount, uint16_t fade, uint16_t fadeBaseColor) :
    sourceColors(0),
    currentColors(0),
    fade_(fade),
    fadeBaseColor(fadeBaseColor),
    isDirty(false)
{
    setColors(colors, colorCount);
}

Palette::~Palette()
{
    delete[] sourceColors;
    delete[] currentColors;
}

void Palette::setColors(const uint16_t* colors, uint16_t colorCount)
{
    delete[] sourceColors;
    delete[] currentColors;

    sourceColors = new uint16_t[colorCount];
    currentColors = new uint16_t[colorCount];
    colorCount_ = colorCount;
    isDirty = true;

    if (colors) {
        for (int i = 0; i < colorCount; i++) {
            sourceColors[i] = colors[i];
        }
    }
}

uint16_t Palette::colorCount() const
{
    return colorCount_;
}

void Palette::setFade(uint16_t fade)
{
    if (fade_ != fade) {
        fade_ = fade;
        isDirty = true;
    }
}

uint16_t Palette::fade() const
{
    return fade_;
}

void Palette::setFadeBaseColor(uint16_t fadeBaseColor)
{
    if (this->fadeBaseColor != fadeBaseColor) {
        this->fadeBaseColor = fadeBaseColor;
        isDirty = true;
    }
}

bool Palette::update()
{
    if (isDirty) {
        applyFade(sourceColors, currentColors, colorCount_, fade_, fadeBaseColor);

        isDirty = false;
        return true;
    }

    return false;
}

void Palette::applyFade(uint16_t* source, uint16_t* dest, uint16_t colorCount, uint16_t fade, uint16_t fadeBaseColor)
{
    if (!fadeTable[15][15]) {
        initialize();
    }

    switch (fade) {
        case 0: {
            for (uint16_t i = 0; i < colorCount; i++) {
                dest[i] = fadeBaseColor;
            }
            break;
        }
        case 15: {
            for (uint16_t i = 0; i < colorCount; i++) {
                dest[i] = source[i];
            }
            break;
        }
        default: {
            uint16_t baseR = fadeBaseColor >> 8;
            uint16_t baseG = (fadeBaseColor & 0x0f0) >> 4;
            uint16_t baseB = fadeBaseColor & 0x00f;
            uint8_t* table = fadeTable[fade];
            for (uint16_t i = 0; i < colorCount; i++) {
                uint16_t color = source[i];
                uint16_t r = color >> 8;
                uint16_t g = (color & 0x0f0) >> 4;
                uint16_t b = color & 0x00f;
                int16_t rDelta = r - baseR;
                int16_t gDelta = g - baseG;
                int16_t bDelta = b - baseB;
                uint16_t fadedR = baseR + (rDelta >= 0 ? table[rDelta] : -table[-rDelta]);
                uint16_t fadedG = baseG + (gDelta >= 0 ? table[gDelta] : -table[-gDelta]);
                uint16_t fadedB = baseB + (bDelta >= 0 ? table[bDelta] : -table[-bDelta]);
                dest[i] = (uint16_t)((fadedR << 8) | (fadedG << 4) | fadedB);
            }
            break;
        }
    }
}
