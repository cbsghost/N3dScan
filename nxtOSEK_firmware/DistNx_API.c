#include "ecrobot_interface.h"
#include "DistNx_API.h"


SINT DistNx_SendCommand(U8 port, U8 cmd)
{
	U8 inBuf[] = { cmd };
	return ecrobot_send_i2c(port, DIST_ADDR, DIST_REG_CMD, inBuf, 1);
}
UINT DistNx_GetValue(U8 port, U8 reg, U8 length)
{
	static U8 outBuf[8];
	ecrobot_read_i2c(port, DIST_ADDR, reg, outBuf, length);
	if(length==2)
		return outBuf[1]*256 + outBuf[0];
	else
		return outBuf[0];
}


inline void DistNx_Init(U8 port, U8 model)
{
	ecrobot_init_i2c(NXT_PORT_S1, LOWSPEED);
	DistNx_SendCommand(port, model);
	DistNx_Energize(port);
}
inline void DistNx_Term(U8 port)
{
	ecrobot_term_i2c(port);
}
inline void DistNx_Energize(byte port)
{
	DistNx_SendCommand(port, DIST_CMD_ENERGIZE);
}
inline unsigned int DistNx_Distance(byte port)
{
	return DistNx_GetValue(port, DIST_REG_DIST_LSB, 2);
}
inline unsigned int DistNx_Voltage(byte port)
{
	return DistNx_GetValue(port, DIST_REG_VOLT_LSB, 2);
}
inline unsigned int DistNx_ModuleType(byte port)
{
	return DistNx_GetValue(port, DIST_REG_MODULE_TYPE, 1);
}
inline unsigned int DistNx_NumPoints(byte port)
{
	return DistNx_GetValue(port, DIST_REG_NUM_POINTS, 1);
}
inline unsigned int DistNx_MinDistance(byte port)
{
	return DistNx_GetValue(port, DIST_REG_DIST_MIN_LSB, 2);
}
inline unsigned int DistNx_MaxDistance(byte port)
{
	return DistNx_GetValue(port, DIST_REG_DIST_MAX_LSB, 2);
}
