#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/touch/TouchDevice.h>

#include <Tactility/Check.h>
#include <Tactility/LogEsp.h>



// Display
#define PAPERS3_EPD_HORIZONTAL_RESOLUTION 540
#define PAPERS3_EPD_VERTICAL_RESOLUTION 960
#define PAPERS3_EPD_DRAW_BUFFER_HEIGHT (PAPERS3_EPD_VERTICAL_RESOLUTION / 10)
#define PAPERS3_EPD_DRAW_BUFFER_SIZE (PAPERS3_EPD_HORIZONTAL_RESOLUTION * PAPERS3_EPD_DRAW_BUFFER_HEIGHT)

class FastEPDDisplay final : public tt::hal::display::DisplayDevice {

public:
    class Configuration {
    public:
        Configuration(
            std::shared_ptr<tt::hal::touch::TouchDevice> touch
        ) : touch(std::move(touch))
        {
        // Additional initialization if needed
        }

         std::shared_ptr<tt::hal::touch::TouchDevice> touch;
    };
private:
    std::unique_ptr<Configuration> configuration;

public:
    explicit FastEPDDisplay(std::unique_ptr<Configuration> inConfiguration)
        : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }


    std::string getName() const override { return "FastEPD Display"; }
    std::string getDescription() const override { return ""; }

    bool start() override { return true; }
    bool stop() override { tt_crash("Not supported"); }

    bool supportsLvgl() const override { return false; }
    bool startLvgl() override { return true; }
    bool stopLvgl() override { tt_crash("Not supported"); }
    lv_display_t* _Nullable getLvglDisplay() const override { return nullptr; }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { return configuration->touch; }

    bool supportsDisplayDriver() const override { return false; }
    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable getDisplayDriver() override { return nullptr; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
