#ifndef   __LOCAL_DEVICE_H__
#define  __LOCAL_DEVICE_H__




typedef struct DeviceInfo{

	int  lockid;
	int  communityId;
	char deviceDame[64];
	

}DeviceInfo,*pDeviceInfo;



typedef struct LocalDeviceOps{
	
	

}LocalDeviceOps,*pLocalDeviceOps;






pLocalDeviceOps createpLocalDeviceServer(void);






#endif 

































