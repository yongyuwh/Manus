/*
   Copyright 2015 Manus VR

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _MANUS_H
#define _MANUS_H

#include <stdint.h>

#ifdef MANUS_EXPORTS
#define MANUS_API __declspec(dllexport)
#else
#define MANUS_API __declspec(dllimport)
#endif

#define MANUS_FINGERS 5

/*! Quaternion representing an orientation. */
typedef struct {
	float w, x, y, z;
} GLOVE_QUATERNION;

/*! Three element vector, can either represent a rotation, translation or acceleration. */
typedef struct {
	float x, y, z;
} GLOVE_VECTOR;

/*! Pose structure representing an orientation and position. */
typedef struct {
	GLOVE_QUATERNION orientation;
	GLOVE_VECTOR position;
} GLOVE_POSE;

/*! Raw data packet from the glove. */
typedef struct {
	//! Linear acceleration vector in Gs.
	GLOVE_VECTOR Acceleration;
	//! Orientation in euler angles.
	GLOVE_VECTOR Euler;
	//! Orientation in quaternions.
	GLOVE_QUATERNION Quaternion;
	//! Normalized bend value for each finger ranging from 0 to 1.
	float Fingers[MANUS_FINGERS];
	//! Sequence number of the data packet.
	unsigned int PacketNumber;
} GLOVE_DATA;

/*! Structure containing the pose of each bone in a finger. */
typedef struct {
	GLOVE_POSE metacarpal, proximal,
		intermediate, distal;
} GLOVE_FINGER;

/*! Skeletal model of the hand which contains a pose for the palm and all the bones in the fingers. */
typedef struct {
	GLOVE_POSE palm;
	GLOVE_FINGER thumb, index, middle, ring, pinky;
} GLOVE_SKELETAL;

/*! Indicates which hand is being queried for.  */
typedef enum {
	GLOVE_LEFT = 0,
	GLOVE_RIGHT,
} GLOVE_HAND;


//-- going to redefine -- 

/**
* \defgroup Glove Manus Glove
* @{
*/

#define MANUS_ERROR -1
#define MANUS_SUCCESS 0
#define MANUS_INVALID_ARGUMENT 1
#define MANUS_DISCONNECTED 2

#ifdef __cplusplus
extern "C" {
#endif
	/*! \brief Initialize the Manus SDK.
	*
	*  Must be called before any other function in the SDK.
	*  This function should only be called once.
	*/
	MANUS_API int ManusInit();

	/*! \brief Shutdown the Manus SDK.
	*
	*  Must be called when the SDK is no longer
	*  needed.
	*/
	MANUS_API int ManusExit();

	/*! \brief Get the state of a glove.
	*
	*  \param hand The left or right hand index.
	*  \param state Output variable to receive the data.
	*  \param timeout Milliseconds to wait until the glove returns a value.
	*/
	MANUS_API int ManusGetData(GLOVE_HAND hand, GLOVE_DATA* data, unsigned int timeout = 0);

	/*! \brief Get a skeletal model for the given glove state.
	*
	*  The skeletal model gives the orientation and position of each bone
	*  in the hand and fingers. The positions are in millimeters relative to
	*  the position of the hand palm.
	*
	*  Since the thumb has no intermediate phalanx it has a separate structure
	*  in the model.
	*
	*  This function is thread-safe.
	* 
	*  \param hand The left or right hand index.
	*  \param model The glove skeletal model.
	*/
	MANUS_API int ManusGetSkeletal(GLOVE_HAND hand, GLOVE_SKELETAL* model, unsigned int timeout = 0);

	/*! \brief Set the ouput power of the vibration motor.
	*
	*  This sets the output power of the vibration motor.
	*
	*  \param glove The glove index.
	*  \param power The power of the vibration motor ranging from 0 to 1 (ex. 0.5 = 50% power).
	*/
	MANUS_API int ManusSetVibration(GLOVE_HAND hand, float power);



	MANUS_API int ManusGetRssi(GLOVE_HAND hand, int32_t* rssi, unsigned int timeout);
	MANUS_API int ManusGetFlags(GLOVE_HAND hand, uint8_t* flags, unsigned int timeout);
	MANUS_API int ManusGetBatteryVoltage(GLOVE_HAND hand, uint16_t* battery, unsigned int timeout);
	MANUS_API int ManusGetBatteryPercentage(GLOVE_HAND hand, uint8_t* battery, unsigned int timeout);

	MANUS_API int ManusCalibrate(GLOVE_HAND hand, bool gyro, bool accel, bool fingers);
	MANUS_API int ManusSetHandedness(GLOVE_HAND hand, bool right_hand);
	MANUS_API bool ManusIsConnected(GLOVE_HAND hand);
	MANUS_API int ManusPowerOff(GLOVE_HAND hand);


#ifdef __cplusplus
}
#endif

/**@}*/

#endif
