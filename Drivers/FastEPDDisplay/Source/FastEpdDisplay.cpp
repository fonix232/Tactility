#include "FastEpdDisplay.h"
#include "FastEpdDisplayDriver.h"

#include <Tactility/Check.h>
#include <Tactility/LogEsp.h>

#include <esp_heap_caps.h>
#include <cstring>

constexpr const char* TAG = "FastEpdDisplay";

FastEpdDisplay::FastEpdDisplay(std::unique_ptr<Configuration> inConfiguration)
    : configuration(std::move(inConfiguration)),
      lock(std::make_shared<tt::Mutex>())
{
    tt_check(configuration != nullptr);
}

FastEpdDisplay::~FastEpdDisplay() {
    if (lvglDisplay != nullptr) {
        stopLvgl();
    }
    if (initialized) {
        stop();
    }
    if (displayDriver != nullptr && displayDriver.use_count() > 1) {
        tt_crash("DisplayDriver is still in use. This will cause memory access violations.");
    }
}

bool FastEpdDisplay::start() {
    if (initialized) {
        TT_LOG_W(TAG, "Already initialized");
        return true;
    }

    TT_LOG_I(TAG, "Initializing FastEPD...");
    TT_LOG_I(TAG, "Panel type: %d, Bus speed: %lu Hz", 
             configuration->panelType, configuration->busSpeed);
    
    // Initialize the panel
    int rc = epd.initPanel(configuration->panelType, configuration->busSpeed);
    if (rc != BBEP_SUCCESS) {
        TT_LOG_E(TAG, "Failed to initialize EPD panel: %d", rc);
        return false;
    }

    initialized = true;
    
    TT_LOG_I(TAG, "Panel initialized successfully");
    TT_LOG_I(TAG, "Native dimensions: %dx%d", epd.width(), epd.height());
    
    // Set graphics mode
    TT_LOG_I(TAG, "Setting graphics mode: %d", configuration->graphicsMode);
    epd.setMode(configuration->graphicsMode);
    
    // Set rotation if configured
    if (configuration->rotation != 0) {
        TT_LOG_I(TAG, "Setting rotation: %d degrees", configuration->rotation);
        epd.setRotation(configuration->rotation);
        TT_LOG_I(TAG, "Rotated dimensions: %dx%d", epd.width(), epd.height());
    }
    
    // Set passes
    TT_LOG_I(TAG, "Setting passes - partial: %d, full: %d", 
             configuration->partialPasses, configuration->fullPasses);
    epd.setPasses(configuration->partialPasses, configuration->fullPasses);

    // Clear the display to black
    TT_LOG_I(TAG, "Clearing display to black...");
    if (!clearWhite(false)) {
        TT_LOG_W(TAG, "Failed to clear display to black");
    } else {
        TT_LOG_I(TAG, "Display cleared to black");
    }

    return true;
}

bool FastEpdDisplay::stop() {
    if (!initialized) {
        return true;
    }

    TT_LOG_I(TAG, "Deinitializing FastEPD...");

    if (displayDriver != nullptr && displayDriver.use_count() > 1) {
        TT_LOG_W(TAG, "DisplayDriver is still in use.");
    }

    if (lvglDisplay != nullptr) {
        stopLvgl();
        lvglDisplay = nullptr;
    }
    
    // Release DisplayDriver
    displayDriver.reset();
    
    // Power off the display
    setPowerOn(false);
    
    // Deinitialize the EPD
    epd.deInit();
    
    initialized = false;
    TT_LOG_I(TAG, "FastEPD deinitialized");
    return true;
}

void FastEpdDisplay::setPowerOn(bool turnOn) {
    if (powered == turnOn) {
        return;
    }

    TT_LOG_I(TAG, "Setting EPD power: %s", turnOn ? "ON" : "OFF");
    
    // Control EPD power using FastEPD
    epd.einkPower(turnOn ? 1 : 0);
    
    powered = turnOn;
    TT_LOG_I(TAG, "EPD power %s", turnOn ? "enabled" : "disabled");
}

int FastEpdDisplay::fullUpdate(int clearMode, bool keepOn, BB_RECT* rect) {
    if (!initialized) {
        TT_LOG_E(TAG, "EPD not initialized");
        return -1;
    }

    TT_LOG_I(TAG, "Performing full update (clearMode=%d, keepOn=%d)", clearMode, keepOn);
    int result = epd.fullUpdate(clearMode, keepOn, rect);
    
    if (result != BBEP_SUCCESS) {
        TT_LOG_E(TAG, "Full update failed: %d", result);
    } else {
        TT_LOG_D(TAG, "Full update completed successfully");
    }
    
    return result;
}

