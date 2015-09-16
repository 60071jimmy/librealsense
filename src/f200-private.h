#pragma once
#ifndef LIBREALSENSE_F200_PRIVATE_H
#define LIBREALSENSE_F200_PRIVATE_H

#include "uvc.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstring>  // for memcpy, memcmp
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#define DELTA_INF                           (10000000.0)
#define M_EPSILON                           (0.0001)

#define IV_COMMAND_FIRMWARE_UPDATE_MODE     0x01
#define IV_COMMAND_GET_CALIBRATION_DATA     0x02
#define IV_COMMAND_LASER_POWER              0x03
#define IV_COMMAND_DEPTH_ACCURACY           0x04
#define IV_COMMAND_ZUNIT                    0x05
#define IV_COMMAND_LOW_CONFIDENCE_LEVEL     0x06
#define IV_COMMAND_INTENSITY_IMAGE_TYPE     0x07
#define IV_COMMAND_MOTION_VS_RANGE_TRADE    0x08
#define IV_COMMAND_POWER_GEAR               0x09
#define IV_COMMAND_FILTER_OPTION            0x0A
#define IV_COMMAND_VERSION                  0x0B
#define IV_COMMAND_CONFIDENCE_THRESHHOLD    0x0C

#define IVCAM_DEPTH_LASER_POWER         1
#define IVCAM_DEPTH_ACCURACY            2
#define IVCAM_DEPTH_MOTION_RANGE        3
#define IVCAM_DEPTH_ERROR               4
#define IVCAM_DEPTH_FILTER_OPTION       5
#define IVCAM_DEPTH_CONFIDENCE_THRESH   6
#define IVCAM_DEPTH_DYNAMIC_FPS         7 // Only available on IVCAM 1.5 / SR300

#define IVCAM_COLOR_EXPOSURE_PRIORITY   1
#define IVCAM_COLOR_AUTO_FLICKER        2
#define IVCAM_COLOR_ERROR               3
#define IVCAM_COLOR_EXPOSURE_GRANULAR   4

#define HW_MONITOR_COMMAND_SIZE         (1000)
#define HW_MONITOR_BUFFER_SIZE          (1000)

#define NUM_OF_CALIBRATION_COEFFS       (64)

namespace rsimpl { namespace f200
{
    enum class IVCAMMonitorCommand : uint32_t
    {
        UpdateCalib         = 0xBC,
        GetIRTemp           = 0x52,
        GetMEMSTemp         = 0x0A,
        HWReset             = 0x28,
        GVD                 = 0x3B,
        BIST                = 0xFF,
        GoToDFU             = 0x80,
        GetCalibrationTable = 0x3D,
        DebugFormat         = 0x0B,
        TimeStampEnable     = 0x0C,
        GetPowerGearState   = 0xFF,
        SetDefaultControls  = 0xA6,
        GetDefaultControls  = 0xA7,
        GetFWLastError      = 0x0E,
        CheckI2cConnect     = 0x4A,
        CheckRGBConnect     = 0x4B,
        CheckDPTConnect     = 0x4C
    };

    struct IVCAMCommand
    {
        IVCAMMonitorCommand cmd;
        int Param1;
        int Param2;
        int Param3;
        int Param4;
        char data[HW_MONITOR_BUFFER_SIZE];
        int sizeOfSendCommandData;
        long TimeOut;
        bool oneDirection;
        char receivedCommandData[HW_MONITOR_BUFFER_SIZE];
        int receivedCommandDataLength;
        char receivedOpcode[4];

        IVCAMCommand(IVCAMMonitorCommand cmd)
        {
            this->cmd = cmd;
            Param1 = 0;
            Param2 = 0;
            Param3 = 0;
            Param4 = 0;
            sizeOfSendCommandData = 0;
            TimeOut = 5000;
            oneDirection = false;
        }
    };

    struct IVCAMCommandDetails
    {
        bool oneDirection;
        char sendCommandData[HW_MONITOR_COMMAND_SIZE];
        int sizeOfSendCommandData;
        long TimeOut;
        char receivedOpcode[4];
        char receivedCommandData[HW_MONITOR_BUFFER_SIZE];
        int receivedCommandDataLength;
    };

