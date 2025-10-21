#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/touch/TouchDevice.h>

// Display
#define PAPERS3_EPD_VERTICAL_RESOLUTION 540
#define PAPERS3_EPD_HORIZONTAL_RESOLUTION 960
#define PAPERS3_EPD_DRAW_BUFFER_HEIGHT (PAPERS3_EPD_VERTICAL_RESOLUTION / 10)
#define PAPERS3_EPD_DRAW_BUFFER_SIZE (PAPERS3_EPD_HORIZONTAL_RESOLUTION * PAPERS3_EPD_DRAW_BUFFER_HEIGHT)

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
