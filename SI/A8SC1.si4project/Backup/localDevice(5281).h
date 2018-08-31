#ifndef   __LOCAL_DEVICE_H__
#define  __LOCAL_DEVICE_H__



typedef struct CardInfo{
	char disabledate[36];
	int untitd;
	char state[3];
	int cellid;
	int rid;
	int blockid;
	char type[3];
	int districtid;  
	char cardid[16];
	
}CardInfo,*pCardInfo;
typedef struct DeviceInfo{

	double  lockid;
	int  communityId;
	char deviceDame[64];
	

}DeviceInfo,*pDeviceInfo;



typedef struct LocalDeviceOps{
	
	

}LocalDeviceOps,*pLocalDeviceOps;








pLocalDeviceOps createpLocalDeviceServer(void);






#endif 

