    struct OACOffsetData
    {
        int OACOffset1;
        int OACOffset2;
        int OACOffset3;
        int OACOffset4;
    };

    struct IVCAMTemperatureData
    {
        float LiguriaTemp;
        float IRTemp;
        float AmbientTemp;
    };

    struct IVCAMThermalLoopParams
    {
        float IRThermalLoopEnable = 1;      // enable the mechanism
        float TimeOutA = 10000;             // default time out
        float TimeOutB = 0;                 // reserved
        float TimeOutC = 0;                 // reserved
        float TransitionTemp = 3;           // celcius degrees, the transition temperatures to ignore and use offset;
        float TempThreshold = 2;            // celcius degrees, the temperatures delta that above should be fixed;
        float HFOVsensitivity = 0.025f;
        float FcxSlopeA = -0.003696988f;    // the temperature model fc slope a from slope_hfcx = ref_fcx*a + b
        float FcxSlopeB = 0.005809239f;     // the temperature model fc slope b from slope_hfcx = ref_fcx*a + b
        float FcxSlopeC = 0;                // reserved
        float FcxOffset = 0;                // the temperature model fc offset
        float UxSlopeA = -0.000210918f;     // the temperature model ux slope a from slope_ux = ref_ux*a + ref_fcx*b
        float UxSlopeB = 0.000034253955f;   // the temperature model ux slope b from slope_ux = ref_ux*a + ref_fcx*b
        float UxSlopeC = 0;                 // reserved
        float UxOffset = 0;                 // the temperature model ux offset
        float LiguriaTempWeight = 1;        // the liguria temperature weight in the temperature delta calculations
        float IrTempWeight = 0;             // the Ir temperature weight in the temperature delta calculations
        float AmbientTempWeight = 0;        // reserved
        float Param1 = 0;                   // reserved
        float Param2 = 0;                   // reserved
        float Param3 = 0;                   // reserved
        float Param4 = 0;                   // reserved
        float Param5 = 0;                   // reserved
    };

    struct IVCAMASICCoefficients
    {
        float CoefValueArray[NUM_OF_CALIBRATION_COEFFS];
    };

    struct IVCAMTesterData
    {
        int16_t TableValidation;
        int16_t TableVarsion;
        OACOffsetData OACOffsetData_;
        IVCAMThermalLoopParams ThermalLoopParams;
        IVCAMTemperatureData TemperatureData;
    };

    struct CameraCalibrationParameters
    {
        float Rmax;
        float Kc[3][3];     // [3x3]: intrinsic calibration matrix of the IR camera
        float Distc[5];     // [1x5]: forward distortion parameters of the IR camera
        float Invdistc[5];  // [1x5]: the inverse distortion parameters of the IR camera
        float Pp[3][4];     // [3x4]: projection matrix
        float Kp[3][3];     // [3x3]: intrinsic calibration matrix of the projector
        float Rp[3][3];     // [3x3]: extrinsic calibration matrix of the projector
        float Tp[3];        // [1x3]: translation vector of the projector
        float Distp[5];     // [1x5]: forward distortion parameters of the projector
        float Invdistp[5];  // [1x5]: inverse distortion parameters of the projector
        float Pt[3][4];     // [3x4]: IR to RGB (texture mapping) image transformation matrix
        float Kt[3][3];
        float Rt[3][3];
        float Tt[3];
        float Distt[5];     // [1x5]: The inverse distortion parameters of the RGB camera
        float Invdistt[5];
        float QV[6];
    };

    struct IVCAMCalibration
    {
        int uniqueNumber; //Should be 0xCAFECAFE in Calibration version 1 or later. In calibration version 0 this is zero.
        int16_t TableValidation;
        int16_t TableVersion;
        CameraCalibrationParameters CalibrationParameters;
    };

    struct SR300RawCalibration
    {
        uint16_t tableVersion;
        uint16_t tableID;
        uint32_t dataSize;
        uint32_t reserved;
        int crc;
        CameraCalibrationParameters CalibrationParameters;
        uint8_t reserved_1[176];
        IVCAMTemperatureData TemperatureData;
        uint8_t reserved21[148];
    };

