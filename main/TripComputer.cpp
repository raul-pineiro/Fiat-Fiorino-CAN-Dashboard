#include "TripComputer.h"

TripComputer::TripComputer() : 
    _total_km(0), _fractional_km(0.0f), _trip_km(0.0f), 
    _trip_time_ms(0), _trip_fuel_consumed(0.0f), 
    _trip_avg_l_100km(0.0f), _trip_avg_kmh(0), _smoothed_fuel_level(-1.0f) {
}

void TripComputer::init(uint32_t total_km, float fractional_km, float trip_km, uint32_t trip_time_sec, float trip_fuel, float recent_avg_l_100km) {
    _total_km = total_km;
    _fractional_km = fractional_km;
    _trip_km = trip_km;
    _trip_time_ms = (uint64_t)trip_time_sec * 1000;
    _trip_fuel_consumed = trip_fuel;
    _trip_avg_l_100km = recent_avg_l_100km;
}

void TripComputer::resetTrip() {
    _trip_km = 0.0f;
    _trip_time_ms = 0;
    _trip_fuel_consumed = 0.0f;
    _trip_avg_l_100km = 0.0f;
    _trip_avg_kmh = 0;
}

void TripComputer::update(uint32_t dt_ms, float speed_kmh, float fuel_lh, uint8_t fuel_level) {
    if (dt_ms == 0) return;

    double dt_hours = (double)dt_ms / (1000.0 * 3600.0);
    _trip_time_ms += dt_ms;
    uint32_t trip_time_sec = _trip_time_ms / 1000;

    if (speed_kmh > 0) {
        double dist_km = speed_kmh * dt_hours;
        
        _fractional_km += dist_km;
        if (_fractional_km >= 1.0f) {
            uint32_t kms_traveled = (uint32_t)_fractional_km;
            _total_km += kms_traveled;
            _fractional_km -= kms_traveled;
        }

        _trip_km += dist_km;
    }

    if (fuel_lh > 0.0f) {
        _trip_fuel_consumed += (fuel_lh * dt_hours);
    }

    if (trip_time_sec > 0) {
        double trip_hours = (double)trip_time_sec / 3600.0;
        _trip_avg_kmh = (uint16_t)(_trip_km / trip_hours);
    }

    if (_trip_km > 0.1f) {
        _trip_avg_l_100km = (_trip_fuel_consumed / _trip_km) * 100.0f;
    }

    float instant_l_100km = 99.9f;
    if (speed_kmh > 3) {
        instant_l_100km = (fuel_lh / (float)speed_kmh) * 100.0f;

        if (_recent_avg_l_100km <= 0.0f) {
            _recent_avg_l_100km = instant_l_100km;
        } else {
            _recent_avg_l_100km = (_recent_avg_l_100km * 0.99f) + (instant_l_100km * 0.01f);
        }
    }
    if (_smoothed_fuel_level < 0.0f) {
        _smoothed_fuel_level = (float)fuel_level;
    } else {
        _smoothed_fuel_level = (_smoothed_fuel_level * 0.95f) + ((float)fuel_level * 0.05f);
    }
}

uint32_t TripComputer::getTotalKm() const { return _total_km; }
float TripComputer::getFractionalKm() const { return _fractional_km; }
float TripComputer::getTripKm() const { return _trip_km; }
uint32_t TripComputer::getTripTimeSec() const { return _trip_time_ms / 1000; }
float TripComputer::getTripFuelConsumed() const { return _trip_fuel_consumed; }
float TripComputer::getTripAvgL100km() const { return _trip_avg_l_100km; }
uint16_t TripComputer::getTripAvgKmh() const { return _trip_avg_kmh; }
float TripComputer::getRecentAvg() const { return _trip_avg_l_100km; }