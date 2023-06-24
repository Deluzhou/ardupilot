#include "AE_RobotArmInfo_Excavator.h"
#include <AP_Inclination/AP_Inclination.h>
#include <AP_AHRS/AP_AHRS.h>

// perform any required initialisation of backend
void AE_RobotArmInfo_Excavator::init()
{
    // update health
    _state.flags.healthy = true;
    // 在这里把前端的Robot_Arm_State::_backend_state赋初值，
    // 因为前端的状态量是为了给挖掘机/掘进机不同的后端使用的，所以包含的内容比较多，进入不同的后端需要根据不同后端类型进行相应初始化
    _state.type = (int8_t)AE_RobotArmInfo::Type::EXCAVATOR;

    ex_info.component = AE_RobotArmInfo::Component_name::EXCAVATOR_ARM;
    ex_info.boom_tip_pos_body = {0,0,0};
    ex_info.forearm_tip_pos_body = {0,0,0};
    ex_info.bucket_tip_pos_body = {0,0,0};
    ex_info.rbt_arm_yaw_body = 0;
    for (uint8_t i=0; i<OIL_CYLINDER_NUM_MAX; i++) {
        ex_info.cylinder_status[i].cylinder_name = (AE_RobotArmInfo::Ex_OC_Name)i;
        ex_info.cylinder_status[i].length_max_mm = 0;
        ex_info.cylinder_status[i].length_mm = 0;
        ex_info.cylinder_status[i].velocity_mms = 0;
    }
}

// retrieve updates from sensor
void AE_RobotArmInfo_Excavator::update()
{
    Inclination *inclination = AP::inclination();
    if (inclination == nullptr) {
        return;
    }
    const AP_AHRS &ahrs = AP::ahrs();
    ex_info.rbt_arm_yaw_body = ahrs.get_yaw();
    if (!calc_excavator_info()) {
        // update health
        _state.flags.healthy = false;
    }

    //计算完成后把挖掘机类的私有变量赋给结构体供外界调用
    if (check_if_info_valid(ex_info)) {
        _state.flags.healthy = true;
    }

    return;
}

bool AE_RobotArmInfo_Excavator::check_if_info_valid(struct Excavator_Robot_Arm_State& ex_state)
{
    //consider calculated data is valid?
    // if(ex_state.cylinder_status->length_mm > ex_state.cylinder_status->length_max_mm)
    return false;
}

// return false if the excavator information has not been calculated exactly.
bool AE_RobotArmInfo_Excavator::calc_excavator_info()
{
    //
    // if(!calc_bucket_position() && !calc_oil_sylinder_length());
    return true;
}

bool AE_RobotArmInfo_Excavator::calc_oil_sylinder_length()
{
    //
    return true;
}

// return false if the position isn't calculated.
// Calculate the three-dimensional coordinates of the tooth tip relative to the body
bool AE_RobotArmInfo_Excavator::calc_bucket_position()
{
    //在这里写你的算法,想要获取什么值，跟下面的方法类似get_ex_param()._mm_QV
    ex_info.boom_tip_pos_body.x = get_ex_param()._mm_QV * cosf(1);
    return true;
}

//update the cutting header state at base's body frame
// void AP_TBM_Info::update_TBM_cutting_header_state(const Vector3f &boom_atti_deg)
// {
//     float roll_rad = boom_atti_deg.x*M_PI/180;
//     _cutting_header_state.cutting_header_height =  DISTANCE_CF * sinf(roll_rad + ANGLE_BOOM_UP) + DISTANCE_SLEWING_TO_GROUND;
// }