    enum class IVCAMDataSource : uint32_t
    {
        TakeFromRO = 0,
        TakeFromRW = 1,
        TakeFromRAM = 2
    };

    enum Property
    {
        IVCAM_PROPERTY_COLOR_EXPOSURE                   =   1,
        IVCAM_PROPERTY_COLOR_BRIGHTNESS                 =   2,
        IVCAM_PROPERTY_COLOR_CONTRAST                   =   3,
        IVCAM_PROPERTY_COLOR_SATURATION                 =   4,
        IVCAM_PROPERTY_COLOR_HUE                        =   5,
        IVCAM_PROPERTY_COLOR_GAMMA                      =   6,
        IVCAM_PROPERTY_COLOR_WHITE_BALANCE              =   7,
        IVCAM_PROPERTY_COLOR_SHARPNESS                  =   8,
        IVCAM_PROPERTY_COLOR_BACK_LIGHT_COMPENSATION    =   9,
        IVCAM_PROPERTY_COLOR_GAIN                       =   10,
        IVCAM_PROPERTY_COLOR_POWER_LINE_FREQUENCY       =   11,
        IVCAM_PROPERTY_AUDIO_MIX_LEVEL                  =   12,
        IVCAM_PROPERTY_APERTURE                         =   13,
        IVCAM_PROPERTY_DISTORTION_CORRECTION_I          =   202,
        IVCAM_PROPERTY_DISTORTION_CORRECTION_DPTH       =   203,
        IVCAM_PROPERTY_DEPTH_MIRROR                     =   204,    //0 - not mirrored, 1 - mirrored
        IVCAM_PROPERTY_COLOR_MIRROR                     =   205,
        IVCAM_PROPERTY_COLOR_FIELD_OF_VIEW              =   207,
        IVCAM_PROPERTY_COLOR_SENSOR_RANGE               =   209,
        IVCAM_PROPERTY_COLOR_FOCAL_LENGTH               =   211,
        IVCAM_PROPERTY_COLOR_PRINCIPAL_POINT            =   213,
        IVCAM_PROPERTY_DEPTH_FIELD_OF_VIEW              =   215,
        IVCAM_PROPERTY_DEPTH_UNDISTORTED_FIELD_OF_VIEW  =   223,
        IVCAM_PROPERTY_DEPTH_SENSOR_RANGE               =   217,
        IVCAM_PROPERTY_DEPTH_FOCAL_LENGTH               =   219,
        IVCAM_PROPERTY_DEPTH_UNDISTORTED_FOCAL_LENGTH   =   225,
        IVCAM_PROPERTY_DEPTH_PRINCIPAL_POINT            =   221,
        IVCAM_PROPERTY_MF_DEPTH_LOW_CONFIDENCE_VALUE    =   5000,
        IVCAM_PROPERTY_MF_DEPTH_UNIT                    =   5001,   // in micron
        IVCAM_PROPERTY_MF_CALIBRATION_DATA              =   5003,
        IVCAM_PROPERTY_MF_LASER_POWER                   =   5004,
        IVCAM_PROPERTY_MF_ACCURACY                      =   5005,
        IVCAM_PROPERTY_MF_INTENSITY_IMAGE_TYPE          =   5006,   //0 - (I0 - laser off), 1 - (I1 - Laser on), 2 - (I1-I0), default is I1.
        IVCAM_PROPERTY_MF_MOTION_VS_RANGE_TRADE         =   5007,
        IVCAM_PROPERTY_MF_POWER_GEAR                    =   5008,
        IVCAM_PROPERTY_MF_FILTER_OPTION                 =   5009,
        IVCAM_PROPERTY_MF_VERSION                       =   5010,
        IVCAM_PROPERTY_MF_DEPTH_CONFIDENCE_THRESHOLD    =   5013,
        IVCAM_PROPERTY_ACCELEROMETER_READING            =   3000,   // three values
        IVCAM_PROPERTY_PROJECTION_SERIALIZABLE          =   3003,   
        IVCAM_PROPERTY_CUSTOMIZED                       =   0x04000000,
    };

