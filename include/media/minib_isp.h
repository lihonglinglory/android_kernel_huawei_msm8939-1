/* < DTS2014122203451 yangzhenxi 20141223 begin */
#ifndef _MINIB_ISP_H_
#define _MINIB_ISP_H_
/* < DTS2015040802929 yangzhenxi 20150408 begin */
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
/* DTS2015040802929 yangzhenxi 20150408 end > */


// Camera Profile
#define ISPCMD_CAMERA_GET_SYSTEMINFORMATION                    0x3001
#define ISPCMD_CAMERA_SET_BASICPARAMETERS                      0x3002
#define ISPCMD_CAMERA_GET_BASICPARAMETERS                      0x3003
#define ISPCMD_CAMERA_SET_METERINGAREA                         0x3004
#define ISPCMD_CAMERA_GET_METERINGAREA                         0x3005
#define ISPCMD_CAMERA_SET_FOCUSAREA                                    0x3006
#define ISPCMD_CAMERA_GET_FOCUSAREA                                    0x3007
#define ISPCMD_CAMERA_SET_DIGTIALZOOMLEVEL                     0x3008
#define ISPCMD_CAMERA_GET_DIGITALZOOMLEVEL                     0x3009
#define ISPCMD_CAMERA_SET_SENSORMODE                           0x300A
#define ISPCMD_CAMERA_GET_SENSORMODE                           0x300B
#define ISPCMD_CAMERA_SET_SCENEMODE                                    0x300C
#define ISPCMD_CAMERA_GET_3AINFO                                       0x3101
#define ISPCMD_CAMERA_GET_MOTIONVECTOR                         0x3103
#define ISPCMD_CAMERA_PREVIEWSTREAMONOFF            0x3010
#define ISPCMD_CAMERA_PREVIEWSTREAMOFF                         0x3011
#define ISPCMD_CAMERA_STARTVIDEO                                       0x3020
#define ISPCMD_CAMERA_STOPVIDEO                                                0x3021
#define ISPCMD_CAMERA_TAKEPICTURES                                     0x3030
#define ISPCMD_CAMERA_TAKES1LOCK                                       0x3031
#define ISPCMD_CAMERA_PREVIEW_DISABLE_AE                       0x3032
#define ISPCMD_CAMERA_PREVIEW_DISABLE_AF                       0x3033
#define ISPCMD_CAMERA_PREVIEW_DISABLE_AWB                      0x3034
#define ISPCMD_CAMERA_TAKES1LOCK_ABORT                         0x3035
#define ISPCMD_CAMERA_STARTDUALVIDEO                           0x3040
#define ISPCMD_CAMERA_STOPDUALVIDEO                                    0x3041
#define ISPCMD_CAMERA_SET_FAST_SHOT_MODE                       0x3042
/* < DTS2015073105787 yuguangcai 20150805 begin */
#define ISPCMD_CAMERA_SET_VCM_INITIAL_STEP                     0x3044
/* DTS2015073105787 yuguangcai 20150805 end > */

