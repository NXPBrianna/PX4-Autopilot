/****************************************************************************
 *
 *   Copyright (c) 2020-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef PX4_RDDRONE_H
#define PX4_RDDRONE_H

#include <termios.h>
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>

#include <px4_platform_common/module_params.h>
#include <px4_platform_common/module.h>
#include <perf/perf_counter.h>
#include <uORB/Publication.hpp>
#include <uORB/Subscription.hpp>
#include <uORB/SubscriptionInterval.hpp>

#include <uORB/topics/landing_target_pose.h>
#include <uORB/topics/uwb_grid.h>
#include <uORB/topics/uwb_distance.h>
#include <uORB/topics/parameter_update.h>

#include <uORB/topics/offboard_control_mode.h>
#include <uORB/topics/vehicle_status.h>
#include <uORB/topics/actuator_controls.h>

#include <matrix/math.hpp>
#include <matrix/Matrix.hpp>

using namespace time_literals;

#define UWB_PARAM_UPDATE_TIME 1000000

#define UWB_CMD  0x8e
#define UWB_CMD_START  0x01
#define UWB_CMD_STOP  0x00
#define UWB_CMD_RANGING  0x0A
#define STOP_B 0x0A

#define UWB_PRECNAV_APP   0x04
#define UWB_APP_START     0x10
#define UWB_APP_STOP      0x11
#define UWB_SESSION_START 0x22
#define UWB_SESSION_STOP  0x23
#define UWB_RANGING_START 0x01
#define UWB_RANGING_STOP  0x00
#define UWB_DRONE_CTL     0x0A
#define UWB_SUBCMD_PRECLAND      0x0B
#define UWB_SUBCMD_FOLLOW_ME     0x0F

#define UWB_CMD_LEN  0x05
#define UWB_CMD_DISTANCE_LEN 0x21
#define UWB_MAC_MODE 2
#define MAX_ANCHORS 12
#define UWB_GRID_CONFIG "/fs/microsd/etc/uwb_grid_config.csv"

typedef struct {  //needs higher accuracy?
	float lat, lon, alt, yaw; //offset to true North
} gps_pos_t;

typedef struct {  //needs higher accuracy?
	int16_t x, y, z;//offset to true North
} position_t;

typedef struct {
	uint8_t MAC[2];					// MAC address of UWB device
	uint8_t status;					// Status of Measurement
	uint16_t distance; 				// Distance in cm
	uint8_t nLos; 					// line of sight y/n
	int16_t aoa_azimuth;			// AOA of incoming msg for Azimuth antenna pairing
	int16_t aoa_elevation;			// AOA of incoming msg for Altitude antenna pairing
	int16_t aoa_dest_azimuth;		// AOA destination Azimuth
	int16_t aoa_dest_elevation; 	// AOA destination elevation
	uint8_t aoa_azimuth_FOM;		// AOA Azimuth FOM
	uint8_t aoa_elevation_FOM;		// AOA Elevation FOM
	uint8_t aoa_dest_azimuth_FOM;	// AOA Azimuth FOM
	uint8_t aoa_dest_elevation_FOM;	// AOA Elevation FOM
} __attribute__((packed)) UWB_range_meas_t;

typedef struct {
	uint32_t initator_time;  	//timestamp of init
	uint32_t sessionId;	// Session ID of UWB session
	uint8_t	num_anchors;	//number of anchors
	gps_pos_t target_gps; //GPS position of Landing Point
	uint8_t  mac_mode;	// MAC address mode, either 2 Byte or 8 Byte
	uint8_t MAC[UWB_MAC_MODE][MAX_ANCHORS];
	position_t target_pos; //target position
	position_t anchor_pos[MAX_ANCHORS]; // Position of each anchor
	uint8_t stop; 		// Should be 27
} grid_msg_t;
typedef struct {
	uint8_t cmd;      			// Should be 0x8E for distance result message
	uint16_t len; 				// Should be 0x30 for distance result message
	uint32_t seq_ctr;			// Number of Ranges since last Start of Ranging
	uint32_t sessionId;			// Session ID of UWB session
	uint32_t range_interval;	// Time between ranging rounds
	uint8_t MAC[2];			// MAC address of UWB device
	UWB_range_meas_t measurements; //Raw anchor_distance distances in CM 2*9
	uint8_t stop; 		// Should be 0x1B
} __attribute__((packed)) distance_msg_t;


typedef enum {
	data 		= -1,
	prec_nav	= 0,
	follow_me	= 1,
} _uwb_driver_mode;

class UWB_SR150 : public ModuleBase<UWB_SR150>, public ModuleParams
{
public:
	UWB_SR150(const char *device_name, speed_t baudrate, bool uwb_pos_debug);

	~UWB_SR150();

	/**
	 * @see ModuleBase::custom_command
	 */
	static int custom_command(int argc, char *argv[]);

	/**
	 * @see ModuleBase::print_usage
	 */
	static int print_usage(const char *reason = nullptr);

	/**
	 * @see ModuleBase::Localization
	 */
	matrix::Vector3d  localization(double distance, double azimuth_dev, double elevation_dev);

	/**
	 * @see ModuleBase::actuator_control
	 */
	void actuator_control(double distance, double azimuth, double elevation);

	/**
	 * @see ModuleBase::Distance Result
	 */
	int distance();

	/**
	 * @see ModuleBase::task_spawn
	 */
	static int task_spawn(int argc, char *argv[]);

	static UWB_SR150 *instantiate(int argc, char *argv[]);

	void run() override;