    enum class FirmwareError : int32_t
    {
        FW_ACTIVE = 0,
        FW_MSAFE_S1_ERR,
        FW_I2C_SAFE_ERR,
        FW_FLASH_SAFE_ERR,
        FW_I2C_CFG_ERR,
        FW_I2C_EV_ERR,
        FW_HUMIDITY_ERR,
        FW_MSAFE_S0_ERR,
        FW_LD_ERR,
        FW_PI_ERR,
        FW_PJCLK_ERR,
        FW_OAC_ERR,
        FW_LIGURIA_TEMPERATURE_ERR,
        FW_CONTINUE_SAFE_ERROR,
        FW_FORZA_HUNG,
        FW_FORZA_CONTINUES_HUNG,
        FW_PJ_EYESAFETY_CHKRHARD,
        FW_MIPI_PCAM_ERR,
        FW_MIPI_TCAM_ERR,
        FW_SYNC_DISABLED,
        FW_MIPI_PCAM_SVR_ERR,
        FW_MIPI_TCAM_SVR_ERR,
        FW_MIPI_PCAM_FRAME_SIZE_ERR,
        FW_MIPI_TCAM_FRAME_SIZE_ERR,
        FW_MIPI_PCAM_FRAME_RESPONSE_ERR,
        FW_MIPI_TCAM_FRAME_RESPONSE_ERR,
        FW_USB_PCAM_THROTTLED_ERR,
        FW_USB_TCAM_THROTTLED_ERR,
        FW_USB_PCAM_QOS_WAR,
        FW_USB_TCAM_QOS_WAR,
        FW_USB_PCAM_OVERFLOW,
        FW_USB_TCAM_OVERFLOW,
        FW_Flash_OEM_SECTOR,
        FW_Flash_CALIBRATION_RW,
        FW_Flash_IR_CALIBRATION,
        FW_Flash_RGB_CALIBRATION,
        FW_Flash_THERMAL_LOOP_CONFIGURATION,
        FW_Flash_REALTEK,
        FW_RGB_ISP_BOOT_FAILED,
        FW_PRIVACY_RGB_OFF,
        FW_PRIVACY_DEPTH_OFF,
        FW_COUNT_ERROR
    };

    std::tuple<CameraCalibrationParameters, IVCAMTemperatureData, IVCAMThermalLoopParams> read_f200_calibration(uvc::device & device, std::timed_mutex & mutex);
    std::tuple<CameraCalibrationParameters, IVCAMTemperatureData, IVCAMThermalLoopParams> read_sr300_calibration(uvc::device & device, std::timed_mutex & mutex);
    float read_mems_temp(uvc::device & device, std::timed_mutex & mutex);
    int read_ir_temp(uvc::device & device, std::timed_mutex & mutex);

    void force_hardware_reset(uvc::device & device, std::timed_mutex & mutex);
    void enable_timestamp(uvc::device & device, std::timed_mutex & mutex, bool colorEnable, bool depthEnable);
    void update_asic_coefficients(uvc::device & device, std::timed_mutex & mutex, const CameraCalibrationParameters & compensated_params); // TODO: Allow you to specify resolution

    class IVCAMHardwareIO
    {
        uvc::device device;
        std::timed_mutex usbMutex;

        CameraCalibrationParameters base_calibration;
        IVCAMTemperatureData base_temperature_data;
        IVCAMThermalLoopParams thermal_loop_params;

        CameraCalibrationParameters compensated_calibration;
        float last_temperature_delta = DELTA_INF;

        std::thread temperatureThread;
        std::atomic<bool> runTemperatureThread;
        std::mutex temperatureMutex;
        std::condition_variable temperatureCv;

        void TemperatureControlLoop();
    public:
        IVCAMHardwareIO(uvc::device device, bool sr300);
        ~IVCAMHardwareIO();

        CameraCalibrationParameters & GetParameters() { std::lock_guard<std::mutex> guard(temperatureMutex); return compensated_calibration; } // TODO: Store
    };

    #define NUM_OF_CALIBRATION_COEFFS   (64)

} } // namespace rsimpl::f200

#endif
