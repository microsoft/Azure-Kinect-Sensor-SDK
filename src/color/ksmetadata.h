#ifndef KSMETADATA_H
#define KSMETADATA_H

// Metadata IDs
#define MetadataId_CaptureStats 0x00000003
#define MetadataId_FrameAlignInfo 0x80000001

// CaptureStats Flags
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_EXPOSURETIME 0x00000001
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_EXPOSURECOMPENSATION 0x00000002
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_ISOSPEED 0x00000004
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_FOCUSSTATE 0x00000008
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_LENSPOSITION 0x00000010
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_WHITEBALANCE 0x00000020
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_FLASH 0x00000040
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_FLASHPOWER 0x00000080
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_ZOOMFACTOR 0x00000100
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_SCENEMODE 0x00000200
#define KSCAMERA_METADATA_CAPTURESTATS_FLAG_SENSORFRAMERATE 0x00000400

#pragma pack(push, 1)
typedef struct tagKSCAMERA_METADATA_ITEMHEADER
{
    uint32_t MetadataId;
    uint32_t Size; // Size of this header + metadata payload following
} KSCAMERA_METADATA_ITEMHEADER, *PKSCAMERA_METADATA_ITEMHEADER;

typedef struct tag_CUSTOM_METADATA_FrameAlignInfo
{
    KSCAMERA_METADATA_ITEMHEADER Header;
    uint32_t Flags;
    uint32_t Reserved;
    uint64_t FramePTS; // 8 bytes
    uint32_t PTSReference;
    uint64_t USBSoFSeqNum; // 8 bytes
    uint64_t USBSoFPTS;    // 8 bytes
} CUSTOM_METADATA_FrameAlignInfo, *PKSCAMERA_CUSTOM_METADATA_FrameAlignInfo;

typedef struct tagKSCAMERA_METADATA_CAPTURESTATS
{
    KSCAMERA_METADATA_ITEMHEADER Header;
    uint32_t Flags;
    uint32_t Reserved;
    uint64_t ExposureTime;
    uint64_t ExposureCompensationFlags;
    int32_t ExposureCompensationValue;
    uint32_t IsoSpeed;
    uint32_t FocusState;
    uint32_t LensPosition; // a.k.a Focus
    uint32_t WhiteBalance;
    uint32_t Flash;
    uint32_t FlashPower;
    uint32_t ZoomFactor;
    uint64_t SceneMode;
    uint64_t SensorFramerate;
} KSCAMERA_METADATA_CAPTURESTATS, *PKSCAMERA_METADATA_CAPTURESTATS;
#pragma pack(pop)

#endif // KSMETADATA_H
