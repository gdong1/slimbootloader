/** @file
  GpioV2SiLib implementation Arrow lake platform

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PciLib.h>
#include <Library/BootloaderCommonLib.h>

#include <Library/PchInfoLib.h>
#include <Register/PchRegsGpioVer2.h>

#include <Register/PchRegsPmcVer2.h>
#include <Register/PchRegsPmcVer4.h>
#include <Library/PchSbiAccessLib.h>

#include <Register/PchRegsGpioVer4.h>
#include <Library/PchInfoLib.h>

#include <GpioConfig.h>
#include <GpioV2Config.h>

#include <Register/PmcRegs.h>
#include <Register/PchRegsPcr.h>
#include "GpioPinsVer4S.h"
#include <Register/PchRegsGpio.h>
#include <Library/ConfigDataLib.h>
#include <PcrDefine.h>
#include <Register/GpioV2ChipsetId.h>
#include "MtlPchSTopology.h"
#include "MtlSocGpioTopology.h"


#define GPIO_PAD_DEF(Group,Pad)                (UINT32)(((Group) << 16) + (Pad))
#define GPIO_GROUP_DEF(GroupIndex,ChipsetId)   ((GroupIndex) | ((ChipsetId) << 8))
#define GPIO_GET_GROUP_INDEX(Group)            ((Group) & 0x1F)
#define GPIO_GET_GROUP_FROM_PAD(GpioPad)       (((GpioPad) & 0x0F1F0000) >> 16)
#define GPIO_GET_GROUP_INDEX_FROM_PAD(GpioPad) GPIO_GET_GROUP_INDEX (GPIO_GET_GROUP_FROM_PAD(GpioPad))
#define GPIO_GET_PAD_NUMBER(GpioPad)           ((GpioPad) & 0x1FF)
#define GPIO_GET_CHIPSET_ID(GpioPad)           (((GpioPad) >> 24) & 0xF)


GLOBAL_REMOVE_IF_UNREFERENCED GPIO_GROUP_TO_GPE_MAPPING mSocSGpioGroupToGpeMapping[] = {
  {GPIO_VER6_SOC_S_GROUP_GPP_A,   0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_GPP_A,     V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_GPP_A},
  {GPIO_VER6_SOC_S_GROUP_GPP_C,   0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_GPP_C,     V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_GPP_C},
  {GPIO_VER6_SOC_S_GROUP_GPP_B,   0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_GPP_B,     V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_GPP_B},
  {GPIO_VER6_SOC_S_GROUP_VGPIO_3, 0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_VGPIO_3,   V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_VGPIO_3},
  {GPIO_VER6_SOC_S_GROUP_VGPIO_4, 0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_VGPIO_4,   V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_VGPIO_4},
  {GPIO_VER6_SOC_S_GROUP_VGPIO_0, 0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_VGPIO_0,   V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_VGPIO_0},
  {GPIO_VER6_SOC_S_GROUP_GPP_D,   0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_GPP_D,     V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_GPP_D},
  {GPIO_VER6_SOC_S_GROUP_JTAG,    0, V_MTL_SOC_S_PMC_PWRM_GPIO_CFG_JTAG,      V_GPIO_VER6_SOC_S_GPIO_PCR_MISCCFG_GPE0_JTAG}
};

GLOBAL_REMOVE_IF_UNREFERENCED PCH_SBI_PID mGpioComSbiIds []=
{
  PID_GPIOCOM0, PID_GPIOCOM1, PID_GPIOCOM2, PID_GPIOCOM3, PID_GPIOCOM4, PID_GPIOCOM5
};

GLOBAL_REMOVE_IF_UNREFERENCED PCH_SBI_PID mSocMGpioComSbiIds[] =
{
  PID_GPIOCOM0, PID_GPIOCOM1, PID_GPIOCOM3, PID_GPIOCOM4, PID_GPIOCOM5
};


GLOBAL_REMOVE_IF_UNREFERENCED GPIO_GROUP_TO_GPE_MAPPING mSocMGpioGroupToGpeMapping[] = {
  {GPIO_VER6_SOC_M_GROUP_CPU,     0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_CPU,     V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_CPU},
  {GPIO_VER6_SOC_M_GROUP_GPP_V,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_V,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_V},
  {GPIO_VER6_SOC_M_GROUP_GPP_C,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_C,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_C},
  {GPIO_VER6_SOC_M_GROUP_GPP_A,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_A,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_A},
  {GPIO_VER6_SOC_M_GROUP_GPP_E,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_E,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_E},
  {GPIO_VER6_SOC_M_GROUP_GPP_H,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_H,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_H},
  {GPIO_VER6_SOC_M_GROUP_GPP_F,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_F,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_F},
  {GPIO_VER6_SOC_M_GROUP_SPI_SYS, 0, V_MTL_SOC_M_PMC_PWRM_GPIO_SPI_SYS,     V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_SPI_SYS},
  {GPIO_VER6_SOC_M_GROUP_USB_THC, 0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_USB,     V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_USB},
  {GPIO_VER6_SOC_M_GROUP_GPP_S,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_S,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_S},
  {GPIO_VER6_SOC_M_GROUP_JTAG,    0, V_MTL_SOC_M_PMC_PWRM_GPIO_JTAG,        V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_JTAG},
  {GPIO_VER6_SOC_M_GROUP_GPP_B,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_B,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_B},
  {GPIO_VER6_SOC_M_GROUP_GPP_D,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_GPP_D,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_GPP_D},
  {GPIO_VER6_SOC_M_GROUP_VGPIO,   0, V_MTL_SOC_M_PMC_PWRM_GPIO_CFG_VGPIO,   V_GPIO_VER6_SOC_M_GPIO_PCR_MISCCFG_GPE0_VGPIO}
};

//
// GPIO_PAD Fileds
//
typedef union {
  struct {
    UINT32    PadNum      :16;
    UINT32    GrpIdx      :8;
    UINT32    ChipsetId   :4;
    UINT32    Rsvd        :4;
  } PadField;
  UINT32      Pad;
} PAD_INFO;

//
// GPIO_CFG_DATA DW1 fields
//
typedef struct {
  UINT32    Rsvd1       :16;
  UINT32    PadNum      :8;
  UINT32    GrpIdx      :5;
  UINT32    Rsvd2       :3;
} GPIO_CFG_DATA_DW1;

/**
  This function gets Group to GPE0 configuration

  @param[out] GpeDw0Value       GPIO Group to GPE_DW0 assignment
  @param[out] GpeDw1Value       GPIO Group to GPE_DW1 assignment
  @param[out] GpeDw2Value       GPIO Group to GPE_DW2 assignment
**/
VOID
EFIAPI
PmcGetGpioGpe (
  OUT UINT32    *GpeDw0Value,
  OUT UINT32    *GpeDw1Value,
  OUT UINT32    *GpeDw2Value
  )
{
  UINT32 Data32;

  Data32 = MmioRead32 (PCH_PWRM_BASE_ADDRESS + R_PMC_PWRM_GPIO_CFG);

  *GpeDw0Value = ((Data32 & B_PMC_PWRM_GPIO_CFG_GPE0_DW0) >> N_PMC_PWRM_GPIO_CFG_GPE0_DW0);
  *GpeDw1Value = ((Data32 & B_PMC_PWRM_GPIO_CFG_GPE0_DW1) >> N_PMC_PWRM_GPIO_CFG_GPE0_DW1);
  *GpeDw2Value = ((Data32 & B_PMC_PWRM_GPIO_CFG_GPE0_DW2) >> N_PMC_PWRM_GPIO_CFG_GPE0_DW2);
}

