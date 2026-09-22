#ifndef _PALETTE_H
#define _PALETTE_H

#include "Util.h"

class Palette {
public:
    Palette(const uint16_t* colors, uint16_t colorCount, uint16_t fade = 15, uint16_t fadeBaseColor = 0x000);
    ~Palette();

    void setColors(const uint16_t* colors, uint16_t colorCount);
    __inline uint16_t colorCount() const;
    void setFade(uint16_t fade);
    __inline uint16_t fade() const;
    void setFadeBaseColor(uint16_t fadeBaseColor);
    virtual bool update();

    inline uint16_t operator[](int index) { return currentColors[index]; }
    inline const uint16_t operator[](int index) const { return currentColors[index]; }

private:
    static void initialize();

    uint16_t fade_;

protected:
    static uint8_t fadeTable[16][16];
    void applyFade(uint16_t* source, uint16_t* dest, uint16_t colorCount, uint16_t fade, uint16_t fadeBaseColor = 0x000);

    uint16_t* sourceColors;
    uint16_t* currentColors;
    uint16_t colorCount_;
    uint16_t fadeBaseColor;
    bool isDirty;
};

#endif
