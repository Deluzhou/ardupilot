#include "SIM_TBM.h"
#include <AP_Math/matrix3.h>

namespace SITL {

#define    Rotation_Channel     9
#define    Boom_Channel         11
#define    Support_Leg_Channel  10
#define    Sprocket_Channel     8

#define TBM_SIM_Debug

#ifdef TBM_SIM_Debug
#define Debug(fmt, args ...)  do {::fprintf(stderr, "%s:%d: " fmt "\n", __FUNCTION__, __LINE__, ## args); } while(0)
#else
#define Debug(fmt, args ...)
#endif

void SimTBM::init()
{
    if(!get_tbm_info()) {
        return;
    }

    AE_RobotArmInfo::TBM_PARAM tbm_params = _armInfo_backend->get_tbm_param();
    Vector3f _euler_boom_e2b_from_sensor;
    Matrix3f dbodyrbody;
    Matrix3f dboomrboom;

    dbodyrbody.from_euler(ahrs.get_roll(), ahrs.get_pitch(), ahrs.get_yaw());
    
    Matrix3f dboomrdbody;
    dboomrdbody.from_euler(0, 0, radians(-90));
    Matrix3f dboomrbody  = dboomrdbody*dbodyrbody;
    
    // bodyrdboom
    if(!dboomrbody.invert())
    {
        return;
    }
    _euler_boom_e2b_from_sensor.x = sitl->state.inclination_state.roll_deg.x;
    _euler_boom_e2b_from_sensor.y = sitl->state.inclination_state.pitch_deg.x;
    _euler_boom_e2b_from_sensor.z = sitl->state.inclination_state.yaw_deg.x;

    dboomrboom.from_euler(radians(_euler_boom_e2b_from_sensor.x),radians(_euler_boom_e2b_from_sensor.y),radians(_euler_boom_e2b_from_sensor.z));
    // bodyrboom = bodyrdboom * dboomrboom;
    dboomrboom = dboomrbody*dboomrboom;
    // bodyrboom
    dboomrboom.to_euler(&_euler_boom_e2b_from_sensor.x,&_euler_boom_e2b_from_sensor.y,&_euler_boom_e2b_from_sensor.z);

    boom_cylinder_length = sqrt(tbm_params._mm_AC * tbm_params._mm_AC + tbm_params._mm_BC * tbm_params._mm_BC -  \
                            2*tbm_params._mm_AC*tbm_params._mm_BC * cos(radians(tbm_params._deg_TCA) + \
                            radians(tbm_params._deg_BCF) + radians(tbm_params._deg_BFC) + _euler_boom_e2b_from_sensor.x));

    rotation_rad = _euler_boom_e2b_from_sensor.z;

    sprocket_rotation_speed = 0;
    support_leg_cylinder_length = 210; // 175 ~ 245

    is_init = true;
}

/* update model by one time step */
void SimTBM::update(const struct sitl_input &input)
{
    if(!get_tbm_info()) {
        return;
    }

    if(!is_init) {
        init();
    }

    calc_speed(input.servos[Rotation_Channel-1], input.servos[Boom_Channel-1], \
               input.servos[Support_Leg_Channel-1], input.servos[Sprocket_Channel-1]);

    // how much time has passed?
    float delta_time = frame_time_us * 1.0e-6f;

    AE_RobotArmInfo::TBM_PARAM tbm_params = _armInfo_backend->get_tbm_param();

    Debug("pwm_boom:%d, pwm_rotation:%d", input.servos[Boom_Channel-1], input.servos[Rotation_Channel-1]);
    Debug("pwm_support_leg:%d, pwm_sprocket:%d", input.servos[Support_Leg_Channel-1], input.servos[Sprocket_Channel-1]);
    Debug("boom_speed:%f, rotation_speed:%f", boom_speed, rotation_speed);
    Debug("sprocket_speed:%f", sprocket_rotation_speed);
    Debug("support_leg_cylinder_length:%f", support_leg_cylinder_length);

    boom_cylinder_length += boom_speed * delta_time;
    rotation_rad += rotation_speed * delta_time;

    support_leg_cylinder_length += support_leg_speed * delta_time;
    constrain_float(support_leg_cylinder_length, 175, 245);
    sitl->state.tbm_state.support_leg_rad = (support_leg_cylinder_length -175) / 70 * M_PI_2;

    Debug("boom_cylinder_length:%f", boom_cylinder_length);

    float theta2 = acosf( (powf(tbm_params._mm_AC, 2) + powf(tbm_params._mm_BC, 2) - powf(boom_cylinder_length, 2)) \
                        / (2*tbm_params._mm_AC*tbm_params._mm_BC)) - radians(tbm_params._deg_BCF) - radians(tbm_params._deg_TCA);

    float roll_to_body = theta2 - radians(tbm_params._deg_BFC);

    Matrix3f bodyrboom;

    bodyrboom.from_euler(roll_to_body, 0, rotation_rad);

    Debug("b_roll:%f,b_pitch:%f,b_yaw:%f", degrees(roll_to_body), 0.0, degrees(rotation_rad));

    Matrix3f dbodyrbody;
    dbodyrbody.from_euler(ahrs.get_roll(), ahrs.get_pitch(), ahrs.get_yaw());
    Debug("apm_roll:%f, apm_pitch:%f, apm_yaw:%f", degrees(ahrs.get_roll()), degrees(ahrs.get_pitch()), degrees(ahrs.get_yaw()));

    Matrix3f dboomrdbody;
    dboomrdbody.from_euler(0, 0, radians(-90));

    Matrix3f dboomrboom;
    dboomrboom = dboomrdbody*dbodyrbody*bodyrboom;

    dboomrboom.to_euler(&sitl->state.inclination_state.roll_deg.x, &sitl->state.inclination_state.pitch_deg.x, \
                        &sitl->state.inclination_state.yaw_deg.x);
    
    sitl->state.inclination_state.roll_deg.x = degrees(sitl->state.inclination_state.roll_deg.x);
    sitl->state.inclination_state.pitch_deg.x = degrees(sitl->state.inclination_state.pitch_deg.x);
    sitl->state.inclination_state.yaw_deg.x = degrees(sitl->state.inclination_state.yaw_deg.x);

    Debug("incli_roll:%f, incli_pitch:%f, incli_yaw:%f",sitl->state.inclination_state.roll_deg.x, sitl->state.inclination_state.pitch_deg.x, sitl->state.inclination_state.yaw_deg.x);

    // update tbm state
    sitl->state.tbm_state.rdheader_yb =  tbm_params._mm_CF*sinf(theta2) + tbm_params._mm_JL;
    sitl->state.tbm_state.rdheader_xb = sinf(rotation_rad)*(tbm_params._mm_CF*cosf(theta2) + tbm_params._mm_JC);
    sitl->state.tbm_state.rdheader_zb = cosf(rotation_rad)*(tbm_params._mm_CF*cosf(theta2) + tbm_params._mm_JC);

    Debug("x:%f, y:%f, z:%f", sitl->state.tbm_state.rdheader_xb, sitl->state.tbm_state.rdheader_yb, sitl->state.tbm_state.rdheader_zb);

    float angle_ACB = radians(tbm_params._deg_TCA) + radians(tbm_params._deg_BCF) + theta2;
    sitl->state.tbm_state.boom_cylinder_L = sqrt(tbm_params._mm_AC * tbm_params._mm_AC + 
                                                        tbm_params._mm_BC * tbm_params._mm_BC - 2*tbm_params._mm_AC*tbm_params._mm_BC*cos(angle_ACB));

    sitl->state.tbm_state.roll_b = roll_to_body;
    sitl->state.tbm_state.pitch_b = 0;
    sitl->state.tbm_state.yaw_b = rotation_rad;

    sitl->state.tbm_state.cutting_header_S = sprocket_rotation_speed;
    sitl->state.tbm_state.turning_angle = rotation_rad;
    sitl->state.tbm_state.boom_rad = theta2 + radians(tbm_params._deg_BCF) + 
                                     acosf((powf(sitl->state.tbm_state.boom_cylinder_L, 2) + powf(tbm_params._mm_BC, 2) - powf(tbm_params._mm_AC, 2)) / (2*sitl->state.tbm_state.boom_cylinder_L*tbm_params._mm_BC));

    // update tbm unity interface
    float OA_OB, angle_HOC, angle_HOJ_2, angle_EOA_4;
    OA_OB = sqrt(powf(0.5*rotation_AB, 2) + powf(rotation_OL, 2));
    angle_HOC = acosf((2*rotation_r*rotation_r-rotation_HC*rotation_HC) / (2*rotation_r*rotation_r));
    angle_HOJ_2 = (M_PI - angle_HOC) / 2;
    angle_EOA_4 = atan(2*rotation_OL/rotation_AB);
    sitl->state.tbm_unity_interface.HuiZhuanTai_Angle = rotation_rad;
    sitl->state.tbm_unity_interface.HuiZhuanTai_Left_Length = sqrt(powf(rotation_r, 2)+powf(OA_OB, 2)-2*rotation_r*OA_OB*cosf(angle_EOA_4+sitl->state.tbm_unity_interface.HuiZhuanTai_Angle-angle_HOJ_2));
    sitl->state.tbm_unity_interface.HuiZhuanTai_Right_Length = sqrt(powf(rotation_r, 2)+powf(OA_OB, 2)-2*rotation_r*OA_OB*cosf(angle_EOA_4-sitl->state.tbm_unity_interface.HuiZhuanTai_Angle-angle_HOJ_2));
    sitl->state.tbm_unity_interface.HuiZhuanTai_Left_Angle = M_PI - acosf((powf(sitl->state.tbm_unity_interface.HuiZhuanTai_Left_Length, 2)+powf(OA_OB, 2)- \
                                                             powf(rotation_r, 2))/(2*sitl->state.tbm_unity_interface.HuiZhuanTai_Left_Length*OA_OB)) - angle_EOA_4;
    sitl->state.tbm_unity_interface.HuiZhuanTai_Right_Angle = M_PI - acosf((powf(sitl->state.tbm_unity_interface.HuiZhuanTai_Right_Length, 2)+powf(OA_OB, 2)- \
                                                             powf(rotation_r, 2))/(2*sitl->state.tbm_unity_interface.HuiZhuanTai_Right_Length*OA_OB)) - angle_EOA_4;
    
    sitl->state.tbm_unity_interface.DaBi_SiGan_Length = sqrt(arm_BC*arm_BC+arm_AB*arm_AB-2*arm_BC*arm_AB*cosf(theta2+radians(arm_angle_BCN)));
    sitl->state.tbm_unity_interface.DaBi_SiGan_Angle = M_PI - radians(arm_angle_BCN) - acosf((powf(sitl->state.tbm_unity_interface.DaBi_SiGan_Length, 2)+arm_BC*arm_BC-arm_AB*arm_AB)/(2*sitl->state.tbm_unity_interface.DaBi_SiGan_Length*arm_BC));
    sitl->state.tbm_unity_interface.Dianji_Angle_HZT = theta2;

    sitl->state.tbm_unity_interface.JieGeBuShenSuo_Length = 20;
    sitl->state.tbm_unity_interface.JieGeTou_Rot_Speed = sprocket_rotation_speed;
    sitl->state.tbm_unity_interface.HouTuiCheng_Angle = radians(30);

    sitl_fdm::testa& a = sitl->state.tbm_unity_interface;
    Debug("Hl:%f, Hr:%f, Hla:%f, Hra:%f, Ha:%f, DSl:%f, DSa:%f, Da:%f, RS:%f", 
           a.HuiZhuanTai_Left_Length, a.HuiZhuanTai_Right_Length, degrees(a.HuiZhuanTai_Left_Angle), degrees(a.HuiZhuanTai_Right_Angle), 
           degrees(a.HuiZhuanTai_Angle), a.DaBi_SiGan_Length, degrees(a.DaBi_SiGan_Angle), degrees(a.Dianji_Angle_HZT), a.JieGeTou_Rot_Speed);
    Debug("                                              ");

    SimRover::update(input);
}

// Motor model
void SimTBM::calc_speed(uint16_t pwm_rotation, uint16_t pwm_boom, uint16_t pwm_support_leg, uint16_t pwm_sprocket)
{
    if(pwm_boom < 1100  || pwm_boom > 1900 || pwm_rotation < 1100 || pwm_rotation > 1900 || \
       pwm_support_leg < 1100  || pwm_support_leg > 1900 || pwm_sprocket < 1100 || pwm_sprocket > 1900 )
    {
        boom_speed = rotation_speed = pwm_support_leg = pwm_sprocket = 0;

        return;
    }

    boom_speed = (pwm_boom - 1500) / 500.0 * 15;

    rotation_speed = (pwm_rotation - 1500) / 500.0 * radians(16);

    support_leg_speed = (pwm_support_leg - 1500) / 500.0 * 15;

    sprocket_rotation_speed = (pwm_sprocket - 1500) / 500.0 * radians(16);
}

bool SimTBM::get_tbm_info()
{
    if(_armInfo == nullptr) {
        _armInfo = AE_RobotArmInfo::get_singleton();
        
        if(_armInfo == nullptr) {
            return false;
        }
    }
    
    if (_armInfo_backend == nullptr) {
        _armInfo_backend = (AE_RobotArmInfo_TBM*)_armInfo->backend();

        if(_armInfo_backend == nullptr) {
            return false;
        }
    }

    if(!_armInfo_backend->get_healthy()) {
        return false;
    }

    return true;
}

} // namespace SITL
