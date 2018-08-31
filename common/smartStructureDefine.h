#ifndef  CMD_STRUCT_DEFINE__H_
#define  CMD_STRUCT_DEFINE__H_


typedef unsigned char uchar;
typedef unsigned int  uint;
typedef struct S_SMARTHOME_CMD  S_SMARTHOME_CMD,*pS_SMARTHOME_CMD;
typedef struct S_CONTROL_CMD   S_CONTROL_CMD,*pS_CONTROL_CMD;
typedef enum SETTINGS SETTINGS;
typedef enum RESULT   RESULT;
typedef enum ACTION   ACTION;
typedef enum DEV_TYPE DEV_TYPE;

enum SETTINGS{
	WELBELL_OPERATE_TARGET_DEVICE = 0,
	WELBELL_OPERATE_TARGET_FLOOR_DEVICE,
	WELBELL_OPERATE_TARGET_FLOOR_ROOM_DEVICE,
	WELBELL_SEARCH_ALL_DEVICES,
	WELBELL_LINKING_STATE,
	WELBELL_STATUS_REFRESH,
	WELBELL_MODIFY_SCENE,
	WELBELL_CREATE_SCENE,
	WELBELL_MODIFY_ROOM,
	WELBELL_MODIFY_LINKAGE_TASK,
	WELBELL_DELETE_LINKAGE_TASK,
	WELBELL_CREATE_FLOOR,
	WELBELL_SETTINGS_MAX,
};
enum RESULT{
	SUCCEED = 0,
	FALSE, 
	RESULT_MAX,
};
enum ACTION{
	OFF = 0,
	ON ,
	ACTION_MAX,
};
enum DEV_TYPE{
	ADJUSTABLE_LIGHT = 0,
	COMMON_LIGHT,
	SOCKET,
	CURTAIN,
	WINDOW_BLINDS,
	AIR_CONDITION,
	TELEVISION,
	VOICE_BOX,
	SPLIT_THE_CURTAIN,
	POINT_CONTACT_TYPE_RELAY,
	SWITCH_TYPE_RELAY,
	INFRARED_TRANSPONDER,
	WIRELESS,
	CONTEXTUAL_MODEL,
	CAMERA,
	SCENE_PANEL,
	REMOTE_CONTROL,
	REPEATERS,
	LUMINANCE_TRANSDUCER,
	RGB_LIGHT,
	VISIBLE_INTERPHONE_MODULE,
	DOOR_LOCK,
	TEMPERATURE_SENSOR,
	HUMIDITY_SENSOR,
	AIR_PURITY_SENSOR,
	COMBUSTIBLE_GAS_SENSOR,
	INFRARED_HUMAN_BODY_SENSOR,
	SMOKE_TRANSDUCER,
	PANALARM,
	S20,
	ALLONE,
	KEPLER,
	SET_TOP_BOX,
    CUSTOMIZE_INFRARED,
    ADJUSTABLE_SPLIT_THE_CURTAIN,
    ADJUSTABLE_ROLLER_SHUTTERS,
    AIR_CONDITIONER_FACEPLATE,
    PUSH_THE_WINDOW_CLEANER,
    COLOR_TEMPERATURE_LIGHT,
    ROLLER_SHUTTER,
    SPRINKLER,
    ROLLER_SHUTTERS,
    SINGLE_CONTROL_STRIP,
    VICENTER300,
    MINIHUB,
    DOOR_MAGNETIC,
    WINDOW_MAGNETIC,
    DRAWER_MAGNETIC,
    OTHER_WINDOW_MAGNETIC,
    SCENE_PANEL_5,
    SCENE_PANEL_7,
    CLOTHES_HANGER,
    HUADING_LIGHT_SOCKET,
    WATER_DETECTOR,
    CO_ALARM,
    DANGER_BUTTON,
    BACKGROUND_MUSIC,
    ELECTRIC_FAN,
    SINGLE_CIRCUIT_SWITCH,
    TWO_CIRCUIT_SWITCH,
    MAX_TYPE,
    };
struct S_SMARTHOME_CMD{
	 uchar manufacturer[32];
	 DEV_TYPE dev_type;
	 uchar  device_name[32];
	 uchar  device_id[32];
	 ACTION action;
	 uint   values;
	 uint   floor_id;
	 uchar  floor_name[32];
	 uint   room_id;
	 uchar  room_name[32];
	struct S_SMARTHOME_CMD *next;
};


struct S_CONTROL_CMD{
	SETTINGS controlCmd;
	RESULT	 resultCode;
	union{
		pS_SMARTHOME_CMD content;
	};
};
#endif

/* 
   0：调光灯 
   1：普通灯光
   2：插座、
   3：幕布、
   4：百叶窗、
   5：空调、
   6：电视；
   7：音箱；
   8：对开窗帘?
   9：点触型继电器；
   10：开关型继电器；
   11：红外转发器；
   12：无线；?
   13：情景模式；?
   14：摄像头；?
   15：情景面板；
   16：遥控器；
   17：中继器；
   18：亮度传感器;
   19：RGB灯；
   20:可视对讲模块;
   21:门锁;
   22:温度传感器；
   23：湿度传感器;
   24:空气质量传感器;
   25:可燃气体传感器;
   26:红外人体传感器;
   27:烟雾传感器;
   28:报警设备；
   29：S20；
   30：Allone；
   31：kepler；
   32：机顶盒；
   33：自定义红外
   34：对开窗帘（支持按照百分比控制）；
   35：卷帘（支持按照百分比控制）；
   36：空调面板；
   37：推窗器；
   38：色温灯；
   39：卷闸门；
   40：花洒；
   41：卷帘（无百分比）；
   42：单控排插；
   43：vicenter300主机；
   44：miniHub；
   45：门磁；
   46：窗磁；
   47：抽屉磁；
   48：其他类型的门窗磁；
   49：情景面板（5键）；
   50：情景面板（7键）；
   51：晾衣架;
   52：华顶夜灯插座；
   53：水浸传探测器；
   54：一氧化碳报警器；
   55：紧急按钮
   56：背景音乐；
   57：电风扇
   58：单路控制器
   59: 双路控制器
   */


































