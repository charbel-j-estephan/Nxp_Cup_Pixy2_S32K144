/*==================================================================================================
*    Copyright 2021-2024 NXP
*
*    NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*    used strictly in accordance with the applicable license terms. By expressly
*    accepting such terms or by downloading, installing, activating and/or otherwise
*    using the software, you are agreeing that you have read, and that you agree to
*    comply with and are bound by, such license terms. If you do not agree to be
*    bound by the applicable license terms, then you may not retain, install,
*    activate or otherwise use the software.
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "pixy2.h"
#include "CDD_I2c.h"
#include "Gpt.h"
/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
static volatile Pixy2 Pixy2Instance;

I2c_DataType PixyReceivedLinesBuffer[100U];
/* Line getFeatures request: {sync 0xAE,0xC1, type=48(getFeatures), len=2, REQUEST, FEATURES}.
 * REQUEST byte (index 4): 0 = getMainFeatures (Pixy2's tracked main vector only, noise-filtered),
 *                         1 = getAllFeatures  (every vector/branch/scratch -> noisy).
 * FEATURES byte (index 5): bitmask, 1 = LINE_VECTOR. */
I2c_DataType PixyLinesRequestCommand[6U] = {174U, 193U, 48U, 2U, 1U, 3U};
I2c_DataType PixyLedSetCommand[7U] = {174U, 193U, 20U, 3U, 0U, 0U, 0U};
I2c_DataType PixyLedSetReceiveBuffer[10U];
/* Integrated white lamp: type=22(setLamp), len=2, {upper, lower}. Separate from the RGB LED. */
I2c_DataType PixyLampSetCommand[6U] = {174U, 193U, 22U, 2U, 0U, 0U};
I2c_DataType PixyLampSetReceiveBuffer[10U];
I2c_RequestType PixyLinesRequest = {0x54, FALSE, FALSE, FALSE, FALSE, 6U, I2C_SEND_DATA, PixyLinesRequestCommand};
I2c_RequestType PixyLinesReceive = {0x54, FALSE, FALSE, FALSE, FALSE, 100U, I2C_RECEIVE_DATA, PixyReceivedLinesBuffer};
I2c_RequestType PixyLedRequest = {0x54, FALSE, FALSE, FALSE, FALSE, 7U, I2C_SEND_DATA, PixyLedSetCommand};
I2c_RequestType PixyLedResponse = {0x54, FALSE, FALSE, FALSE, FALSE, 10U, I2C_RECEIVE_DATA, PixyLedSetReceiveBuffer};
I2c_RequestType PixyLampRequest = {0x54, FALSE, FALSE, FALSE, FALSE, 6U, I2C_SEND_DATA, PixyLampSetCommand};
I2c_RequestType PixyLampResponse = {0x54, FALSE, FALSE, FALSE, FALSE, 10U, I2C_RECEIVE_DATA, PixyLampSetReceiveBuffer};

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
void Pixy2Init(I2c_AddressType PixyI2cAddress, uint8 PixyI2cChannel){
    Pixy2Instance.I2cAddress = PixyI2cAddress;
    Pixy2Instance.I2cChannel = PixyI2cChannel;
}

void Pixy2SetLed(uint8 Red, uint8 Green, uint8 Blue){
    PixyLedSetCommand[4] = Red;
    PixyLedSetCommand[5] = Green;
    PixyLedSetCommand[6] = Blue;
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLedRequest);
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLedResponse);
}

/* Drive the integrated white headlights (0 = off, 1 = on). Turning the upper lamp on
 * forces a shorter exposure, which cuts motion blur and gives cleaner line edges. */
void Pixy2SetLamp(uint8 Upper, uint8 Lower){
    PixyLampSetCommand[4] = Upper;
    PixyLampSetCommand[5] = Lower;
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLampRequest);
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLampResponse);
}

void Pixy2GetVectors(DetectedVectors *DetectedVectors){
    uint16 Index = 6U, PacketLength, FeatureLength, VectorsNumber, CurrentVector = 0U, Offset;
    uint8  FeatureType;
    uint8_t* FeatureDataPointer;
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLinesRequest);
    I2c_SyncTransmit(Pixy2Instance.I2cChannel, &PixyLinesReceive);

    PacketLength = PixyReceivedLinesBuffer[3U] + 4U;
    DetectedVectors->NumberOfIntersections = 0U;   /* reset before this frame's parse */

    while(Index < PacketLength){
        FeatureType   = PixyReceivedLinesBuffer[Index];        /* NEW: read the type byte  */
        FeatureLength = PixyReceivedLinesBuffer[Index + 1U];
        FeatureDataPointer = &PixyReceivedLinesBuffer[Index + 2U];

        if(FeatureType == 1U){
            VectorsNumber = FeatureLength / 6U;
            for (uint8 FeatureDataIndex = 0U; FeatureDataIndex < VectorsNumber; FeatureDataIndex++) {
                Offset = FeatureDataIndex * 6U;
                DetectedVectors->Vectors[CurrentVector].x0 = FeatureDataPointer[Offset];
                DetectedVectors->Vectors[CurrentVector].y0 = FeatureDataPointer[Offset + 1U];
                DetectedVectors->Vectors[CurrentVector].x1 = FeatureDataPointer[Offset + 2U];
                DetectedVectors->Vectors[CurrentVector].y1 = FeatureDataPointer[Offset + 3U];
                DetectedVectors->Vectors[CurrentVector].VectorIndex = FeatureDataPointer[Offset + 4U];
                CurrentVector++;
            }
        }
        else if(FeatureType == 2U){
            uint16 SubOffset = 0U;
            while (SubOffset < FeatureLength) {
                uint8 BranchCount = FeatureDataPointer[SubOffset + 2U];
                if (BranchCount > 6U) {
                    BranchCount = 6U;   /* clamp -- guard against corrupt/garbage count */
                }

                if (DetectedVectors->NumberOfIntersections < 6U /* bound-check the output array too */) {
                    uint8 CurrentIntersection = DetectedVectors->NumberOfIntersections;
                    DetectedVectors->Intersections[CurrentIntersection].x = FeatureDataPointer[SubOffset + 0U];
                    DetectedVectors->Intersections[CurrentIntersection].y = FeatureDataPointer[SubOffset + 1U];
                    DetectedVectors->Intersections[CurrentIntersection].BranchCount = BranchCount;
                    for (uint8 BranchIndex = 0U; BranchIndex < BranchCount; BranchIndex++) {
                        DetectedVectors->Intersections[CurrentIntersection].BranchAngle[BranchIndex] =
                            FeatureDataPointer[SubOffset + 3U + BranchIndex * 2U];
                        DetectedVectors->Intersections[CurrentIntersection].BranchIndex[BranchIndex] =
                            FeatureDataPointer[SubOffset + 3U + BranchIndex * 2U + 1U];
                    }
                    DetectedVectors->NumberOfIntersections++;
                }

                SubOffset += 3U + BranchCount * 2U;   /* advance by this record's actual size */
            }
        }
        /* else: unknown feature type (e.g. barcode) -- length is still valid, so just skip its bytes */

        Index += 2U + FeatureLength;
    }
    DetectedVectors->NumberOfVectors = CurrentVector;
}

#ifdef __cplusplus
}
#endif

/** @} */
