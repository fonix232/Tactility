#pragma once

#include <Tactility/Mutex.h>
#include <Tactility/hal/display/DisplayDriver.h>
#include <FastEPD.h>

class FastEpdDisplayDriver : public tt::hal::display::DisplayDriver {

    FASTEPD* epd;
    std::shared_ptr<tt::Lock> lock;
    int graphicsMode;

public:

    FastEpdDisplayDriver(
        FASTEPD* epd,
        std::shared_ptr<tt::Lock> lock,
        int graphicsMode
    ) : epd(epd), 
        lock(lock), 
        graphicsMode(graphicsMode) 
    {}

    tt::hal::display::ColorFormat getColorFormat() const override {
        // FastEPD supports monochrome (1BPP) or 4BPP grayscale
        // For now, we'll use monochrome
        return tt::hal::display::ColorFormat::Monochrome;
    }

    bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override;

    uint16_t getPixelWidth() const override { return epd->width(); }

    uint16_t getPixelHeight() const override { return epd->height(); }

    std::shared_ptr<tt::Lock> getLock() const override { return lock; }
};
