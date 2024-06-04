#pragma once

#include "AP_Inclination.h"
#include "AP_Inclination_Backend_Serial.h"

/*
 Limited by the number of serial ports, we use a three-in-one inclination sensor.
 It means that we integrate the data of the three inclination sensors through another circuit board,
 and then send them to pixhawk together.
*/
class AP_Inclination_3HDAs_binary_Serial : public AP_Inclination_Backend_Serial
{

public:

    using AP_Inclination_Backend_Serial::AP_Inclination_Backend_Serial;
    void init_serial(uint8_t serial_instance) override;
private:
    // int test_count;
    // get a reading,Vector3f's first value is boom, second is forearm, third is bucket.
    // bool get_reading(Vector3f &reading_roll_deg, Vector3f &reading_pitch_deg, Vector3f &reading_yaw_deg, InstallLocation location) override;
    bool get_reading(Vector3f &reading_roll_deg, Vector3f &reading_pitch_deg, Vector3f &reading_yaw_deg, Vector3f &read_velocity_rad, InstallLocation location) override;
    uint8_t  linebuf[53];
    uint8_t  linebuf_len;
};


//Convert 4-byte data stored in little endian to float type
typedef union
{
    float f;
    unsigned long ul;
}FloatuLong_t;

