/**/
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define CI_CHIP_TYPE 1302


#define AUDIO_PLAYER_ENABLE         1 //用于屏蔽播放器任务相关代码      0：屏蔽，1：开启

#if AUDIO_PLAYER_ENABLE
#define USE_PROMPT_DECODER 1          //播放器是否支持prompt解码器
#define USE_MP3_DECODER 1             //为1时加入mp3解码器
#define AUDIO_PLAY_SUPPT_MP3_PROMPT 1 //播放器默认开启mp3播报音

#define AUDIO_PLAY_BLOCK_CONT 4 //播放器底层缓冲区个数
#endif

#define USE_ALC_AUTO_SWITCH_MODULE 1          //使用动态alc模块:1开启，0关闭
#define USE_DENOISE_MODULE         0         //使用降噪模块:1开启，0关闭
#define USE_DOA_MODULE             0        //使用声源定位模块：1开启，0关闭
#define USE_DEREVERB_MODULE        0       //使用降混响模块：1开启，0关闭
#define USE_BEAMFORMING_MODULE     0       //使用双麦语音增强模块:1开启，0关闭
#define USE_AEC_MODULE             0      //使用回声消除模块:1开启，0关闭

#if USE_AEC_MODULE
#define PAUSE_VOICE_IN_WITH_PLAYING  0//开启aec时关闭
#define IF_JUST_CLOSE_HPOUT_WHILE_NO_PLAY   1
#endif


#if USE_BEAMFORMING_MODULE || USE_AEC_MODULE || USE_DOA_MODULE ||USE_DEREVERB_MODULE
#define HOST_CODEC_CHA_NUM  2
#endif


#define MSG_COM_USE_UART_EN 1
//#define UART_AUTO_TEST          0     //0:串口0位打印关闭自动化测试  串口1通信    1：串口0通信  串口1打印 打开自动化测试

//#ifdef UART_AUTO_TEST           
// #define UART_PROTOCOL_NUMBER (HAL_UART0_BASE)     // 通信口HAL_UART0_BASE ~ HAL_UART2_BASE
// #define CONFIG_CI_LOG_UART HAL_UART1_BASE         //打印口
// #else 
#define UART_PROTOCOL_NUMBER (HAL_UART1_BASE)     // 通信口HAL_UART0_BASE ~ HAL_UART2_BASE
#define CONFIG_CI_LOG_UART HAL_UART0_BASE         //打印口
//#endif

#define UART_PROTOCOL_BAUDRATE (UART_BaudRate9600)//通信口波特率

#define UART_PROTOCOL_VER 2 //串口协议版本号，1：一代协议，2：二代协议，255：平台生成协议(只有发送没有接收)


#define USE_EXTERNAL_CRYSTAL_OSC             1      //使用外部晶振

#if (USE_EXTERNAL_CRYSTAL_OSC == 0)
#define UART_BAUDRATE_CALIBRATE         0           // 是否使能波特率校准功能
#define BAUDRATE_SYNC_PERIOD            60000       // 波特率同步周期，单位毫秒
#define BAUD_CALIBRATE_MAX_WAIT_TIME    60          // 等待反馈包的超时时间，单位毫秒


#endif

#ifndef USE_NIGHT_LIGHT
#define USE_NIGHT_LIGHT                 1       
#endif


#endif /* _USER_CONFIG_H_ */
