/****************************************************************
                       Copyright (c)
                    Microsoft Corporation
                    All Rights Reserved
               Licensed under the MIT License.
****************************************************************/

/**
 * Kinect For Azure SDK.
 */

#ifndef COLOR_COMMAND_H
#define COLOR_COMMAND_H

//************************ Includes *****************************

#ifdef __cplusplus
extern "C" {
#endif

//**************Symbolic Constant Macros (defines)  *************

//************************ Typedefs *****************************
#define DEV_CMD_SET_SYS_CFG 0x80000001
#define DEV_CMD_GET_SYS_CFG 0x80000002
#define DEV_CMD_IMU_STREAM_START 0x80000003
#define DEV_CMD_IMU_STREAM_STOP 0x80000004
#define DEV_CMD_GET_JACK_STATE 0x80000006

//************ Declarations (Statics and globals) ***************

//******************* Function Prototypes ***********************

#ifdef __cplusplus
}
#endif

#endif /* COLOR_COMMAND_H */
