#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/display/DisplayDriver.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <Tactility/Lock.h>
#include <Tactility/Mutex.h>

#include <FastEPD.h>
#include <lvgl.h>
#include <memory>

class FastEpdDisplay final : public tt::hal::display::DisplayDevice {

public:

    class Configuration {
    public:
        Configuration(
            int panelType,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr,
            uint32_t busSpeed = 20000000,
            int graphicsMode = BB_MODE_4BPP,
            int rotation = 0,
            uint8_t partialPasses = 3,
            uint8_t fullPasses = 5
        ) : panelType(panelType),
            touch(std::move(touch)),
            busSpeed(busSpeed),
            graphicsMode(graphicsMode),
            rotation(rotation),
            partialPasses(partialPasses),
            fullPasses(fullPasses)
        {}

        int panelType;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        uint32_t busSpeed;
        int graphicsMode;
        int rotation;
        uint8_t partialPasses;
        uint8_t fullPasses;
    };

private:

    std::unique_ptr<Configuration> configuration;
    std::shared_ptr<tt::Mutex> lock;
    std::shared_ptr<tt::hal::display::DisplayDriver> displayDriver;
    lv_display_t* _Nullable lvglDisplay = nullptr;
    FASTEPD epd;
    bool initialized = false;
    bool powered = false;

    static void flushCallback(lv_display_t* display, const lv_area_t* area, uint8_t* pixelMap);
    void flushInternal(const lv_area_t* area, uint8_t* pixelMap);

public:

    explicit FastEpdDisplay(std::unique_ptr<Configuration> inConfiguration);
    
    ~FastEpdDisplay() override;

    std::string getName() const override { return "FastEPD"; }
    
    std::string getDescription() const override { 
        return "E-Ink display powered by FastEPD library"; 
    }

    // Device lifecycle
    bool start() override;
    bool stop() override;

    // Power control
    void setPowerOn(bool turnOn) override;
    bool isPoweredOn() const override { return powered; }
    bool supportsPowerControl() const override { return true; }

    // Touch device
    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { 
        return configuration->touch; 
    }

    // LVGL support
    bool supportsLvgl() const override { return true; }
    bool startLvgl() override;
    bool stopLvgl() override;
    lv_display_t* _Nullable getLvglDisplay() const override { return lvglDisplay; }

    // DisplayDriver support
    bool supportsDisplayDriver() const override { return true; }
    /** @return a NativeDisplay instance if this device supports it */
    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable getDisplayDriver() final;

    // EPD specific functions
    FASTEPD* getEpd() { return &epd; }
    
    /**
     * Perform a full update with optional clearing
     * @param clearMode One of CLEAR_NONE, CLEAR_FAST, CLEAR_SLOW, CLEAR_WHITE, CLEAR_BLACK
     * @param keepOn Keep the display powered after update
     * @param rect Optional rect to update (nullptr for full screen)
     */
    int fullUpdate(int clearMode = CLEAR_FAST, bool keepOn = false, BB_RECT* rect = nullptr);
    
    /**
     * Perform a partial update
     * @param keepOn Keep the display powered after update
     * @param startRow Starting row (default 0)
     * @param endRow Ending row (default full height)
     */
    int partialUpdate(bool keepOn = false, int startRow = 0, int endRow = 4095);
    
    /**
     * Clear the display to white
     * @param keepOn Keep the display powered after clear
     * @return true if successful, false otherwise
     */
    bool clearWhite(bool keepOn = false);
    
    /**
     * Clear the display to black
     * @param keepOn Keep the display powered after clear
     * @return true if successful, false otherwise
     */
    bool clearBlack(bool keepOn = false);
};
