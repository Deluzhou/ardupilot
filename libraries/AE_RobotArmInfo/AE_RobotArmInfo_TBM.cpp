#define ALLOW_DOUBLE_MATH_FUNCTIONS


#include <AP_Logger/AP_Logger.h>
#include "AE_RobotArmInfo_TBM.h"


// perform any required initialisation of backend
void AE_RobotArmInfo_TBM::init()
{
    // update health
    _state.flags.healthy = true;

    // assign the initial value of Robot_Arm_State::_backend_state at the front end
    _state.type = (int8_t)AE_RobotArmInfo::Type::TBM;

    // initialize according to different backend types
    _cuthead_state.cutheader_height = 0;            // vertical_pos
    _cuthead_state.cutheader_horizon_pos = 0;   // horizon_pos
    _cuthead_state.cutheader_vertical_vel = 0;
    _cuthead_state.cutheader_horizon_vel = 0;
    _back_leg_state.back_support_leg_height = 0;
    _back_leg_state.back_support_leg_horizon = 0;
    _last_t_us = AP_HAL::micros64();
    _cutting_header_height_last = 0;
    _cutting_header_hor_last = 0;

    // for (uint8_t i=0; i<TBM_OIL_CYLINDER_NUM_MAX; i++) {
    //     _cuthead_state.cylinder_status[i].cylinder_name = (AE_RobotArmInfo::TBM_OC_Name)i;
    //     _cuthead_state.cylinder_status[i].length_max_mm = get_tbm_param()._cylinder_max[i];
    //     _cuthead_state.cylinder_status[i].length_mm = 0;
    //     _cuthead_state.cylinder_status[i].velocity_mms = 0;
    // }

    for(uint8_t i=0;i<CUTTING_HEADER_CYLINDER_NUM;i++)
    {
      _cuthead_state.cylinder_status[i].cylinder_name = (AE_RobotArmInfo::TBM_CH_OC_Name)i;
      _cuthead_state.cylinder_status[i].length_max_mm = get_tbm_param()._ch_cylinder_max[i];
      _cuthead_state.cylinder_status[i].length_mm = 0;
      _cuthead_state.cylinder_status[i].velocity_mms = 0;
    }

    for(uint8_t i=0;i<BACK_SUPPORT_LEG_CYLINDER_NUM;i++)
    {
      _back_leg_state.cylinder_status[i].cylinder_name = (AE_RobotArmInfo::TBM_BSL_OC_Name)i;
      _back_leg_state.cylinder_status[i].length_max_mm = get_tbm_param()._bsl_cylinder_max[i];
      _back_leg_state.cylinder_status[i].length_mm = 0;
      _back_leg_state.cylinder_status[i].velocity_mms = 0;
    }

}

// retrieve updates from inclination and ahrs sensors
void AE_RobotArmInfo_TBM::update()
{
    Inclination *inclination = AP::inclination();
    if (inclination == nullptr) {
        AP_HAL::panic("AE_RobotArmInfo_TBM should be got inclination data before calc");
        return;
    }

    AE_SlewingEncoder *slewingencoder = AE::slewingencoder();
    if(slewingencoder == nullptr){
        AP_HAL::panic("AE_RobotArmInfo_TBM should be got slewingencoder data before calc");
        return;
    }

    const AP_AHRS &ahrs = AP::ahrs();

    if (!calc_TBM_info(ahrs, inclination, slewingencoder)) {
        // update health
        _state.flags.healthy = false;
    }

    if (check_if_cutting_head_info_valid(_cuthead_state)) {
        _state.flags.healthy = true;
    }

    Write_TBM_CutheadInfo();

    return;
}

bool AE_RobotArmInfo_TBM::calc_TBM_info(const AP_AHRS &_ahrs, const Inclination *_inclination, const AE_SlewingEncoder *_slewingencoder)
{
    if (!update_TBM_cutting_header_state(_ahrs, _inclination, _slewingencoder)) {
        return false;
    }
    return true;
}

