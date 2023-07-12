
#pragma once
#include <AP_AHRS/AP_AHRS.h>
#include <AP_Inclination/AP_Inclination.h>
#include <AE_SlewingEncoder/AE_SlewingEncoder.h>
#include "AE_RobotArmInfo_Backend.h"
#include "AE_RobotArmInfo.h"

// Number of Cylinders for Cutter Head Assembly. One cylinder controls the vertical, the other one controls the herizontal.
#ifndef CUTTING_HEADER_CYLINDER_NUM
#define CUTTING_HEADER_CYLINDER_NUM 2
#endif

#ifndef BACK_SUPPORT_LEG_CYLINDER_NUM
#define BACK_SUPPORT_LEG_CYLINDER_NUM 2
#endif

#ifndef TBM_OIL_CYLINDER_NUM_MAX
#define TBM_OIL_CYLINDER_NUM_MAX 4
#endif

class AE_RobotArmInfo_TBM : public AE_RobotArmInfo_Backend
{

public:
    // Constructor
    using AE_RobotArmInfo_Backend::AE_RobotArmInfo_Backend;

    // perform any required initialisation of backend
    void init() override;

    // retrieve updates from sensor
    void update() override;

    struct TBM_Cutting_Header_State {
        float cutheader_height;         //equal to the struct AE_RobotArmWP::RobotArmLocation.yvertical
        float cutheader_horizon_pos;     //equal to the struct AE_RobotArmWP::RobotArmLocation.xhorizontal
        float cutheader_vertical_vel;   //Provides for the closed-loop control of the cutting head
        float cutheader_horizon_vel;    //Provides for the closed-loop control of the cutting head
        struct AE_RobotArmInfo::TBM_CH_Cylinder_State cylinder_status[CUTTING_HEADER_CYLINDER_NUM];
    };

    struct Back_Support_Leg_State {
        float back_support_leg_height;
        float back_support_leg_horizon;
        struct AE_RobotArmInfo::TBM_BSL_Cylinder_State cylinder_status[BACK_SUPPORT_LEG_CYLINDER_NUM];
    };

    //get the cutting header state at base's body frame
    TBM_Cutting_Header_State get_TBM_cutting_header_state() const
    {
        return _cuthead_state;
    }

    //update the cutting header state at base's body frame
    // void  AP_TBM_Info::update_TBM_cutting_header_state(const Vector3f &boom_atti_deg);

private:

    float _dt;
    uint64_t _last_t_us;
    float _cutting_header_height_last;
    float _cutting_header_hor_last;

    TBM_Cutting_Header_State _cuthead_state;
    Back_Support_Leg_State _back_leg_state;

    void Write_TBM_CutheadInfo();
    

    // bool calc_TBM_info(const AP_AHRS &_ahrs, const Inclination *_inclination);
    bool calc_TBM_info(const AP_AHRS &ahrs, const Inclination *inclination, const AE_SlewingEncoder *slewingencoder);

    // update the cutting header state at base's body frame
    //bool update_TBM_back_leg_state(void);

    // update the cutting header state at base's body frame
    bool update_TBM_cutting_header_state(const AP_AHRS &ahrs, const Inclination *inclination, const AE_SlewingEncoder *slewingencoder);
    bool check_if_cutting_head_info_valid(struct TBM_Cutting_Header_State& cutting_header_state);

};