/**
  Return opcode supported for writing to GPIO lock unlock register

  @retval UINT8   Lock Opcode
**/
UINT8
EFIAPI
GpioGetLockOpcode (
  VOID
  )
{
  // if (IsPchS ()) {
  //   return PrivateControlWrite;
  // } else if (IsPchP ()) {
    // return GpioLockUnlock;
    return GpioLockUnlock;
  // } else {
  //   ASSERT (FALSE);
  //   return 0;
  // }
}

/**
  This internal procedure will check if group is within DeepSleepWell.

  @param[in]  Group               GPIO Group

  @retval GroupWell               TRUE:  This is DSW Group
                                  FALSE: This is not DSW Group
**/
BOOLEAN
EFIAPI
GpioIsDswGroup (
  IN  GPIO_GROUP         Group
  )
{
  // if ((Group == GPIO_VER4_S_GROUP_GPD) || (Group == GPIO_VER2_LP_GROUP_GPD)) {
  //   return TRUE;
  // } else {
  //   return FALSE;
  // }
  return FALSE;
}

/**
  This procedure will retrieve address and length of GPIO info table

  @param[out]  GpioGroupInfoTableLength   Length of GPIO group table

  @retval Pointer to GPIO group table

**/
/*
CONST GPIO_GROUP_INFO*
EFIAPI
GpioGetGroupInfoTable (
  OUT UINT32              *GpioGroupInfoTableLength
  )
{
  if (MtlIsSocM ()) {
    *GpioGroupInfoTableLength = ARRAY_SIZE (mSocMGpioGroupInfo);
    return mSocMGpioGroupInfo;
  } else {
    *GpioGroupInfoTableLength = ARRAY_SIZE (mSocSGpioGroupInfo);
    return mSocSGpioGroupInfo;
  }
}
*/


