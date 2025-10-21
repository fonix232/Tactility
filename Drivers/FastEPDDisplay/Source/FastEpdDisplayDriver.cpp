#include "FastEpdDisplayDriver.h"
#include <Tactility/LogEsp.h>
#include <cstring>

constexpr const char* TAG = "FastEpdDisplayDriver";

bool FastEpdDisplayDriver::drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) {
    if (epd == nullptr || pixelData == nullptr) {
        TT_LOG_E(TAG, "Invalid parameters");
        return false;
    }

    // Calculate dimensions
    int bmpWidth = xEnd - xStart;
    int bmpHeight = yEnd - yStart;
    
    if (bmpWidth <= 0 || bmpHeight <= 0) {
        TT_LOG_E(TAG, "Invalid bitmap dimensions: %dx%d", bmpWidth, bmpHeight);
        return false;
    }

    TT_LOG_D(TAG, "Drawing bitmap: x=%d-%d, y=%d-%d (%dx%d)", 
             xStart, xEnd, yStart, yEnd, bmpWidth, bmpHeight);

    // For monochrome mode (1BPP), we need to convert the pixel data
    // The pixelData is expected to be in the format required by the ColorFormat
    const uint8_t* srcData = static_cast<const uint8_t*>(pixelData);
    
    // Copy pixel data to EPD buffer
    // We'll use drawPixel for simplicity, but this could be optimized
    for (int y = 0; y < bmpHeight; y++) {
        for (int x = 0; x < bmpWidth; x++) {
            int srcX = x;
            int srcY = y;
            int dstX = xStart + x;
            int dstY = yStart + y;
            
            // For monochrome (1BPP), 8 pixels per byte
            int byteIndex = (srcY * ((bmpWidth + 7) / 8)) + (srcX / 8);
            int bitIndex = 7 - (srcX % 8);
            uint8_t pixel = (srcData[byteIndex] >> bitIndex) & 0x01;
            
            // Draw the pixel (0 = black, 1 = white in typical 1BPP format)
            epd->drawPixel(dstX, dstY, pixel ? BBEP_WHITE : BBEP_BLACK);
        }
    }

    return true;
}
