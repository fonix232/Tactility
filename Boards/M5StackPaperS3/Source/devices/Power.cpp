#include "Power.hpp"

#include <ChargeFromAdcVoltage.h>
#include <Tactility/Log.h>
#include <esp_adc/adc_oneshot.h>

using namespace tt::hal::power;

constexpr auto* TAG = "PaperS3Power";

// M5Stack PaperS3 pin definitions
constexpr adc_channel_t VBAT_ADC_CHANNEL = ADC_CHANNEL_2;  // GPIO3 (ADC1_CHANNEL_2) - Battery voltage
constexpr adc_channel_t CHARGE_STATUS_ADC_CHANNEL = ADC_CHANNEL_3;  // GPIO4 (ADC1_CHANNEL_3) - Charge status

// Voltage divider ratio for M5Stack PaperS3
// The battery voltage is divided by 2 before reaching the ADC
constexpr float VOLTAGE_DIVIDER_MULTIPLIER = 2.0f;

// Battery voltage range for LiPo batteries
constexpr float MIN_BATTERY_VOLTAGE = 3.3f;  // Minimum safe voltage
constexpr float MAX_BATTERY_VOLTAGE = 4.2f;  // Maximum charge voltage

// Charge status voltage threshold (rises to ~0.25-0.3V when charging, ~0.01V when not charging)
constexpr int CHARGING_VOLTAGE_THRESHOLD_MV = 150;  // 0.15V threshold (midpoint between 0.01V and 0.25V)

PaperS3Power::PaperS3Power(
    std::unique_ptr<ChargeFromAdcVoltage> chargeFromAdcVoltage,
    adc_oneshot_unit_handle_t adcHandle,
    adc_channel_t chargeStatusAdcChannel
)
    : chargeFromAdcVoltage(std::move(chargeFromAdcVoltage)),
      adcHandle(adcHandle),
      chargeStatusAdcChannel(chargeStatusAdcChannel) {
}

void PaperS3Power::initializeChargeStatus() {
    if (chargeStatusInitialized) {
        return;
    }

    // Configure the charge status ADC channel
    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,  // 12dB attenuation for full voltage range
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    
    esp_err_t err = adc_oneshot_config_channel(adcHandle, chargeStatusAdcChannel, &config);
    if (err != ESP_OK) {
        TT_LOG_E(TAG, "Failed to configure charge status ADC channel %d: %s", 
                 chargeStatusAdcChannel, esp_err_to_name(err));
        return;
    }

    chargeStatusInitialized = true;
    TT_LOG_I(TAG, "Charge status ADC channel %d initialized", chargeStatusAdcChannel);
}

bool PaperS3Power::isCharging() {
    if (!chargeStatusInitialized) {
        initializeChargeStatus();
        if (!chargeStatusInitialized) {
            return false;
        }
    }

    int adc_raw = 0;
    esp_err_t ret = adc_oneshot_read(adcHandle, chargeStatusAdcChannel, &adc_raw);
    
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to read charge status ADC: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Convert raw ADC value to voltage (approximate, without calibration)
    // For 12-bit ADC with 12dB attenuation, range is roughly 0-3100mV for 0-4095
    int voltage_mv = (adc_raw * 3100) / 4095;
    
    // When charging, voltage rises to ~0.25-0.3V (250-300mV)
    // When not charging, voltage is ~0.01V (10mV)
    // Use 150mV as threshold (midpoint)
    bool charging = voltage_mv > CHARGING_VOLTAGE_THRESHOLD_MV;
    
    TT_LOG_D(TAG, "Charge status: raw=%d, voltage=%dmV, charging=%d", adc_raw, voltage_mv, charging);
    
    return charging;
}

bool PaperS3Power::supportsMetric(MetricType type) const {
    switch (type) {
        using enum MetricType;
        case IsCharging:
        case BatteryVoltage:
        case ChargeLevel:
            return true;
        default:
            return false;
    }
}

bool PaperS3Power::getMetric(MetricType type, MetricData& data) {
    switch (type) {
        using enum MetricType;
        case IsCharging:
            data.valueAsBool = isCharging();
            return true;
            
        case BatteryVoltage: {
            uint32_t voltage = 0;
            if (chargeFromAdcVoltage->readBatteryVoltageSampled(voltage)) {
                data.valueAsUint32 = voltage;
                return true;
            }
            return false;
        }
        
        case ChargeLevel: {
            uint32_t voltage = 0;
            if (chargeFromAdcVoltage->readBatteryVoltageSampled(voltage)) {
                data.valueAsUint8 = chargeFromAdcVoltage->estimateChargeLevelFromVoltage(voltage);
                return true;
            }
            return false;
        }
        
        default:
            return false;
    }
}

std::shared_ptr<PowerDevice> createPower() {
    // Configure ADC for battery voltage monitoring on GPIO3 (ADC1_CHANNEL_2)
    ChargeFromAdcVoltage::Configuration config = {
        .adcMultiplier = VOLTAGE_DIVIDER_MULTIPLIER,
        .adcRefVoltage = 3.3f,  // ESP32-S3 reference voltage
        .adcChannel = VBAT_ADC_CHANNEL,
        .adcConfig = {
            .unit_id = ADC_UNIT_1,
            .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
            .ulp_mode = ADC_ULP_MODE_DISABLE,
        },
        .adcChannelConfig = {
            .atten = ADC_ATTEN_DB_12,  // 12dB attenuation for ~3.3V range
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        },
    };

    // Create ChargeFromAdcVoltage with battery voltage range
    // This will create and own the ADC unit handle for ADC_UNIT_1
    auto chargeFromAdcVoltage = std::make_unique<ChargeFromAdcVoltage>(config, MIN_BATTERY_VOLTAGE, MAX_BATTERY_VOLTAGE);

    // Get the ADC handle from ChargeFromAdcVoltage so we can configure the charge status channel
    // on the same ADC unit (ADC_UNIT_1)
    adc_oneshot_unit_handle_t adcHandle = chargeFromAdcVoltage->getAdcHandle();
    
    if (adcHandle == nullptr) {
        TT_LOG_E(TAG, "Failed to get ADC handle from ChargeFromAdcVoltage");
    } else {
        TT_LOG_I(TAG, "Sharing ADC unit handle for battery voltage and charge status monitoring");
    }
    
    // Create PaperS3Power with shared ADC handle for both battery voltage and charge status
    return std::make_shared<PaperS3Power>(
        std::move(chargeFromAdcVoltage), 
        adcHandle,
        CHARGE_STATUS_ADC_CHANNEL
    );
}
