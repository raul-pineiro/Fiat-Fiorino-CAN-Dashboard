#pragma once

#include <stdint.h>

/**
 * @class TripComputer
 * @brief Handles telemetric calculations including distance integration, fuel consumption, and averages.
 */
class TripComputer {
public:
    /**
     * @brief Default constructor. Initializes all metrics to zero.
     */
    TripComputer();

    /**
     * @brief Loads initial or persistent state into the computer.
     * 
     * @param total_km Odometer total kilometers.
     * @param fractional_km Fractional part of the current kilometer.
     * @param trip_km Current trip distance.
     * @param trip_time_sec Current trip elapsed time in seconds.
     * @param trip_fuel Accumulated trip fuel consumed in liters.
     * @param recent_avg_l_100km Recent average fuel consumption in liters per 100 kilometers.
     */
    void init(uint32_t total_km, float fractional_km, float trip_km, uint32_t trip_time_sec, float trip_fuel, float recent_avg_l_100km);

    /**
     * @brief Processes a time delta and updates all internal metrics.
     * 
     * @param dt_ms Delta time since last update in milliseconds.
     * @param speed_kmh Current vehicle speed in km/h.
     * @param fuel_lh Current instantaneous fuel consumption in l/h.
     * @param fuel_level Current fuel tank level (liters or percentage).
     */
    void update(uint32_t dt_ms, uint16_t speed_kmh, float fuel_lh, uint8_t fuel_level);

    /**
     * @brief Resets all trip-specific metrics to zero. Retains total odometer values.
     */
    void resetTrip();

    /**
     * @brief Retrieves the total accumulated distance (odometer).
     * @return Total distance in kilometers.
     */
    uint32_t getTotalKm() const;

    /**
     * @brief Retrieves the fractional part of the current kilometer.
     * @return Distance in kilometers (between 0.0 and 0.999...).
     */
    float getFractionalKm() const;

    /**
     * @brief Retrieves the distance covered in the current trip.
     * @return Trip distance in kilometers.
     */
    float getTripKm() const;

    /**
     * @brief Retrieves the elapsed time of the current trip.
     * @return Trip time in seconds.
     */
    uint32_t getTripTimeSec() const;

    /**
     * @brief Retrieves the total fuel consumed during the current trip.
     * @return Consumed fuel in liters.
     */
    float getTripFuelConsumed() const;

    /**
     * @brief Retrieves the average fuel consumption for the current trip.
     * @return Average consumption in liters per 100 kilometers (L/100km).
     */
    float getTripAvgL100km() const;

    /**
     * @brief Retrieves the average speed for the current trip.
     * @return Average speed in km/h.
     */
    uint16_t getTripAvgKmh() const;

    /**
     * @brief Retrieves the estimated remaining driving range.
     * @return Estimated autonomy in kilometers.
     */
    uint16_t getAutonomyKm() const;

    /**
     * @brief Retrieves the most recent average fuel consumption.
     * @return Recent average consumption in liters per 100 kilometers (L/100km).
     */
    float getRecentAvg() const;

private:
    uint32_t _total_km;             /**< Total odometer value in kilometers */
    float _fractional_km;           /**< Fractional kilometer part (0.0 to <1.0) */
    
    float _trip_km;                 /**< Distance of the current trip in kilometers */
    uint64_t _trip_time_ms;         /**< Accumulated trip time in milliseconds */
    float _trip_fuel_consumed;      /**< Total fuel consumed in the current trip in liters */
    float _trip_avg_l_100km;        /**< Calculated average consumption (L/100km) */
    uint16_t _trip_avg_kmh;         /**< Calculated average speed (km/h) */
    uint16_t _autonomy_km;          /**< Calculated remaining range in kilometers */
    float _recent_avg_l_100km;      /**< Recent average fuel consumption (L/100km) */

    float _smoothed_fuel_level;     /**< Smoothed fuel level for more stable autonomy calculations */
};