int FastEpdDisplay::partialUpdate(bool keepOn, int startRow, int endRow) {
    if (!initialized) {
        TT_LOG_E(TAG, "EPD not initialized");
        return -1;
    }

    TT_LOG_I(TAG, "Performing partial update (keepOn=%d, rows=%d-%d)", keepOn, startRow, endRow);
    int result = epd.partialUpdate(keepOn, startRow, endRow);
    
    if (result != BBEP_SUCCESS) {
        TT_LOG_E(TAG, "Partial update failed: %d", result);
    } else {
        TT_LOG_D(TAG, "Partial update completed successfully");
    }
    
    return result;
}

bool FastEpdDisplay::clearWhite(bool keepOn) {
    if (!initialized) {
        TT_LOG_E(TAG, "EPD not initialized");
        return false;
    }

    TT_LOG_I(TAG, "Clearing display to white (keepOn=%d)...", keepOn);
    TT_LOG_I(TAG, "Current mode: %d, dimensions: %dx%d", 
             configuration->graphicsMode, epd.width(), epd.height());
    
    // // Fill the internal buffer with white (0xFF)
    epd.fillScreen(BBEP_WHITE);
    
    // // Perform a full update to show the white screen
    int result = epd.fullUpdate(CLEAR_WHITE, keepOn, nullptr);

    // int result = epd.clearWhite(keepOn);
    
    if (result == BBEP_SUCCESS) {
        TT_LOG_I(TAG, "Display cleared to white");
        return true;
    } else {
        TT_LOG_E(TAG, "Failed to clear to white: %d", result);
        return false;
    }
}

bool FastEpdDisplay::clearBlack(bool keepOn) {
    if (!initialized) {
        TT_LOG_E(TAG, "EPD not initialized");
        return false;
    }

    TT_LOG_I(TAG, "Clearing display to black (keepOn=%d)...", keepOn);
    TT_LOG_I(TAG, "Current mode: %d, dimensions: %dx%d", 
             configuration->graphicsMode, epd.width(), epd.height());
    
    // // Fill the internal buffer with black (0x00)
    // TT_LOG_I(TAG, "Filling screen with black...");
    epd.fillScreen(BBEP_BLACK);
    
    // // Perform a full update to show the black screen
    // TT_LOG_I(TAG, "Performing full update (CLEAR_NONE)...");
    int result = epd.fullUpdate(CLEAR_BLACK, keepOn, nullptr);

    // int result = epd.clearBlack(keepOn);
    
    if (result == BBEP_SUCCESS) {
        TT_LOG_I(TAG, "Display cleared to black successfully");
        return true;
    } else {
        TT_LOG_E(TAG, "Failed to clear to black: %d", result);
        return false;
    }
}

bool FastEpdDisplay::startLvgl() {
    if (lvglDisplay != nullptr) {
        TT_LOG_W(TAG, "LVGL display already initialized");
        return true;
    }

    if (displayDriver != nullptr && displayDriver.use_count() > 1) {
        TT_LOG_W(TAG, "DisplayDriver is still in use.");
    }

    if (!initialized) {
        TT_LOG_E(TAG, "Cannot start LVGL: EPD not initialized");
        return false;
    }

    TT_LOG_I(TAG, "Starting LVGL display...");
    
    // Determine color format based on graphics mode
    lv_color_format_t colorFormat;
    if (configuration->graphicsMode == BB_MODE_1BPP) {
        colorFormat = LV_COLOR_FORMAT_I1;  // 1-bit monochrome
        TT_LOG_I(TAG, "Using LVGL color format: I1 (1-bit monochrome)");
    } else if (configuration->graphicsMode == BB_MODE_4BPP) {
        colorFormat = LV_COLOR_FORMAT_I8;  // 8-bit grayscale (LVGL doesn't have I4)
        TT_LOG_I(TAG, "Using LVGL color format: I8 (8-bit grayscale)");
    } else {
        TT_LOG_E(TAG, "Unsupported graphics mode: %d", configuration->graphicsMode);
        return false;
    }

    int width = epd.width();
    int height = epd.height();
    
    // Calculate buffer size based on format
    size_t bufferSize;
    if (colorFormat == LV_COLOR_FORMAT_I1) {
        bufferSize = (width * height) / 8;  // 1 bit per pixel
    } else {
        bufferSize = width * height;  // 1 byte per pixel for I8
    }
    
    TT_LOG_I(TAG, "Allocating LVGL buffer: %dx%d = %zu bytes", width, height, bufferSize);
    
    // Allocate buffer in PSRAM
    void* buffer = heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
    if (buffer == nullptr) {
        TT_LOG_E(TAG, "Failed to allocate LVGL buffer (%zu bytes)", bufferSize);
        return false;
    }
    
    TT_LOG_I(TAG, "Creating LVGL display...");
    lvglDisplay = lv_display_create(width, height);
    if (lvglDisplay == nullptr) {
        TT_LOG_E(TAG, "Failed to create LVGL display");
        heap_caps_free(buffer);
        return false;
    }
    
    // Set up display buffer
    lv_display_set_color_format(lvglDisplay, colorFormat);
    lv_display_set_buffers(lvglDisplay, buffer, nullptr, bufferSize, LV_DISPLAY_RENDER_MODE_DIRECT);
    
    // Set flush callback
    lv_display_set_flush_cb(lvglDisplay, flushCallback);
    lv_display_set_user_data(lvglDisplay, this);
    
    TT_LOG_I(TAG, "LVGL display started successfully");
    return true;
}