// Bulk Data
#define ISPCMD_BULK_WRITE_COMPONENTCODE                                0x2000
#define ISPCMD_BULK_READ_COMPONENTCODE                         0x2001
#define ISPCMD_BULK_WRITE_BASICCODE                                    0x2002
#define ISPCMD_BULK_READ_BASICCODE                                     0x2003
#define ISPCMD_BULK_WRITE_ADVANCEDCODE                         0x2004
#define ISPCMD_BULK_READ_ADVANCEDCODE                          0x2005
#define ISPCMD_BULK_WRITE_CLIBRATIONDATA                       0x2006
#define ISPCMD_BULK_READ_CALIBRATIONDATA                       0x2007
#define ISPCMD_BULK_WRITE_BOOTCODE                                     0X2008
#define ISPCMD_BULK_WRITE_MEMORY                                       0x2100
#define ISPCMD_BULK_READ_MEMORY                                                0x2101
#define ISPCMD_BULK_LOG_DUMP                                           0x2102
/* < DTS2015011603233 houzhipeng 20150119 begin */
#define ISPCMD_BULK_GET_OTP_DATA                                       0x2104
/* DTS2015011603233 houzhipeng 20150119 end > */
/*< DTS2015012606635 tangying/205982 20150126 begin*/
/*add camera ois driver*/
#define ISPCMD_BULK_READ_OIS_DATA                    0x2105
#define ISPCMD_BULK_WRITE_OIS_DATA                   0x2106
/*DTS2015012606635 tangying/205982 20150126 end >*/
/*< DTS2015030502023 tangying/205982 20150305 begin*/
/*add hdr and mutiframe interface base on mini isp*/
#define ISPCMD_BULK_WRITE_START_ZSL_SNAPSHOT                    0x2109
#define ISPCMD_BULK_WRITE_STOP_ZSL_SNAPSHOT                    0x210A
/*DTS2015030502023 tangying/205982 20150305 end >*/
#define ISPCMD_GET_BULK_CHIPTEST_REPORT                                0x010A
// Basic Setting
#define ISPCMD_BASIC_SET_ANTIFLICKER                           0x1010
#define ISPCMD_BASIC_GET_ANTIFLICKER                           0x1011
#define ISPCMD_BASIC_SET_AUTOEXPOSURE                          0x1012
#define ISPCMD_BASIC_GET_AUTOEXPOSURE                          0x1013
#define ISPCMD_BASIC_SET_METERINGAREA                          0x1014
#define ISPCMD_BASIC_GET_METERINGAREA                          0x1015
#define ISPCMD_BASIC_SET_EXPOSURECOMPENSATION          0x1016
#define ISPCMD_BASIC_GET_EXPOSURECOMPENSATION          0x1017
#define ISPCMD_BASIC_SET_AEMODE                                                0x1018
#define ISPCMD_BASIC_GET_AEMODE                                                0x1019
#define ISPCMD_BASIC_SET_BURSTMODE                                     0x101A
#define ISPCMD_BASIC_SET_AE_OVER_STATE                         0x101B
#define ISPCMD_BASIC_SET_AUTOWHITEBALANCE                      0x1020
#define ISPCMD_BASIC_GET_AUTOWHITEBALANCE                      0x1021
#define ISPCMD_BASIC_SET_WHITEBALANCE                          0x1022
#define ISPCMD_BASIC_GET_WHITEBALANCE                          0x1023
#define ISPCMD_BASIC_SET_FOCUSMODE                                     0x1030
#define ISPCMD_BASIC_GET_FOCUSMODE                                     0x1031
#define ISPCMD_BASIC_SET_FOCUSAREA                                     0x1032
#define ISPCMD_BASIC_GET_FOCUSAREA                                     0x1033
#define ISPCMD_BASIC_GET_FOCALLENGTH                           0x1034
#define ISPCMD_BASIC_GET_FOCUSDISTANCE                         0x1035
#define ISPCMD_BASIC_SET_AFFLASHON                                     0x1036
#define ISPCMD_BASIC_SET_GSENSOR_INFO                          0x1037
#define ISPCMD_BASIC_SET_AFLEVEL                                       0x1038
#define ISPCMD_BASIC_SET_ISOLEVEL                                      0x1040
#define ISPCMD_BASIC_GET_ISOLEVEL                                      0x1041
#define ISPCMD_BASIC_SET_DIGTIALZOOMLEVEL                      0x1042
#define ISPCMD_BASIC_GET_DIGITALZOOMLEVEL                      0x1043
#define ISPCMD_BASIC_SET_FLASHLIGHTMODE                                0x1044
#define ISPCMD_BASIC_GET_FLASHLIGHTMODE                                0x1045
#define ISPCMD_BASIC_SET_PREFLASHON                                    0x1046
#define ISPCMD_BASIC_SET_MAINFLASHENERGY                       0x1047
#define ISPCMD_BASIC_SET_SHARPNESS                                     0x1050
#define ISPCMD_BASIC_GET_SHARPNESS                                     0x1051
#define ISPCMD_BASIC_SET_CONTRAST                                      0x1052
#define ISPCMD_BASIC_GET_CONTRAST                                      0x1053
#define ISPCMD_BASIC_SET_SATURATION                                    0x1054
#define ISPCMD_BASIC_GET_SATURATION                                    0x1055
#define ISPCMD_BASIC_SET_DIGITALLIGHTING                       0x1056
#define ISPCMD_BASIC_GET_DIGITALLIGHTING                       0x1057
#define ISPCMD_BASIC_SET_CAPTURE_MODE                          0x1058
#define ISPCMD_BASIC_SET_SCENEMODE                                     0x1060
#define ISPCMD_BASIC_GET_SCENEMODE                                     0x1061
#define ISPCMD_BASIC_SET_FACEAREA                                      0x1062
#define ISPCMD_BASIC_GET_FACEAREA                                      0x1063
#define ISPCMD_BASIC_SET_PICTUREFORMAT                         0x1070
#define ISPCMD_BASIC_GET_PICTUREFORMAT                         0x1071
#define ISPCMD_BASIC_SET_PICTURESIZE                           0x1072
#define ISPCMD_BASIC_GET_PICTURESIZE                           0x1073
#define ISPCMD_BASIC_SET_LIVEVIEWFORMAT                                0x1080
#define ISPCMD_BASIC_GET_LIVEVIEWFORMAT                                0x1081
#define ISPCMD_BASIC_SET_LIVEVIEWFPSRANGE                      0x1082
#define ISPCMD_BASIC_GET_LIVEVIEWFPSRANGE                      0x1083
#define ISPCMD_BASIC_SET_LIVEVIEWSIZE                          0x1084
#define ISPCMD_BASIC_GET_LIVEVIEWSIZE                          0x1085
#define ISPCMD_BASIC_SET_VideoStabilization                    0x1090
#define ISPCMD_BASIC_GET_VideoStabilization                    0x1091
#define ISPCMD_BASIC_SET_3DNR                                          0x1092
#define ISPCMD_BASIC_GET_3DNR                                          0x1093
#define ISPCMD_BASIC_SET_WDR                                           0x1094
#define ISPCMD_BASIC_GET_WDR                                           0x1095
#define ISPCMD_BASIC_SET_HDR                                           0x1096
#define ISPCMD_BASIC_GET_HDR                                           0x1097
#define ISPCMD_BASIC_SET_SUPERRESOLUTION                       0x1098
#define ISPCMD_BASIC_GET_SUPERRESOLUTION                       0x1099
#define ISPCMD_BASIC_SET_ZEROSHUTTERLAG                                0x109A
#define ISPCMD_BASIC_GET_ZEROSHUTTERLAG                                0x109B
#define ISPCMD_BASIC_SET_HISTOGRAM                                     0x109C
#define ISPCMD_BASIC_GET_HISTOGRAM                                     0x109E
#define ISPCMD_BASIC_SET_SHUTTERKEY                                    0x109F
#define ISPCMD_BASIC_TAKE_AE_IMAGE                                     0x10A0
#define ISPCMD_BASIC_SET_DENOISEMODE                0x10A1
#define ISPCMD_BASIC_HDR_PROCESS                                       0x10A3
#define ISPCMD_BASIC_SET_S1AF_ENABLE                           0x10A4
#define ISPCMD_BASIC_SET_Motion_Detected                       0x10A5
/* < DTS2015010900746 yangzhenxi 20150109 begin */
#define ISPCMD_BASIC_SET_AF_STEPS                       0x10A7
/* DTS2015010900746 yangzhenxi 20150109 end > */
/* < DTS2015041001148 yangyang 20150410 begin */
#define ISPCMD_BASIC_SET_IQ_BANDING                     0x10AE
/* < DTS2015041001148 yangyang 20150410 end*/

