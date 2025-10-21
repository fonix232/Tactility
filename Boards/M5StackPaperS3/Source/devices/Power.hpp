#pragma once

#include <memory>
#include <esp_adc/adc_oneshot.h>
#include <ChargeFromAdcVoltage.h>
#include <Tactility/hal/power/PowerDevice.h>

namespace tt::hal::power {

/**
 * @brief Power management for M5Stack PaperS3
 * 
 * Provides battery voltage monitoring and charge detection.
 * - Battery voltage: GPIO3 (ADC1_CHANNEL_2) with 2x voltage divider
 * - Charge status: GPIO4 (ADC1_CHANNEL_3) - rises to ~0.25-0.3V when charging, ~0.01V when not charging
 */
class PaperS3Power : public PowerDevice {
private:
    std::unique_ptr<::ChargeFromAdcVoltage> chargeFromAdcVoltage;
    adc_oneshot_unit_handle_t adcHandle;
    adc_channel_t chargeStatusAdcChannel;
    bool chargeStatusInitialized = false;

public:
    explicit PaperS3Power(
        std::unique_ptr<::ChargeFromAdcVoltage> chargeFromAdcVoltage,
        adc_oneshot_unit_handle_t adcHandle,
        adc_channel_t chargeStatusAdcChannel
    );
    ~PaperS3Power() override = default;

    std::string getName() const override { return "M5Stack PaperS3 Power"; }
    std::string getDescription() const override { return "Battery monitoring with charge detection"; }

    bool supportsMetric(MetricType type) const override;
    bool getMetric(MetricType type, MetricData& data) override;

private:
    void initializeChargeStatus();
    bool isCharging();
};

} // namespace tt::hal::power

std::shared_ptr<tt::hal::power::PowerDevice> createPower();
