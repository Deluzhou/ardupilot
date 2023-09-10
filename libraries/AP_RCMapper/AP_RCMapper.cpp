#include <AP_HAL/AP_HAL.h>
#include "AP_RCMapper.h"

const AP_Param::GroupInfo RCMapper::var_info[] = {
    // @Param: ROLL
    // @DisplayName: Roll channel
    // @Description: Roll channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Roll is normally on channel 1, but you can move it to any channel with this parameter.  Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO("ROLL",        0, RCMapper, _ch_roll, 10),

    // @Param: PITCH
    // @DisplayName: Pitch channel
    // @Description: Pitch channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Pitch is normally on channel 2, but you can move it to any channel with this parameter.  Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO("PITCH",       1, RCMapper, _ch_pitch, 16),

    // @Param: THROTTLE
    // @DisplayName: Throttle channel
    // @Description: Throttle channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Throttle is normally on channel 3, but you can move it to any channel with this parameter. Warning APM 2.X: Changing the throttle channel could produce unexpected fail-safe results if connection between receiver and on-board PPM Encoder is lost. Disabling on-board PPM Encoder is recommended.  Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO("THROTTLE",    2, RCMapper, _ch_throttle, 9),

    // @Param: YAW
    // @DisplayName: Yaw channel
    // @Description: Yaw channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Yaw (also known as rudder) is normally on channel 4, but you can move it to any channel with this parameter.  Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO("YAW",         3, RCMapper, _ch_yaw, 15),

    // @Param{Rover,Sub}: FORWARD
    // @DisplayName: Forward channel
    // @Description: Forward channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Forward is normally on channel 5, but you can move it to any channel with this parameter. Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("FORWARD",    4, RCMapper, _ch_forward, 6, AP_PARAM_FRAME_SUB),

    // @Param{Rover,Sub}: LATERAL
    // @DisplayName: Lateral channel
    // @Description: Lateral channel number. This is useful when you have a RC transmitter that can't change the channel order easily. Lateral is normally on channel 6, but you can move it to any channel with this parameter. Reboot is required for changes to take effect.
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("LATERAL",    5, RCMapper, _ch_lateral, 7, AP_PARAM_FRAME_SUB),

    // @Param{Rover,Sub}: BOOM
    // @DisplayName: Boom channel
    // @Description: 
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("BOOM",    6, RCMapper, _ch_boom, 3, AP_PARAM_FRAME_ROVER ),

    // @Param{Rover,Sub}: FOREARM
    // @DisplayName: Forearm channel
    // @Description: 
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("FOREARM",    7, RCMapper, _ch_forearm, 2, AP_PARAM_FRAME_ROVER ),

    // @Param{Rover,Sub}: BUCKET
    // @DisplayName: Bucket channel
    // @Description: 
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("BUCKET",    8, RCMapper, _ch_bucket, 1, AP_PARAM_FRAME_ROVER ),

    // @Param{Rover,Sub}: ROTATION
    // @DisplayName: Rotation channel
    // @Description: 
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("ROTATION",    9, RCMapper, _ch_rotation, 4, AP_PARAM_FRAME_ROVER ),

    // @Param{Rover,Sub}: CUTTING_HEADER
    // @DisplayName: Cutting_header channel
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("CUT_HEAD",    10, RCMapper, _ch_cutting_header, 13, AP_PARAM_FRAME_ROVER ),

    // @Param{Rover,Sub}: SUPPORT_LEG
    // @DisplayName: Support_leg channel
    // @Range: 1 8
    // @Increment: 1
    // @User: Advanced
    // @RebootRequired: True
    AP_GROUPINFO_FRAME("SUP_LEG",    11, RCMapper, _ch_support_leg, 14, AP_PARAM_FRAME_ROVER ),

    AP_GROUPEND
};

// singleton instance
RCMapper *RCMapper::_singleton;

// object constructor.
RCMapper::RCMapper(void)
{
    if (_singleton != nullptr) {
        AP_HAL::panic("RCMapper must be singleton");
    }
    AP_Param::setup_object_defaults(this, var_info);
    _singleton = this;
}

RCMapper *AP::rcmap() {
    return RCMapper::get_singleton();
}