bool FastEpdDisplay::stopLvgl() {
    if (lvglDisplay == nullptr) {
        return true;
    }

    auto touch_device = getTouchDevice();
    if (touch_device != nullptr) {
        touch_device->stopLvgl();
    }

    TT_LOG_I(TAG, "Stopping LVGL display...");
    
    // Delete the display (this will automatically free the buffers)
    lv_display_delete(lvglDisplay);
    lvglDisplay = nullptr;
    
    TT_LOG_I(TAG, "LVGL display stopped");
    return true;
}

void FastEpdDisplay::flushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* pixelMap) {
    auto* self = static_cast<FastEpdDisplay*>(lv_display_get_user_data(display));
    if (self != nullptr) {
        self->flushInternal(area, pixelMap);
    }
    lv_display_flush_ready(display);
}

void FastEpdDisplay::flushInternal(const lv_area_t* area, uint8_t* pixelMap) {
    if (!initialized) {
        return;
    }

    TT_LOG_D(TAG, "LVGL flush: area (%d,%d)-(%d,%d)", 
             area->x1, area->y1, area->x2, area->y2);
    
    int width = area->x2 - area->x1 + 1;
    int height = area->y2 - area->y1 + 1;
    
    lv_color_format_t colorFormat = lv_display_get_color_format(lvglDisplay);
    
    if (colorFormat == LV_COLOR_FORMAT_I1) {
        // 1-bit monochrome: 8 pixels per byte
        int byteWidth = (width + 7) / 8;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int byteIndex = y * byteWidth + (x / 8);
                int bitIndex = 7 - (x % 8);
                bool pixel = (pixelMap[byteIndex] & (1 << bitIndex)) != 0;
                epd.drawPixelFast(area->x1 + x, area->y1 + y, pixel ? BBEP_WHITE : BBEP_BLACK);
            }
        }
    } else if (colorFormat == LV_COLOR_FORMAT_I8) {
        // 8-bit grayscale: 1 byte per pixel
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                uint8_t gray = pixelMap[y * width + x];
                // Convert to 4-bit grayscale (0-15)
                uint8_t gray4 = gray >> 4;
                epd.drawPixelFast(area->x1 + x, area->y1 + y, gray4);
            }
        }
    }
    
    // NOTE: We do NOT trigger EPD update here!
    // E-ink updates are very slow (100ms-2s) and would block the LVGL task,
    // causing watchdog timeouts. Instead, pixels are drawn to the internal buffer,
    // and you must manually trigger updates by calling fullUpdate() or partialUpdate()
    // from your application code at appropriate times (e.g., after UI changes settle).
}

std::shared_ptr<tt::hal::display::DisplayDriver> FastEpdDisplay::getDisplayDriver() {
    assert(lvglDisplay == nullptr); // Still attached to LVGL context. Call stopLvgl() first.
    if (displayDriver == nullptr) {
        displayDriver = std::make_shared<FastEpdDisplayDriver>(
            getEpd(),
            lock,
            configuration->graphicsMode
        );
    }
    return displayDriver;
}
