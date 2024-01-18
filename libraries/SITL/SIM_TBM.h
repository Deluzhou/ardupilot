/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  tbm simulator class
*/

#pragma once

#include "SIM_Rover.h"
#include <AE_RobotArmInfo/AE_RobotArmInfo.h>
#include <AE_RobotArmInfo/AE_RobotArmInfo_Backend.h>
#include <AE_RobotArmInfo/AE_RobotArmInfo_TBM.h>
#include <AP_AHRS/AP_AHRS.h>

namespace SITL {

/*
  a tbm simulator
 */
class SimTBM : public SimRover {
public:
    SimTBM(const char *frame_str):SimRover(frame_str), ahrs(AP::ahrs()), is_init(false) {
      sitl->state.inclination_state.roll_deg.zero();
      sitl->state.inclination_state.pitch_deg.zero();
      sitl->state.inclination_state.yaw_deg.zero();
      sitl->state.inclination_state.yaw_deg[0] = -90;
    };

    /* update model by one time step */
    void update(const struct sitl_input &input) override;

    /* static object creator */
    static Aircraft *create(const char *frame_str) {
        return new SimTBM(frame_str);
	  }

private:
    float rotation_speed;		// rad/s the rotating speed of the rotation calculated by pwm
    float boom_speed;			  // mm/s the speed of the boom cylinder calculated by pwm
    float boom_cylinder_length; // mm
    float sprocket_rotation_speed; // rad/s
    float support_leg_cylinder_length; // mm
    float support_leg_speed; // mm/s
    float rotation_rad;
    bool is_init;

    // unity param
    float rotation_r = 722; 
    float rotation_OL = 1700;
    float rotation_AB = 530;
    float rotation_HC = 1430;
    float arm_AB = 1669;
    float arm_BC = 571.4;
    float arm_angle_BCN = 57.14;

    AE_RobotArmInfo *_armInfo;
    AE_RobotArmInfo_TBM* _armInfo_backend;

    AP_AHRS& ahrs;

    void calc_speed(uint16_t pwm_rotation, uint16_t pwm_boom, uint16_t pwm_support_leg, uint16_t pwm_sprocket);
    bool get_tbm_info();

    void init();
};

} // namespace SITL