// update the cutting header state at base's body frame
// return true if update successed
bool AE_RobotArmInfo_TBM::update_TBM_cutting_header_state(const AP_AHRS &_ahrs, const Inclination *_inclination, const AE_SlewingEncoder *slewingencoder)
{
    //get this time and dt
    uint64_t t_us = AP_HAL::micros64();
    static uint8_t count = 0;
    _dt += float((t_us - _last_t_us))/1000000;

    Vector3f _euler_boom_e2b_from_sensor;
    float boom_to_body;
    float slewing_to_body;
    float angle_ACB;
    //get transformation matrix for axis change
    Matrix3f transformation;
    Matrix3f boom_matrix;

    transformation.from_euler(_ahrs.get_roll(),_ahrs.get_pitch(),radians(_inclination->yaw_deg_location(Boom) - 90));
    if(!transformation.invert())
    {
        return false;
    }
    _euler_boom_e2b_from_sensor = _inclination->get_deg_location(Boom);

    //Convert the sensor angle to the angle under the body coordinate system
    boom_matrix.from_euler(radians(_euler_boom_e2b_from_sensor.x),radians(_euler_boom_e2b_from_sensor.y),radians(_euler_boom_e2b_from_sensor.z));
    boom_matrix = transformation*boom_matrix;
    boom_matrix.to_euler(&_euler_boom_e2b_from_sensor.x,&_euler_boom_e2b_from_sensor.y,&_euler_boom_e2b_from_sensor.z);

    //calculate cutting head infomation including height,hor,cyl length,height speed,hor speed 
    //height and hor
    boom_to_body = _euler_boom_e2b_from_sensor.x + radians(get_tbm_param()._deg_BFC);

    // slewing deg while cutting head is in mid deg=0,should use information from encoder but now use inclination
    slewing_to_body = slewingencoder->get_angle_deg_diff_base2arm_loc(slewingencoder->INSTALL_SLEWING);
    _cuthead_state.cutheader_height =  get_tbm_param()._mm_CF*sinf(boom_to_body) + get_tbm_param()._mm_JL;
    _cuthead_state.cutheader_horizon_pos = sinf(slewing_to_body)*(get_tbm_param()._mm_CF*cosf(boom_to_body) + get_tbm_param()._mm_JC);

    //calculate cutting head height cyl length
    angle_ACB = radians(get_tbm_param()._deg_TCA) + radians(get_tbm_param()._deg_BCF) + boom_to_body;
    _cuthead_state.cylinder_status[0].length_mm = sqrt(get_tbm_param()._mm_AC * get_tbm_param()._mm_AC + 
                                                        get_tbm_param()._mm_BC * get_tbm_param()._mm_BC - 2*get_tbm_param()._mm_AC*get_tbm_param()._mm_BC*cos(angle_ACB));

    //calculate cutting head hor cyl length
    // _cuthead_state.cylinder_status[1].length_mm = sqrt( 2*get_tbm_param()._mm_OB*get_tbm_param()._mm_OB*(1-cos(slewing_to_body))
    // + get_tbm_param()._mm_AB*get_tbm_param()._mm_AB - 2*get_tbm_param()._mm_OB*get_tbm_param()._mm_AB*sqrt(2*(1-cos(slewing_to_body)))*
    // sin(radians(get_tbm_param()._deg_OBA)-slewing_to_body/2))-get_tbm_param()._mm_AB;

    //speed
    if(count >= 3)
    {
        _cuthead_state.cutheader_vertical_vel = (_cuthead_state.cutheader_height - _cutting_header_height_last)/_dt;
        _cuthead_state.cutheader_horizon_vel = (_cuthead_state.cutheader_horizon_pos - _cutting_header_hor_last)/_dt;
        _cuthead_state.cutheader_vertical_vel = low_pass_filter_vertical_vel.apply(_cuthead_state.cutheader_vertical_vel, _dt);
        _cuthead_state.cutheader_horizon_vel  = low_pass_filter_horizon_vel.apply(_cuthead_state.cutheader_horizon_vel, _dt);
        
        _dt = 0;
        count = 0;
        
        _cutting_header_height_last = _cuthead_state.cutheader_height;
        _cutting_header_hor_last = _cuthead_state.cutheader_horizon_pos;
    }

    //save this infomation to last infomation for next calculate
    _last_t_us = t_us;
    count++;

    return true;
}

// return true if cutting header info is valid
bool AE_RobotArmInfo_TBM::check_if_cutting_head_info_valid(struct TBM_Cutting_Header_State& cuthead_state)
{
    if (cuthead_state.cylinder_status[0].length_mm > cuthead_state.cylinder_status[0].length_max_mm) {
        return false;
    }
    return true;
}

void AE_RobotArmInfo_TBM::Write_TBM_CutheadInfo()
{
    AP::logger().Write("ARMP", "TimeUS,height,horiz,boomLength,Vvel,Hvel",
                       "smmmnn", // units: seconds, meters
                       "FBBBBB", // mult: 1e-6, 1e-2
                       "Qfffff", // format: uint64_t, float, float, float
                       AP_HAL::micros64(),
                       (float)_cuthead_state.cutheader_height,
                       (float)_cuthead_state.cutheader_horizon_pos,
                       (float)_cuthead_state.cylinder_status[0].length_mm,
                       (float)_cuthead_state.cutheader_vertical_vel,
                       (float)_cuthead_state.cutheader_horizon_vel);
}

Vector2f AE_RobotArmInfo_TBM::get_rdHeader_pos(void)
{
    return Vector2f(_cutting_header_hor_last,
                    _cutting_header_height_last);
}


