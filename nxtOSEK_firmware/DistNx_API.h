#ifndef _DISTNxAPI_H
#define _DISTNxAPI_H


// I2C Bus address (specification states 0x02 but ecrobot uses only odd values)
#define DIST_ADDR 				0x01

// Commands for the distance sensor
#define DIST_CMD_GP2D120     	0x32
#define DIST_CMD_GP2YA21     	0x33
#define DIST_CMD_GP2YA02     	0x34
#define DIST_CMD_CUSTOM      	0x35
#define DIST_CMD_ENERGIZE  		0x45
#define DIST_CMD_DEENERGIZE 	0x44
#define DIST_CMD_ADPA_ON     	0x4E
#define DIST_CMD_ADPA_OFF    	0x4F

// Registers in the sensor to read from / write to
#define DIST_REG_VERSION       	0x00
#define DIST_REG_VENDORID      	0x08
#define DIST_REG_DEVICEID      	0x10
#define DIST_REG_CMD           	0x41
#define DIST_REG_DIST_LSB      	0x42
#define DIST_REG_DIST_MSB      	0x43
#define DIST_REG_VOLT_LSB    	0x44
#define DIST_REG_VOLT_MSB      	0x45
#define DIST_REG_MODULE_TYPE   	0x50
#define DIST_REG_NUM_POINTS    	0x51
#define DIST_REG_DIST_MIN_LSB  	0x52
#define DIST_REG_DIST_MIN_MSB  	0x53
#define DIST_REG_DIST_MAX_LSB  	0x54
#define DIST_REG_DIST_MAX_MSB  	0x55
#define DIST_REG_VOLT1_LSB     	0x56
#define DIST_REG_VOLT1_MSB     	0x57
#define DIST_REG_DIST1_LSB     	0x58
#define DIST_REG_DIST1_MSB     	0x59

// Models
#define DIST_MODEL_SHORT		0x32
#define DIST_MODEL_MEDIUM		0x33
#define DIST_MODEL_LONG			0x34

// A method to send (write to the cmd register) a command to the distance sensor
// Param: 	U8 	port 	- the sensor port on the NXT brick
//			U8	cmd		- the command to send
// Return:	SINT		- a status message (1: success, 0: failure)
SINT DistNx_SendCommand(U8 port, U8 cmd);
// A method to read values off a register on the sensor
// Param: 	U8 	port 	- the sensor port on the NXT brick
//			U8	reg		- the register to read from
//			U8	length	- the number of bytes to expect
// Return:	UINT		- the integer value read
UINT DistNx_GetValue(U8 port, U8 reg, U8 length);

// A method to initialize the port and power up the sensor
// Param: 	U8 	port 	- the sensor port on the NXT brick
//			U8	model	- the type of sensor (short-,medium-,long range)
inline void DistNx_Init(U8 port, U8 model);
// A method to power down the sensor and terminate the port
// Param: 	U8 	port 	- the sensor port on the NXT brick
inline void DistNx_Term(U8 port);
// A method to power up the sensor
// Param: 	U8 	port 	- the sensor port on the NXT brick
inline void DistNx_Energize(U8 port);
// A method to get the distance reading
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_Distance(U8 port);
// A method to get the raw voltage reading
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_Voltage(U8 port);
// A method to get the type of the module
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_ModuleType(U8 port);
// A method to get the number of points
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_NumPoints(U8 port);
// A method to get the minimum distance
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_MinDistance(U8 port);
// A method to get the maximum distance
// Param: 	U8 	port 	- the sensor port on the NXT brick
// Return:	UINT		- the integer value read
inline unsigned int DistNx_MaxDistance(U8 port);


#include "DistNx_API.c"
#endif
