#pragma once

#include <inttypes.h>
#include <AP_Common/AP_Common.h>
#include <AP_Param/AP_Param.h>

class RCMapper {
public:
    RCMapper();

    /* Do not allow copies */
    RCMapper(const RCMapper &other) = delete;
    RCMapper &operator=(const RCMapper&) = delete;

    // get singleton instance
    static RCMapper *get_singleton()
    {
        return _singleton;
    }

    /// roll - return input channel number for roll / aileron input
    uint8_t roll() const { return _ch_roll; }

    /// pitch - return input channel number for pitch / elevator input
    uint8_t pitch() const { return _ch_pitch; }

    /// throttle - return input channel number for throttle input
    uint8_t throttle() const { return _ch_throttle; }

    /// yaw - return input channel number for yaw / rudder input
    uint8_t yaw() const { return _ch_yaw; }

    /// forward - return input channel number for forward input
    uint8_t forward() const { return _ch_forward; }

    /// lateral - return input channel number for lateral input
    uint8_t lateral() const { return _ch_lateral; }

    /// boom - return input channel number for boom input
    uint8_t boom() const { return _ch_boom; }

    /// forearm - return input channel number for forearm input
    uint8_t forearm() const { return _ch_forearm; }

    /// bucket - return input channel number for bucket input
    uint8_t bucket() const { return _ch_bucket; }

    /// rotation - return input channel number for rotation input
    uint8_t rotation() const { return _ch_rotation; }


    static const struct AP_Param::GroupInfo var_info[];

private:
    // channel mappings
    AP_Int8 _ch_roll;
    AP_Int8 _ch_pitch;
    AP_Int8 _ch_yaw;
    AP_Int8 _ch_throttle;
    AP_Int8 _ch_forward;
    AP_Int8 _ch_lateral;
    AP_Int8 _ch_boom;
    AP_Int8 _ch_forearm;
    AP_Int8 _ch_bucket;
    AP_Int8 _ch_rotation;
    static RCMapper *_singleton;
};

namespace AP
{
RCMapper *rcmap();
};
