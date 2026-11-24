/*
 * measurement_data.h
 *
 *  Created on: Nov 23, 2026
 *      Author: Ana
 */


#ifndef MEASUREMENT_DATA_H
#define MEASUREMENT_DATA_H

#include <stdint.h>

/* PV monitoring states */
typedef enum //The compiler assigns integer values automatically starting from 0 which was explicitly set
{
    PV_STATE_STARTUP = 0U, // 0, System has started, but no valid monitoring result is available yet
    PV_STATE_NORMAL,       // 1, Measurements are valid and output is within the configured operating range
    PV_STATE_LOW_OUTPUT,   // 2, Output has remained below the configured threshold for the required persistence period
    PV_STATE_SENSOR_ERROR  // 3, Sensor acquisition or measurement validation has failed
} PV_State_t;
// Used for a state-machine

/* Complete data from one acquisition cycle */
typedef struct
{
    uint32_t sequence_number; // ID
    uint32_t timestamp_ms;    // Acquisition time

    uint32_t voltage_mV; //
    uint32_t current_mA;
    uint32_t power_mW;

    PV_State_t fault_state;   // communicates the local detection result

    uint8_t data_valid;       // distinguishes a valid measurement from an unavailable or invalid one

} MeasurementSnapshot_t; // The Measurement struct used between the two tasks

#endif /* MEASUREMENT_DATA_H */
 /* INC_MEASUREMENT_DATA_H_ */