/**
  Get GPIO Chipset ID specific to PCH generation and series
**/
UINT32
EFIAPI
GpioGetThisChipsetId (
  VOID
  )
{
  if (MtlIsSocM ()) {
    return GPIOV2_MTL_SOC_M_CHIPSET_ID;
  } else if (MtlIsSocS ()) {
    return GPIOV2_MTL_SOC_S_CHIPSET_ID | GPIOV2_MTL_PCH_S_CHIPSET_ID;
  } else {
    return 0;
  }
}


/**
  Get information for GPIO Group required to program GPIO and PMC for desired 1-Tier GPE mapping

  @param[out] GpioGroupToGpeMapping        Table with GPIO Group to GPE mapping
  @param[out] GpioGroupToGpeMappingLength  GPIO Group to GPE mapping table length
**/
VOID
EFIAPI
GpioGetGroupToGpeMapping (
  OUT GPIO_GROUP_TO_GPE_MAPPING  **GpioGroupToGpeMapping,
  OUT UINT32                     *GpioGroupToGpeMappingLength
  )
{
  if (MtlIsSocM ()) {
    *GpioGroupToGpeMapping = mSocMGpioGroupToGpeMapping;
    *GpioGroupToGpeMappingLength = ARRAY_SIZE (mSocMGpioGroupToGpeMapping);
  } else {
    *GpioGroupToGpeMapping = mSocSGpioGroupToGpeMapping;
    *GpioGroupToGpeMappingLength = ARRAY_SIZE (mSocSGpioGroupToGpeMapping);
  }
}


/**
  This procedure will get Gpio Pad from Cfg Dword

  @param[in]  GpioItem         Pointer to the Gpio Cfg Data Item
  @param[out] GpioPad          Gpio Pad
**/
VOID
EFIAPI
GpioGetGpioPadFromCfgDw (
  IN  UINT32            *GpioItem,
  OUT GPIO_PAD          *GpioPad
  )
{
  GPIO_CFG_DATA_DW1     *Dw1;
  PAD_INFO              PadInfo;

  Dw1 = (GPIO_CFG_DATA_DW1 *) (&GpioItem[1]);

  PadInfo.PadField.PadNum    = (UINT16) Dw1->PadNum;
  PadInfo.PadField.GrpIdx    = (UINT8)  Dw1->GrpIdx;
  PadInfo.PadField.ChipsetId = GpioGetThisChipsetId ();
  *GpioPad = PadInfo.Pad;

  //
  // Remove PadInfo data from DW1
  //
  Dw1->PadNum = 0;
  Dw1->GrpIdx = 0;
}



//
// Die structure
//
GPIOV2_CONTROLLER   mGpipController[] = {
  {
    // GPIO in PCH
    .CommunityNum = sizeof (MtlPchSCommunities) / sizeof (GPIOV2_COMMUNITY),
    .Communities  = MtlPchSCommunities,
    .ChipsetId    = GPIOV2_MTL_PCH_S_CHIPSET_ID,
    .P2sbBase     = PCI_LIB_ADDRESS(0x80, 31, 1, 0),
    .SbRegBar     = 0  //Should be updated when SBREG_BAR is set
  },
  {
    // GPIO in SOC-S
    .CommunityNum = sizeof (MtlSocSCommunities) / sizeof (GPIOV2_COMMUNITY),
    .Communities  = MtlSocSCommunities,
    .ChipsetId    = GPIOV2_MTL_SOC_S_CHIPSET_ID,
    .P2sbBase     = PCI_LIB_ADDRESS(0, 31, 1, 0),
    .SbRegBar     = PCH_PCR_BASE_ADDRESS
  },
  {
    // GPIO in SOC-M
    .CommunityNum = sizeof (MtlSocMCommunities) / sizeof (GPIOV2_COMMUNITY),
    .Communities  = MtlSocMCommunities,
    .ChipsetId    = GPIOV2_MTL_SOC_M_CHIPSET_ID,
    .P2sbBase     = PCI_LIB_ADDRESS(0, 31, 1, 0),
    .SbRegBar     = PCH_PCR_BASE_ADDRESS
  }

};


/**
  This procedure retrieves pointer a P2SB controller where PAD belong to

  @param[in] GpioPad              GPIO PAD

  @retval                         A P2SB controller pointer
**/
GPIOV2_CONTROLLER *
EFIAPI
GpioGetController (
  IN GPIOV2_PAD             GpioPad
  )
{
  UINT16                   ChipsetId;
  UINTN                    Index;

  ChipsetId = GPIOV2_PAD_GET_CHIPSETID (GpioPad);

  for (Index = 0; Index < sizeof (mGpipController) / sizeof (GPIOV2_CONTROLLER); Index++) {
    if (mGpipController[Index].ChipsetId == ChipsetId) {
      return &mGpipController[Index];
    }
  }
  ASSERT (FALSE);
  return NULL;  
}

