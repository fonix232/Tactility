#include "Display.hpp"

#include <Gt911Touch.h>
#include <EpdiyDisplayHelper.h>
// #include <FastEpdDisplayHelper.h>
// #include <M5GfxDisplayHelper.h>


static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note: Interrupt pin is 47
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        PAPERS3_EPD_VERTICAL_RESOLUTION,
        PAPERS3_EPD_HORIZONTAL_RESOLUTION,
        true, // swapXY
        true, // mirrorX
        false, // mirrorY
        GPIO_NUM_NC,
        GPIO_NUM_48
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();
    // EPDiy implementation - commented out for testing
    return EpdiyDisplayHelper::createM5PaperS3Display(
        touch
    );
}
