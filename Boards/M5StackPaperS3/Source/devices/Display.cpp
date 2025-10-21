#include "Display.hpp"

#include <Gt911Touch.h>
#include <FastEpdDisplayHelper.h>


static std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() {
    // Note: Interrupt pin is 47
    auto configuration = std::make_unique<Gt911Touch::Configuration>(
        I2C_NUM_0,
        PAPERS3_EPD_VERTICAL_RESOLUTION,
        PAPERS3_EPD_HORIZONTAL_RESOLUTION,
        true,
        false,
        false,
        GPIO_NUM_NC,
        GPIO_NUM_48
    );

    return std::make_shared<Gt911Touch>(std::move(configuration));
}

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    auto touch = createTouch();

    auto configuration = std::make_unique<FastEPDDisplay::Configuration>(
        touch
    );

    auto display = std::make_shared<FastEPDDisplay>(std::move(configuration));
    return std::reinterpret_pointer_cast<tt::hal::display::DisplayDevice>(display);

    // FastEPD implementation
    return FastEpdDisplayHelper::createM5PaperS3Display(touch);
}
