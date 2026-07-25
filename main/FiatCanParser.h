#pragma once
#include <stdint.h>

namespace FiatCAN {
    
    /**
     * @name Extended 29-bit CAN IDs
     */
    ///@{
    constexpr uint32_t ID_ENGINE_DATA = 0x04214001;
    constexpr uint32_t ID_CLUSTER_KM  = 0x0C014003;
    ///@}

    /**
     * @brief Display modes for the dashboard trip computer.
     */
    enum TripMode {
        TRIP_IDLE            = 0x00,
        TRIP_AVG_CONSUMPTION = 0xF2,
        TRIP_AVG_SPEED       = 0xF3,
        TRIP_DISTANCE        = 0xF4,
        TRIP_TRAVEL_TIME     = 0xF5
    };

    /**
     * @brief Decoded real-time engine metrics.
     */
    struct EngineData {
        uint32_t rpm;
        float injection_mm3;
        float consumption_lh;
        int temp;
        uint8_t fuel;
    };

    /**
     * @brief Decodes the total odometer distance from the cluster.
     * 
     * @param payload Raw CAN frame data payload.
     * @param dlc Data Length Code (must be >= 4).
     * @return Odometer value in kilometers, or 0 if payload is too short.
     */
    inline uint32_t parseClusterKM(const uint8_t* payload, uint8_t dlc) {
        if (dlc >= 4) {
            return (payload[1] << 16) | (payload[2] << 8) | payload[3];
        }
        return 0;
    }

    /**
     * @brief Decodes engine telemetry including RPM, fuel injection, and temperature.
     * 
     * @param payload Raw CAN frame data payload.
     * @param dlc Data Length Code (must be >= 7).
     * @return Populated EngineData structure.
     */
    inline EngineData parseEngineData(const uint8_t* payload, uint8_t dlc) {
        EngineData data = {0, 0.0f, 0.0f, 0, 0};
        
        if (dlc >= 7) {
            data.rpm = payload[6] * 32;
            
            uint16_t raw_iq = (payload[4] << 8) | payload[5];
            data.injection_mm3 = raw_iq / 100.0f;
            
            // Calculates liters/hour based on injection quantity and RPM
            data.consumption_lh = (raw_iq * data.rpm * 1.2f) / 1000000.0f; 
            
            data.temp = payload[3] - 40;
            data.fuel = payload[3];
        }
        
        return data;
    }

    /**
     * @brief Determines the active trip computer menu based on cluster messages.
     * 
     * @param payload Raw CAN frame data payload.
     * @param dlc Data Length Code (must be >= 6).
     * @return Current TripMode, defaulting to TRIP_IDLE.
     */
    inline TripMode parseTripMode(const uint8_t* payload, uint8_t dlc) {
        if (dlc >= 6) {
            switch(payload[5]) {
                case 0xF2: return TRIP_AVG_CONSUMPTION;
                case 0xF3: return TRIP_AVG_SPEED;
                case 0xF4: return TRIP_DISTANCE;
                case 0xF5: return TRIP_TRAVEL_TIME;
                default:   return TRIP_IDLE;
            }
        }
        return TRIP_IDLE;
    }
}