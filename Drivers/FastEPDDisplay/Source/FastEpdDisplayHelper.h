#pragma once

/**
 * FastEPD Display Helper Functions
 * 
 * This file provides convenience functions for creating FastEpdDisplay instances
 * for common E-Ink panel configurations.
 */

#include "FastEpdDisplay.h"
#include <memory>

namespace FastEpdDisplayHelper {

/**
 * Create a FastEpdDisplay for M5Stack PaperS3
 * 
 * Note: The M5Paper S3 panel is defined as 960x540 in FastEPD library.
 * To use it in portrait mode (540x960), we apply a 90-degree rotation.
 * 
 * @param touch Optional touch device
 * @param busSpeed SPI bus speed (default 20MHz)
 * @param graphicsMode BB_MODE_1BPP or BB_MODE_4BPP (default 4bpp grayscale)
 * @param portraitMode If true, rotate to portrait (540x960). If false, use landscape (960x540)
 * @return Configured display instance
 */
inline std::shared_ptr<FastEpdDisplay> createM5PaperS3Display(
    std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr,
    uint32_t busSpeed = 20000000,
    int graphicsMode = BB_MODE_4BPP,
    bool portraitMode = false
) {
    // FastEPD defines M5Paper S3 as 960x540 (landscape)
    // For portrait mode (540x960), we use 90-degree rotation
    auto config = std::make_unique<FastEpdDisplay::Configuration>(
        BB_PANEL_M5PAPERS3,
        touch,
        busSpeed,
        graphicsMode,
        3,    // Partial passes
        5     // Full passes
    );
    
    return std::make_shared<FastEpdDisplay>(std::move(config));
}

} // namespace FastEpdDisplayHelper