private:
	void parameters_update();

	void grid_info_read(position_t *grid);

	DEFINE_PARAMETERS(
		(ParamInt<px4::params::UWB_PORT_CFG>) 			_uwb_port_cfg,
		(ParamInt<px4::params::UWB_MODE>)  			_uwb_mode_p,
		(ParamFloat<px4::params::UWB_INIT_OFF_X>) 		_uwb_init_off_x,
		(ParamFloat<px4::params::UWB_INIT_OFF_Y>) 		_uwb_init_off_y,
		(ParamFloat<px4::params::UWB_INIT_OFF_Z>) 		_uwb_init_off_z,
		(ParamFloat<px4::params::UWB_INIT_YAW>) 		_uwb_init_off_yaw,
		(ParamFloat<px4::params::UWB_INIT_PITCH>) 		_uwb_init_off_pitch,
		(ParamFloat<px4::params::UWB_FW_DIST>)   		_uwb_follow_distance, //Follow me
		(ParamFloat<px4::params::UWB_FW_DIST_MIN>) 		_uwb_follow_distance_min,
		(ParamFloat<px4::params::UWB_FW_DIST_MAX>) 		_uwb_follow_distance_max,
		(ParamFloat<px4::params::UWB_THROTTLE>) 		_uwb_throttle,
		(ParamFloat<px4::params::UWB_THROTTLE_REV>) 		_uwb_throttle_reverse,
		(ParamFloat<px4::params::UWB_THR_HEAD>) 		_uwb_thrust_head,
		(ParamFloat<px4::params::UWB_THR_HEAD_MIN>) 		_uwb_thrust_head_min,
		(ParamFloat<px4::params::UWB_THR_HEAD_MAX>) 		_uwb_thrust_head_max
	)


	uORB::SubscriptionInterval 						_parameter_update_sub{ORB_ID(parameter_update), 1_s};
	uORB::Publication<offboard_control_mode_s>				_offboard_control_mode_pub{ORB_ID(offboard_control_mode)};
	uORB::Publication<actuator_controls_s>					_actuator_controls_pubs[4] {ORB_ID(actuator_controls_0), ORB_ID(actuator_controls_1), ORB_ID(actuator_controls_2), ORB_ID(actuator_controls_3)};
	uORB::Subscription							_vehicle_status_sub{ORB_ID(vehicle_status)};

	hrt_abstime param_timestamp{0};

	int _uart;
	fd_set _uart_set;
	struct timeval _uart_timeout {};
	bool _uwb_pos_debug;
	_uwb_driver_mode _uwb_mode;

	uORB::Publication<uwb_grid_s> _uwb_grid_pub{ORB_ID(uwb_grid)};
	uwb_grid_s _uwb_grid{};

	uORB::Publication<uwb_distance_s> _uwb_distance_pub{ORB_ID(uwb_distance)};
	uwb_distance_s _uwb_distance{};

	uORB::Publication<landing_target_pose_s> _landing_target_pub{ORB_ID(landing_target_pose)};
	landing_target_pose_s _landing_target{};

	grid_msg_t _uwb_grid_info{};
	distance_msg_t _distance_result_msg{};
	matrix::Vector3d _rel_pos;

	matrix::Vector3d _uwb_init_offset;
	matrix::Vector3d _uwb_init_attitude;

	perf_counter_t _read_count_perf;
	perf_counter_t _read_err_perf;
};
#endif //PX4_RDDRONE_H
