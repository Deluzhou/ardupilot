#define ALLOW_DOUBLE_MATH_FUNCTIONS

#include <AP_Inclination/AP_Inclination.h>
#include <AP_AHRS/AP_AHRS.h>
#include <AP_Logger/AP_Logger.h>
#include "AE_RobotArmInfo_Excavator.h"



// perform any required initialisation of backend
void AE_RobotArmInfo_Excavator::init()
{
    // update health
    _state.flags.healthy = true;

    //init param Robot_Arm_State::_backend_state
    _state.type = (int8_t)AE_RobotArmInfo::Type::EXCAVATOR;

    ex_info.component = AE_RobotArmInfo::Component_name::EXCAVATOR_ARM;
    ex_info.boom_tip_pos_body = {0,0,0};
    ex_info.forearm_tip_pos_body = {0,0,0};
    ex_info.bucket_tip_pos_body = {0,0,0};
    ex_info.rbt_arm_yaw_body = 0;
    for (uint8_t i=0; i<OIL_CYLINDER_NUM_MAX; i++) {
        ex_info.cylinder_status[i].cylinder_name = (AE_RobotArmInfo::Ex_OC_Name)i;
        ex_info.cylinder_status[i].length_max_mm = get_ex_param()._cylinder_max[i];
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
    if (!calc_excavator_info(ahrs, inclination)) {
        // update health
        _state.flags.healthy = false;
    }

    if (check_if_info_valid(ex_info)) {
        _state.flags.healthy = true;
    }

    Write_Excavator_ArmInfo();

    return;
}

bool AE_RobotArmInfo_Excavator::check_if_info_valid(struct Excavator_Robot_Arm_State& ex_state)
{
    //consider calculated data is valid
    if(ex_state.cylinder_status[0].length_mm > ex_state.cylinder_status[0].length_max_mm ||
        ex_state.cylinder_status[1].length_mm > ex_state.cylinder_status[1].length_max_mm ||
        ex_state.cylinder_status[2].length_mm > ex_state.cylinder_status[2].length_max_mm)
    {
      return false;
    }
    return true;
}

// return false if the excavator information has not been calculated exactly.
bool AE_RobotArmInfo_Excavator::calc_excavator_info(const AP_AHRS &_ahrs, const Inclination *_inclination)
{
    Matrix3f transformation;
    Matrix3f boom_matrix;
    Matrix3f forearm_matrix;
    Matrix3f bucket_matrix;
    Vector3f _euler_boom_e2b_from_sensor;
    Vector3f _euler_forearm_e2b_from_sensor;
    Vector3f _euler_bucket_e2b_from_sensor;
    float boom_to_body;
    float forearm_to_body;
    float bucket_to_body;
    float slewing_to_body;
    float boom_to_slewing;
    float forearm_to_boom;
    float bucket_to_forearm;
    
    transformation.from_euler(_ahrs.get_roll(),_ahrs.get_pitch(),radians(_inclination->yaw_deg_location(Boom)));
    if(!transformation.invert())
    {
        return false;
    }
    _euler_boom_e2b_from_sensor     = _inclination->get_deg_location(Boom);
    _euler_forearm_e2b_from_sensor  = _inclination->get_deg_location(Forearm);
    _euler_bucket_e2b_from_sensor   = - _inclination->get_deg_location(Bucket);

    boom_matrix.from_euler(radians(_euler_boom_e2b_from_sensor.x),radians(_euler_boom_e2b_from_sensor.y),radians(_euler_boom_e2b_from_sensor.z));
    forearm_matrix.from_euler(radians(_euler_forearm_e2b_from_sensor.x),radians(_euler_forearm_e2b_from_sensor.y),radians(_euler_forearm_e2b_from_sensor.z));
    bucket_matrix.from_euler(radians(_euler_bucket_e2b_from_sensor.x),radians(_euler_bucket_e2b_from_sensor.y),radians(_euler_bucket_e2b_from_sensor.z));
    
    boom_matrix = transformation*boom_matrix;
    forearm_matrix = transformation*forearm_matrix;
    bucket_matrix = transformation*bucket_matrix;

    boom_matrix.to_euler(&_euler_boom_e2b_from_sensor.x,&_euler_boom_e2b_from_sensor.y,&_euler_boom_e2b_from_sensor.z);
    forearm_matrix.to_euler(&_euler_forearm_e2b_from_sensor.x,&_euler_forearm_e2b_from_sensor.y,&_euler_forearm_e2b_from_sensor.z);
    bucket_matrix.to_euler(&_euler_bucket_e2b_from_sensor.x,&_euler_bucket_e2b_from_sensor.y,&_euler_bucket_e2b_from_sensor.z);

    slewing_to_body = radians(_inclination->yaw_deg_location(Boom));
    adjust_to_body_origin(_euler_boom_e2b_from_sensor.y,_euler_forearm_e2b_from_sensor.y,_euler_bucket_e2b_from_sensor.y,boom_to_body,forearm_to_body,bucket_to_body);
    boom_to_slewing = boom_to_body;
    forearm_to_boom = forearm_to_body - boom_to_slewing;
    bucket_to_forearm = bucket_to_body - forearm_to_boom - boom_to_slewing;
 
    calc_bucket_position(boom_to_body,forearm_to_body,bucket_to_body,slewing_to_body);
    calc_oil_cylinder_length(boom_to_slewing,forearm_to_boom,bucket_to_forearm);
    return true;
}

void AE_RobotArmInfo_Excavator::calc_oil_cylinder_length(float boom_to_slewing,float forearm_to_boom,float bucket_to_forearm)
{
    float angle_ACB;
    float phi;
    float angle_KQN;
    float distance_NK;
    float angle_MNK;
    float angle_QNK;
    float angle_GNM;
    angle_ACB = radians(get_ex_param()._deg_TCA) + radians(get_ex_param()._deg_BCF) + boom_to_slewing;
    ex_info.cylinder_status[0].length_mm = sqrt(get_ex_param()._mm_AC * get_ex_param()._mm_AC + get_ex_param()._mm_BC * get_ex_param()._mm_BC - 2*get_ex_param()._mm_AC*get_ex_param()._mm_BC*cos(angle_ACB));
    phi = (M_PI - radians(get_ex_param()._deg_DFC) -radians(get_ex_param()._deg_QFG) -radians(get_ex_param()._deg_GFE) - forearm_to_boom);
    ex_info.cylinder_status[1].length_mm = sqrt(get_ex_param()._mm_DF * get_ex_param()._mm_DF + get_ex_param()._mm_EF*get_ex_param()._mm_EF -2*get_ex_param()._mm_EF*get_ex_param()._mm_DF*cos(phi));
    angle_KQN = M_PI - radians(get_ex_param()._deg_NQF) -bucket_to_forearm -radians(get_ex_param()._deg_KQV);
    distance_NK = sqrt(get_ex_param()._mm_QN*get_ex_param()._mm_QN + get_ex_param()._mm_QK *get_ex_param()._mm_QK
                    - 2*get_ex_param()._mm_QK*get_ex_param()._mm_QN*cos(angle_KQN));
    angle_MNK = acos((get_ex_param()._mm_MN *get_ex_param()._mm_MN + distance_NK*distance_NK
                        - get_ex_param()._mm_MK *get_ex_param()._mm_MK)/(2*get_ex_param()._mm_MN*distance_NK));
    angle_QNK = asin(get_ex_param()._mm_QK*sin(angle_KQN)/distance_NK);
    angle_GNM = M_2_PI - radians(get_ex_param()._deg_GNF) - radians(get_ex_param()._deg_GNQ) - angle_MNK - angle_QNK;
    ex_info.cylinder_status[2].length_mm = sqrt(get_ex_param()._mm_GN *get_ex_param()._mm_GN + get_ex_param()._mm_MN*get_ex_param()._mm_MN 
                                            - 2*get_ex_param()._mm_MN*get_ex_param()._mm_GN*cos(angle_GNM));
}

 // return false if the position isn't calculated.
 // Calculate the three-dimensional coordinates of the tooth tip relative to the body
void AE_RobotArmInfo_Excavator::calc_bucket_position(float boom,float forearm,float bucket,float slewing)
{
    //在这里写你的算法,想要获取什么值，跟下面的方法类似get_ex_param()._mm_QV
    ex_info.bucket_tip_pos_body.x = cosf(slewing)*(get_ex_param()._mm_QV*cosf(bucket) + get_ex_param()._mm_FQ*cosf(forearm)
    + get_ex_param()._mm_CF*cosf(boom) + get_ex_param()._mm_JC);
    ex_info.bucket_tip_pos_body.y = sinf(slewing)*(get_ex_param()._mm_QV*cosf(bucket) + get_ex_param()._mm_FQ*cosf(forearm)
    + get_ex_param()._mm_CF*cosf(boom) + get_ex_param()._mm_JC);
    ex_info.bucket_tip_pos_body.z = get_ex_param()._mm_QV*sinf(bucket) + get_ex_param()._mm_FQ*sinf(forearm)
    + get_ex_param()._mm_CF*sinf(boom) + get_ex_param()._mm_JL;
}

void AE_RobotArmInfo_Excavator::adjust_to_body_origin(float euler_boom, float euler_forearm, float euler_bucket,
                                                        float &boom_to_body,float &forearm_to_body,float &bucket_to_body)
{
    float angle_GNM;
    float angle_QNM;
    float distance_QM;
    float angle_MQN;
    float angle_MQK;
    float angle_DQV;

    boom_to_body = euler_boom + radians(get_ex_param()._deg_BFC);
    forearm_to_body = euler_forearm;
    angle_GNM = M_PI + forearm_to_body - radians(get_ex_param()._deg_GNF) - euler_bucket;
    angle_QNM = M_2PI - radians(get_ex_param()._deg_FNQ) - radians(get_ex_param()._deg_GNF) - angle_GNM;
    distance_QM = sqrt(get_ex_param()._mm_QN * get_ex_param()._mm_QN + get_ex_param()._mm_MN * get_ex_param()._mm_MN
                    - 2 * get_ex_param()._mm_QN * get_ex_param()._mm_MN * cosf(angle_QNM));
    angle_MQN = acosf((distance_QM * distance_QM + get_ex_param()._mm_QN * get_ex_param()._mm_QN - get_ex_param()._mm_MN * get_ex_param()._mm_MN)
                    / (2 * distance_QM * get_ex_param()._mm_QN));
    angle_MQK = acosf((get_ex_param()._mm_QK * get_ex_param()._mm_QK + distance_QM * distance_QM - get_ex_param()._mm_MK * get_ex_param()._mm_MK)
                    / (2 * get_ex_param()._mm_QK * get_ex_param()._mm_MK));
    angle_DQV = M_2PI - radians(get_ex_param()._deg_NQF) - angle_MQN - angle_MQK - radians(get_ex_param()._deg_KQV);
    bucket_to_body = forearm_to_body + angle_DQV - M_PI;

    if(bucket_to_body < (-2*M_PI)){
        bucket_to_body += 2*M_PI;
    }
    if(bucket_to_body > 2*M_PI){
        bucket_to_body -= 2*M_PI;
    }
} 

void AE_RobotArmInfo_Excavator::Write_Excavator_ArmInfo()
{
    AP::logger().Write("ARMP", "TimeUS,tip_x,tip_y,tip_z,boom_cyl,forearm_cyl,bucket_cyl",
                "sm", // units: seconds, meters
                "FB", // mult: 1e-6, 1e-2
                "Qffffff", // format: uint64_t, float, float, float
                AP_HAL::micros64(),
                (float)ex_info.bucket_tip_pos_body.x,
                (float)ex_info.bucket_tip_pos_body.y,
                (float)ex_info.bucket_tip_pos_body.z,
                (float)ex_info.cylinder_status[0].length_mm,
                (float)ex_info.cylinder_status[1].length_mm,
                (float)ex_info.cylinder_status[2].length_mm);
}