// System Management
#define ISPCMD_SYSTEM_GET_FIRMWAREVERSION                      0x0001
#define ISPCMD_SYSTEM_GET_CHIPID                                       0x0003
#define ISPCMD_SYSTEM_GET_FIRSTSENSORID                                0x0005
#define ISPCMD_SYSTEM_GET_SECONDSENSORID                       0x0007
#define ISPCMD_SYSTEM_CHANGEMODE                                       0x0010
#define ISPCMD_SYSTEM_GET_STATUSOFMODECHANGE           0x0011
#define ISPCMD_SYSTEM_CAPTUREIMAGES                                    0x0012
#define ISPCMD_SYSTEM_GET_STATUSOFIMAGECAPTURE         0x0013
#define ISPCMD_SYSTEM_GET_STATUSOFLASTEXECUTEDCOMMAND  0x0015
#define ISPCMD_SYSTEM_GET_ERRORCODE                                    0x0016
#define ISPCMD_SYSTEM_SET_ISPREGISTER                          0x0100
#define ISPCMD_SYSTEM_GET_ISPREGISTER                          0x0101
#define ISPCMD_SYSTEM_SET_FIRSTSENSORREGISTER          0x0102
#define ISPCMD_SYSTEM_GET_FIRSTSENSORREGISTER          0x0103
#define ISPCMD_SYSTEM_SET_DEBUGCOMMAND                         0x0104
#define ISPCMD_SYSTEM_GET_DEBUGCOMMAND                         0x0105
#define ISPCMD_SYSTEM_SET_EXPO_TIME                                    0x0106
#define ISPCMD_SYSTEM_SET_AD_GAIN                                      0x0107
#define ISPCMD_SYSTEM_SET_LOG_LEVEL                                    0x0109

