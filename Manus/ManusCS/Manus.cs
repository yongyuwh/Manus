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

using System;
using System.Runtime.InteropServices;

namespace ManusMachina {
#pragma warning disable 0649 // Disable 'field never assigned' warning
    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_QUATERNION {
        public float w, x, y, z;

        public GLOVE_QUATERNION(float w, float x, float y, float z) {
            this.w = w;
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public static GLOVE_QUATERNION operator *(GLOVE_QUATERNION a, GLOVE_QUATERNION b) {
            return new GLOVE_QUATERNION(
                a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.y + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.z - a.y * b.x + a.z * b.w);
        }

        public float this[int index] {
            get {
                switch (index) {
                    case 0: return this.w;
                    case 1: return this.x;
                    case 2: return this.y;
                    case 3: return this.z;
                    default: throw new InvalidOperationException();
                }
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_VECTOR {
        public float x, y, z;

        public GLOVE_VECTOR(float x, float y, float z) {
            this.x = x;
            this.y = y;
            this.z = z;
        }

        public GLOVE_VECTOR ToDegrees() {
            return new GLOVE_VECTOR((float)(x * 180.0 / Math.PI), (float)(y * 180.0 / Math.PI), (float)(z * 180.0 / Math.PI));
        }

        public float[] ToArray() {
            return new float[3] { this.x, this.y, this.z };
        }

        public float this[int index] {
            get {
                switch (index) {
                    case 0: return this.x;
                    case 1: return this.y;
                    case 2: return this.z;
                    default: throw new InvalidOperationException();
                }
            }

            set {
                switch (index) {
                    case 0: this.x = value; return;
                    case 1: this.y = value; return;
                    case 2: this.z = value; return;
                    default: throw new InvalidOperationException();
                }
            }
        }


        public static GLOVE_VECTOR operator +(GLOVE_VECTOR a, GLOVE_VECTOR b) {
            return new GLOVE_VECTOR(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        public static GLOVE_VECTOR operator -(GLOVE_VECTOR a, GLOVE_VECTOR b) {
            return new GLOVE_VECTOR(a.x - b.x, a.y - b.y, a.z - b.z);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_POSE {
        public GLOVE_QUATERNION orientation;
        public GLOVE_VECTOR position;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_DATA {

        public GLOVE_VECTOR Acceleration;
        public GLOVE_VECTOR Euler;
        public GLOVE_QUATERNION Quaternion;
        [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 5)]
        public float[] Fingers;
        public uint PacketNumber;

    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_FINGER {
        public GLOVE_POSE metacarpal, proximal,
            intermediate, distal;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct GLOVE_SKELETAL {
        public GLOVE_POSE palm;
        public GLOVE_FINGER thumb, index, middle,
            ring, pinky;
    }

#pragma warning restore 0649

    public enum GLOVE_HAND {
        GLOVE_LEFT = 0,
        GLOVE_RIGHT,
    };


    /*!
    *   \brief Glove class
    *
    */
    [System.Serializable]
    public class Glove {
        public const int ERROR = -1;
        public const int SUCCESS = 0;
        public const int INVALID_ARGUMENT = 1;
        public const int OUT_OF_RANGE = 2;
        public const int DISCONNECTED = 3;

        /*! \brief Initialize the Manus SDK.
        *
        *  Must be called before any other function
        *  in the SDK.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusInit();

        /*! \brief Shutdown the Manus SDK.
        *
        *  Must be called when the SDK is no longer
        *  needed.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusExit();


        /*! \brief Get the state of a glove.
        *
        *  \param hand The left or right hand index.
        *  \param state Output variable to receive the data.
        *  \param timeout Milliseconds to wait until the glove returns a value.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusGetData(GLOVE_HAND hand, out GLOVE_DATA data, uint timeout = 0);


        /*! \brief Get a skeletal model for the given glove state.
        *
        *  The skeletal model gives the orientation and position of each bone
        *  in the hand and fingers. The positions are in millimeters relative to
        *  the position of the hand palm.
        *
        *  Since the thumb has no intermediate phalanx it has a separate structure
        *  in the model.
        * 
        *  \param hand The left or right hand index.
        *  \param model The glove skeletal model.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusGetSkeletal(GLOVE_HAND hand, out GLOVE_SKELETAL model, uint timeout = 1000);

        /*! \brief Configure the handedness of the glove.
        *
        *  This reconfigures the glove for a different hand.
        *
        *  \warning This function overwrites factory settings on the
        *  glove, it should only be called if the user requested it.
        *
        *  \param hand The left or right hand index.
        *  \param right_hand Set the glove as a right hand.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusSetHandedness(GLOVE_HAND hand, GLOVE_HAND newHand);

        /*! \brief Calibrate the IMU on the glove.
        *
        *  This will run a self-test of the IMU and recalibrate it.
        *  The glove should be placed on a stable flat surface during
        *  recalibration.
        *
        *  \warning This function overwrites factory settings on the
        *  glove, it should only be called if the user requested it.
        *
        *  \param hand The left or right hand index.
        *  \param gyro Calibrate the gyroscope.
        *  \param accel Calibrate the accelerometer.
        *  \param fingers Calibrate the fingers.
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusCalibrate(GLOVE_HAND hand, bool gyro = true, bool accel = true, bool fingers = false);

        /*! \brief Set the ouput power of the vibration motor.
        *
        *  This sets the output power of the vibration motor.
        *
        *  \param glove The glove index.
        *  \param power The power of the vibration motor ranging from 0 to 1 (ex. 0.5 = 50% power).
        */
        [DllImport("Manus.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int ManusSetVibration(GLOVE_HAND hand, float power);
    }
}