// AL6045 Operation
#define ISPCMD_AL6045_MINIISPOPEN                                      0x4000
#define ISPCMD_AL6045_ISOAWBCALIBRATION                                0x4100
#define ISPCMD_AL6045_TAKEPICSHADING                           0x4101
/* < DTS2015040802929 yangzhenxi 20150408 begin */
typedef enum {
/* < DTS2015011603233 houzhipeng 20150119 begin */
	MINI_ISP_STATE_IDLE				= (0U),
	MINI_ISP_STATE_READY			= (1U<<0),
	MINI_ISP_STATE_SYNC				= (1U<<1),
	MINI_ISP_STATE_PWDN				= (1U<<2),
	/*< DTS2015012606635 tangying/205982 20150126 begin*/
	/*add camera ois driver*/
	MINI_ISP_STATE_OIS				= (1U<<3),
	/*DTS2015012606635 tangying/205982 20150126 end >*/
}misp_state;


#ifdef CONFIG_COMPAT
struct msm_camera_spi_array32 {
	compat_uptr_t param;
	uint16_t size;
	uint16_t opcode;
	uint16_t delay;
	uint8_t is_block_data;
	uint8_t is_recv;
	uint8_t is_wait_state;
	misp_state wait_state;
};

struct msm_camera_spi_reg_settings32 {
	compat_uptr_t reg_settings; //msm_camera_spi_array32
	uint16_t size;
};
#endif
struct msm_camera_spi_array {
	uint8_t *param;
	uint16_t size;
	uint16_t opcode;
	uint16_t delay;
	uint8_t is_block_data;
	uint8_t is_recv;
	uint8_t is_wait_state;
	misp_state wait_state;
};
struct msm_camera_spi_reg_settings {
	struct msm_camera_spi_array *reg_settings;
	uint16_t size;
};
#endif
/* DTS2015040802929 yangzhenxi 20150408 end > */
/* DTS2014122203451 yangzhenxi 20141223 end > */
