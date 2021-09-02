/** @file

  Copyright (c) 2016 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Stage2.h"

UINT8   mFspPhaseMask;

// Create a platform service
const PLATFORM_SERVICE   mPlatformService = {
  .Header.Signature = PLATFORM_SERVICE_SIGNATURE,
  .Header.Version   = PLATFORM_SERVICE_VERSION,
  .NotifyPhase      = BoardNotifyPhase
};


/**
  Platform notify service.

  @param[in] Phase          Boot phase to notify.

**/
VOID
EFIAPI
BoardNotifyPhase (
  IN BOARD_INIT_PHASE   Phase
  )
{
  FSP_INIT_PHASE  FspPhase;
  UINT8           FspPhaseMask;
  EFI_STATUS      Status;

  // This is board notification from payload
  FspPhaseMask = 0;
  if (Phase == PostPciEnumeration) {
    FspPhase     = EnumInitPhaseAfterPciEnumeration;
    FspPhaseMask = BIT0;
  } else if (Phase == ReadyToBoot) {
    FspPhase     = EnumInitPhaseReadyToBoot;
    FspPhaseMask = BIT1;
  } else if (Phase == EndOfFirmware) {
    FspPhase = EnumInitPhaseEndOfFirmware;
    FspPhaseMask = BIT2;
  }

  if (FspPhaseMask != 0) {

    if (FspPhaseMask & BIT1) {
      // Sigal MP to stop
      MpInit (EnumMpInitDone);
    }

    if ((FspPhaseMask & mFspPhaseMask) == 0) {
      // Only call FSP notify once
      Status = CallFspNotifyPhase (FspPhase);
      FspResetHandler (Status);
      ASSERT_EFI_ERROR (Status);

      mFspPhaseMask |= FspPhaseMask;
    }

    if (FspPhaseMask & (BIT1 | BIT2)) {
      // Only call board init for ReadyToBoot & EndOfFirmware
      BoardInit (Phase);
    }

    if (FspPhaseMask & BIT2) {
      InitializeDebugAgent (DEBUG_AGENT_INIT_DXE_UNLOAD, NULL, NULL);
    }
  }

}

/**
  Display graphical splash screen

  @retval EFI_SUCCESS     Splash screen was successfully displayed
  @retval EFI_NOT_FOUND   Frame buffer hob not found
  @retval EFI_UNSUPPORTED BmpImage is not a valid *.BMP image

**/
EFI_STATUS
EFIAPI
DisplaySplash (
  VOID
  )
{
  EFI_STATUS                           Status;
  VOID                                *SplashLogoBmp;
  EFI_PEI_GRAPHICS_INFO_HOB           *GfxInfoHob;

  // Get framebuffer info
  GfxInfoHob = (EFI_PEI_GRAPHICS_INFO_HOB *)GetGuidHobData (NULL, NULL, &gEfiGraphicsInfoHobGuid);
  if (GfxInfoHob == NULL) {
    return EFI_NOT_FOUND;
  }

  // Convert image from BMP format and write to frame buffer
  SplashLogoBmp = (VOID *)(UINTN)PCD_GET32_WITH_ADJUST (PcdSplashLogoAddress);
  ASSERT (SplashLogoBmp != NULL);
  Status = DisplayBmpToFrameBuffer (SplashLogoBmp, NULL, 0, GfxInfoHob);

  return Status;
}

/**
  Print out the current memory map information

  @param  MemoryMapInfo    MEMORY_MAP_ENTRY array containing the system momory map.

**/
VOID
PrintMemoryMap (
  IN MEMORY_MAP_INFO      *MemoryMapInfo
  )
{
  UINTN  Idx;

  for (Idx = 0; Idx < MemoryMapInfo->Count; Idx++) {
    DEBUG ((DEBUG_INFO, "MEM: %016lX %016lX %02X %d\n", \
            MemoryMapInfo->Entry[Idx].Base,  MemoryMapInfo->Entry[Idx].Size, \
            MemoryMapInfo->Entry[Idx].Flag,  MemoryMapInfo->Entry[Idx].Type));
  }
}

/**
  Translate the resource HOB into memory map data structure.

  @param ResourceDescriptor     Pointer to a resource HOB.

  @param Param                  Array of MemoryMapInfo structure containing memory map entry.
**/
VOID
EFIAPI
MemResHobCallback (
  IN EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor,
  IN VOID                         *Param
  )
{
  MEMORY_MAP_INFO      *MemoryMapInfo;
  UINTN                 Idx;

  MemoryMapInfo = (MEMORY_MAP_INFO *)Param;
  Idx           = MemoryMapInfo->Count;
  if (Idx < PcdGet32 (PcdMemoryMapEntryNumber)) {
    MemoryMapInfo->Entry[Idx].Base = ResourceDescriptor->PhysicalStart;
    MemoryMapInfo->Entry[Idx].Size = ResourceDescriptor->ResourceLength;
    MemoryMapInfo->Entry[Idx].Type = (ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) ? 1 : 2;
    MemoryMapInfo->Entry[Idx].Flag = 0;
    MemoryMapInfo->Count++;
  } else {
    ASSERT (FALSE);
  }
}

/**
  The function is called by PerformQuickSort to sort memory map by its base.

  @param[in] Buffer1         The pointer to first buffer.
  @param[in] Buffer2         The pointer to second buffer.

  @retval 0                  Buffer1 base is less than Buffer2 base.
  @retval 1                  Buffer1 base is greater than or equal to Buffer2 base.

**/
INTN
EFIAPI
CompareMemoryMap (
  IN CONST VOID                 *Buffer1,
  IN CONST VOID                 *Buffer2
  )
{
  if (((MEMORY_MAP_ENTRY *)Buffer1)->Base < ((MEMORY_MAP_ENTRY *)Buffer2)->Base) {
    return  0;
  } else {
    return  1;
  }
}

/**
  Carve out a bootloader reserved, AcpiNvs, AcpiRecalim and payload reserved memory range
  from the system memory map.

  @param MemoryMapInfo     Array of MemoryMapInfo structure containing memory map entry.

  @retval EFI_SUCCESS           Memory ranges have been splitted successfully.
  @retval EFI_UNSUPPORTED       Required memory range size is not page aligned.
  @retval EFI_OUT_OF_RESOURCES  The system memory map top entry is too small.
  @retval EFI_ABORTED           No memory range has been splitted.

**/
EFI_STATUS
SplitMemroyMap (
  IN OUT  MEMORY_MAP_INFO      *MemoryMapInfo
  )
{
  UINTN            Idx;
  UINT32           NewIdx;
  UINTN            Loop;
  UINT8            Flag;
  UINT8            Type;
  UINT32           Adjust;
  MEMORY_MAP_ENTRY     TempMemoryMap;
  LOADER_GLOBAL_DATA  *LdrGlobal;

  LdrGlobal = (LOADER_GLOBAL_DATA *)GetLoaderGlobalDataPointer();
  NewIdx = MemoryMapInfo->Count;

  for (Idx = 0; Idx < MemoryMapInfo->Count; Idx++) {
    if ((UINT64)LdrGlobal->StackTop == (MemoryMapInfo->Entry[Idx].Base + MemoryMapInfo->Entry[Idx].Size)) {
      //
      // Add bootloader and payload reserved memory map entry
      //
      for (Loop = 0; (Loop < 4) && (NewIdx < PcdGet32 (PcdMemoryMapEntryNumber)); Loop++) {
        Flag   = 0;
        Type   = MEM_MAP_TYPE_RESERVED;
        switch (Loop) {
        case 0:
          // Bootloader reserved memory
          Adjust = LdrGlobal->StackTop - LdrGlobal->MemPoolStart;
          break;
        case 1:
          // ACPI NVS memory
          Adjust = PcdGet32 (PcdLoaderAcpiNvsSize);
          Type   = MEM_MAP_TYPE_ACPI_NVS;
          break;
        case 2:
          // ACPI Reclaim memory
          Adjust = PcdGet32 (PcdLoaderAcpiReclaimSize);
          Type   = MEM_MAP_TYPE_ACPI_RECLAIM;
          break;
        case 3:
          // Payload reserved memory
          Adjust = PcdGet32 (PcdPayloadReservedMemSize);
          Flag   = MEM_MAP_FLAG_PAYLOAD;
          break;
        default:
          Adjust = 0;
          break;
        }
        if (Adjust == 0) {
          continue;
        }
        // Adjust size must be page aligned
        if ((Adjust & (EFI_PAGE_SIZE - 1)) != 0) {
          return EFI_UNSUPPORTED;
        }
        if (MemoryMapInfo->Entry[Idx].Size < Adjust) {
          return EFI_OUT_OF_RESOURCES;
        }
        MemoryMapInfo->Entry[Idx].Size   -= Adjust;
        MemoryMapInfo->Entry[NewIdx].Base = MemoryMapInfo->Entry[Idx].Base + MemoryMapInfo->Entry[Idx].Size;
        MemoryMapInfo->Entry[NewIdx].Size = Adjust;
        MemoryMapInfo->Entry[NewIdx].Type = Type;
        MemoryMapInfo->Entry[NewIdx].Flag = Flag;
        NewIdx++;
      }
      break;
    }
  }

  MemoryMapInfo->Entry[NewIdx].Base = PcdGet32(PcdFlashBaseAddress);
  MemoryMapInfo->Entry[NewIdx].Size = PcdGet32(PcdFlashSize);
  MemoryMapInfo->Entry[NewIdx].Type = MEM_MAP_TYPE_RESERVED;
  MemoryMapInfo->Entry[NewIdx].Flag = 0;
  NewIdx++;

  if (NewIdx <= MemoryMapInfo->Count) {
    return EFI_ABORTED;
  }
  MemoryMapInfo->Count = NewIdx;

  PerformQuickSort (MemoryMapInfo->Entry, NewIdx, sizeof (MEMORY_MAP_ENTRY), CompareMemoryMap, &TempMemoryMap);

  return EFI_SUCCESS;
}


/**
  Carve out a bootloader reserved, AcpiNvs, AcpiRecalim and payload reserved memory range
  from the system memory map.

  @param MemoryMapInfo     Array of MemoryMapInfo structure containing memory map entry.

  @retval EFI_SUCCESS           Memory ranges have been splitted successfully.
  @retval EFI_UNSUPPORTED       Required memory range size is not page aligned.
  @retval EFI_OUT_OF_RESOURCES  The system memory map top entry is too small.
  @retval EFI_ABORTED           No memory range has been splitted.

**/
EFI_STATUS
BuildMemroyHobs (
  IN   MEMORY_MAP_INFO         *MemoryMapInfo
  )
{
  UINTN                        Idx;
  MEMORY_MAP_ENTRY             *MapEntry;
  EFI_RESOURCE_ATTRIBUTE_TYPE  Attribue;
  EFI_RESOURCE_TYPE            ResourceType;

  Attribue = EFI_RESOURCE_ATTRIBUTE_PRESENT |
             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
             EFI_RESOURCE_ATTRIBUTE_TESTED |
             EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE;

  for (Idx = 0; Idx < MemoryMapInfo->Count; Idx++) {
    MapEntry     = &MemoryMapInfo->Entry[Idx];
    ResourceType = EFI_RESOURCE_SYSTEM_MEMORY;
    switch (MapEntry->Type) {
    case MEM_MAP_TYPE_ACPI_RECLAIM:
      BuildMemoryAllocationHob (MapEntry->Base, MapEntry->Size, EfiACPIReclaimMemory);
      break;
    case MEM_MAP_TYPE_ACPI_NVS:
      BuildMemoryAllocationHob (MapEntry->Base, MapEntry->Size, EfiACPIMemoryNVS);
      break;
    case MEM_MAP_TYPE_RESERVED:
      ResourceType = EFI_RESOURCE_MEMORY_RESERVED;
      break;
    default:
      break;
    }
    BuildResourceDescriptorHob (ResourceType, Attribue, MapEntry->Base, MapEntry->Size);
  }

  return EFI_SUCCESS;
}

/**
  Build some basic HOBs

  After silicon initialization, the information is available
  to build some basic HOBs. These HOBs could be used/updated
  by stage2 code, or used by payload.

  @param Stage2Param         Stage2 Param pointer.

  @return                  The HOB list pointer.
**/
VOID *
EFIAPI
BuildBaseInfoHob (
  IN  STAGE2_PARAM                     *Stage2Param
  )
{
  SERIAL_PORT_INFO                     *SerialPortInfo;
  LOADER_FSP_INFO                      *LoaderFspInfo;
  MEMORY_MAP_INFO                      *MemoryMapInfo;
  UINT32                               Length;
  LOADER_GLOBAL_DATA                   *LdrGlobal;
  EFI_PEI_GRAPHICS_INFO_HOB            *FspGfxHob;
  EFI_PEI_GRAPHICS_INFO_HOB            *BlGfxHob;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB     *BlGfxDeviceInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GfxMode;
  EFI_STATUS                           Status;
  UINT8*                               *HashHob;
  UINT32                               HobDataSize;
  EXT_BOOT_LOADER_VERSION              *VersionHob;
  SEED_LIST_INFO_HOB                   *SeedListInfoHob;
  HASH_STORE_TABLE                     *HashStorePtr;

  LdrGlobal = (LOADER_GLOBAL_DATA *)GetLoaderGlobalDataPointer();

  // Build Loader FSP info hob
  LoaderFspInfo = BuildGuidHob (&gLoaderFspInfoGuid, sizeof (LOADER_FSP_INFO));
  if (LoaderFspInfo != NULL) {
    LoaderFspInfo->FspsBase   = PCD_GET32_WITH_ADJUST (PcdFSPSBase);
    LoaderFspInfo->FspHobList = LdrGlobal->FspHobList;
  }

  // Build serial port hob
  SerialPortInfo = BuildGuidHob (&gLoaderSerialPortInfoGuid, sizeof (SERIAL_PORT_INFO));
  if (SerialPortInfo != NULL) {
    SerialPortInfo->Type        = 1;
    SerialPortInfo->BaseAddr    = 0x3F8;
    SerialPortInfo->Baud        = 115200;
    SerialPortInfo->RegWidth    = 1;
    SerialPortInfo->InputHertz  = 1843200;
    SerialPortInfo->UartPciAddr = 0;
  }

  // Build graphic info hob
  BlGfxHob = (EFI_PEI_GRAPHICS_INFO_HOB *)GetGuidHobData (NULL, &Length, &gEfiGraphicsInfoHobGuid);
  if (BlGfxHob == NULL) {
    FspGfxHob = (EFI_PEI_GRAPHICS_INFO_HOB *)GetGuidHobData (LdrGlobal->FspHobList, &Length,
                  &gEfiGraphicsInfoHobGuid);
    if (FspGfxHob != NULL) {
      BlGfxHob = BuildGuidHob (&gEfiGraphicsInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));
      if (BlGfxHob != NULL) {
        CopyMem (BlGfxHob, FspGfxHob, sizeof (EFI_PEI_GRAPHICS_INFO_HOB));
        GfxMode = &BlGfxHob->GraphicsMode;
        DEBUG ((DEBUG_INFO, "Graphics Info: %d x %d x 32 @ 0x%08X\n",GfxMode->HorizontalResolution,\
          GfxMode->VerticalResolution, BlGfxHob->FrameBufferBase));
        if ((GfxMode->PixelFormat != PixelRedGreenBlueReserved8BitPerColor) &&
            (GfxMode->PixelFormat != PixelBlueGreenRedReserved8BitPerColor)) {
          DEBUG ((DEBUG_ERROR, "Graphics PixelFormat NOT expected (0x%x)\n", GfxMode->PixelFormat));
        }
      }
    } else {
      DEBUG ((DEBUG_INFO, "Failed to get GFX HOB from FSP\n"));
    }
  } else {
    DEBUG ((DEBUG_INFO, "Use existing GFX HOB from bootloader\n"));
  }

  // Build graphic device info hob
  BlGfxDeviceInfo = BuildGuidHob (&gEfiGraphicsDeviceInfoHobGuid, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB));
  if (BlGfxDeviceInfo != NULL) {
    SetMem (BlGfxDeviceInfo, sizeof (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB), 0xFF);
  }

  // Build Memory Map hob
  Length = sizeof (MEMORY_MAP_INFO) + sizeof (MEMORY_MAP_ENTRY) * PcdGet32 (PcdMemoryMapEntryNumber);
  MemoryMapInfo = BuildGuidHob (&gLoaderMemoryMapInfoGuid, Length);
  if (MemoryMapInfo != NULL) {
    SetMem (MemoryMapInfo, Length, 0);
    MemoryMapInfo->Revision = 1;
    TraverseMemoryResourceHob (LdrGlobal->FspHobList, MemResHobCallback, MemoryMapInfo);
    Status = SplitMemroyMap (MemoryMapInfo);
    BuildMemroyHobs (MemoryMapInfo);
    PrintMemoryMap (MemoryMapInfo);
    if (EFI_ERROR (Status)) {
      CpuHaltWithStatus ("Memory map failure !", Status);
    }
  }

  // Build key hash Hob for Payload
  if (LdrGlobal->HashStorePtr != NULL) {

    HashStorePtr = (HASH_STORE_TABLE *)LdrGlobal->HashStorePtr;
    HashHob      = BuildGuidHob (&gPayloadKeyHashGuid, HashStorePtr->UsedLength);

    //Copy the hash store to key hash Hob
    CopyMem (HashHob, (UINT8 *) HashStorePtr, HashStorePtr->UsedLength);

  }

  // Build boot loader version Hob for Payload
  if (LdrGlobal->VerInfoPtr != NULL) {
    HobDataSize = sizeof (EXT_BOOT_LOADER_VERSION);
    VersionHob  = BuildGuidHob (&gBootLoaderVersionGuid, HobDataSize);
    if (VersionHob != NULL) {
      VersionHob->Revision = 1;
      ZeroMem (&VersionHob->Reserved0, sizeof (VersionHob->Reserved0));
      CopyMem (&VersionHob->Version, LdrGlobal->VerInfoPtr, sizeof (BOOT_LOADER_VERSION));
    }
  }

  // Build Seed List Info hob for Payload
  if (PcdGetBool (PcdSeedListEnabled)) {
    Length = PcdGet32 (PcdSeedListBufferSize);
    SeedListInfoHob = BuildGuidHob (&gSeedListInfoHobGuid, Length);
    if (SeedListInfoHob != NULL) {
      ZeroMem (SeedListInfoHob, Length);
      SeedListInfoHob->Revision       = 1;
      SeedListInfoHob->TotalSeedCount = 0;
      SeedListInfoHob->BufferSize     = Length;
    }
  }
  return LdrGlobal->LdrHobList;
}

/**
  Build and update HOBs.

  Before jumping to payload, more information is available, so update some HOBs
  built early, and build more HOBs for payload.

  @param Stage2Param         Stage2 Param pointer.

  @return                  The HOB list pointer.
**/
VOID *
EFIAPI
BuildExtraInfoHob (
  IN  STAGE2_PARAM                 *Stage2Param
  )
{
  LOADER_GLOBAL_DATA               *LdrGlobal;
  S3_DATA                          *S3Data;
  SERIAL_PORT_INFO                 *SerialPortInfo;
  SYSTEM_TABLE_INFO                *SystemTableInfo;
  SYS_CPU_INFO                     *SysCpuInfo;
  PERFORMANCE_INFO                 *PerformanceInfo;
  OS_BOOT_OPTION_LIST              *OsBootOptionInfo;
  EFI_PEI_GRAPHICS_INFO_HOB        *GfxInfoHob;
  EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *GfxDeviceInfoHob;
  LOADER_PLATFORM_INFO             *LoaderPlatformInfo;
  LOADER_PLATFORM_DATA             *LoaderPlatformData;
  LOADER_LIBRARY_DATA              *LoaderLibData;
  UINT32                           Count;
  UINT32                           Length;
  BOOT_LOADER_SERVICES_LIST        *HobServicesList;
  SERVICES_LIST                    *PldServicesList;
  SERVICES_LIST                    *CoreServicesList;
  FLASH_MAP                        *FlashMapPtr;
  FLASH_MAP                        *FlashMapHob;
  UINTN                            Index;
  SEED_LIST_INFO_HOB               *SeedListInfoHob;
  PLT_DEVICE_TABLE                 *DeviceTable;
  VOID                             *DeviceTableHob;
  LDR_SMM_INFO                     *SmmInfoHob;
  SYS_CPU_TASK_HOB                 *SysCpuTaskHob;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *PldSerialPortInfo;
  UINT32                           RegEax;
  UINT8                            PhysicalAddressBits;
  EFI_RESOURCE_ATTRIBUTE_TYPE      ResourceAttribute;

  LdrGlobal = (LOADER_GLOBAL_DATA *)GetLoaderGlobalDataPointer();
  S3Data    = (S3_DATA *)LdrGlobal->S3DataPtr;

  // Build library data hob
  LoaderLibData = BuildGuidHob (&gLoaderLibraryDataGuid, sizeof (LOADER_LIBRARY_DATA));
  if (LoaderLibData != NULL) {
    LoaderLibData->Count = (UINT16)PcdGet32 (PcdMaxLibraryDataEntry);
    LoaderLibData->Flags = 0;
    LoaderLibData->Data  = LdrGlobal->LibDataPtr;
  }

  // Update graphic info HOB
  GfxInfoHob = (EFI_PEI_GRAPHICS_INFO_HOB *)GetGuidHobData (NULL, NULL, &gEfiGraphicsInfoHobGuid);
  if (GfxInfoHob != NULL) {
    PlatformUpdateHobInfo (&gEfiGraphicsInfoHobGuid, GfxInfoHob);
  }

  // Update graphic device info HOB
  GfxDeviceInfoHob = (EFI_PEI_GRAPHICS_DEVICE_INFO_HOB *)GetGuidHobData (NULL, NULL, &gEfiGraphicsDeviceInfoHobGuid);
  if (GfxDeviceInfoHob != NULL) {
    PlatformUpdateHobInfo (&gEfiGraphicsDeviceInfoHobGuid, GfxDeviceInfoHob);
  }

  // Update serial port hob
  SerialPortInfo = (SERIAL_PORT_INFO *)GetGuidHobData (NULL, NULL, &gLoaderSerialPortInfoGuid);
  if (SerialPortInfo != NULL) {
    PlatformUpdateHobInfo (&gLoaderSerialPortInfoGuid, SerialPortInfo);
  }

  // Build ACPI Hob
  SystemTableInfo = BuildGuidHob (&gLoaderSystemTableInfoGuid, sizeof (SYSTEM_TABLE_INFO));
  if (SystemTableInfo != NULL) {
    SystemTableInfo->AcpiTableBase = S3Data->AcpiBase;
    SystemTableInfo->AcpiTableSize = S3Data->AcpiTop - S3Data->AcpiBase;
    SystemTableInfo->SmbiosTableBase = (UINT64)PcdGet32 (PcdSmbiosTablesBase);
    SystemTableInfo->SmbiosTableSize = (UINT32)PcdGet16 (PcdSmbiosTablesSize);
    PlatformUpdateHobInfo (&gLoaderSystemTableInfoGuid, SystemTableInfo);
  }

  // Build Loader Platform Data Hob
  LoaderPlatformData = BuildGuidHob (&gLoaderPlatformDataGuid, sizeof (LOADER_PLATFORM_DATA));
  if (LoaderPlatformData != NULL) {
    LoaderPlatformData->Revision = 1;
    LoaderPlatformData->DebugLogBuffer = (DEBUG_LOG_BUFFER_HEADER *) GetDebugLogBufferPtr ();
    LoaderPlatformData->ConfigDataPtr  = GetConfigDataPtr ();
    LoaderPlatformData->ContainerList  = GetContainerListPtr ();
    LoaderPlatformData->DmaBufferPtr   = GetDmaBufferPtr ();
  }

  // Build flash map info hob
  FlashMapPtr = LdrGlobal->FlashMapPtr;
  FlashMapHob = BuildGuidHob (&gFlashMapInfoGuid, FlashMapPtr->Length);
  if (FlashMapHob != NULL) {
    CopyMem (FlashMapHob, FlashMapPtr, FlashMapPtr->Length);
  }

  // Build OS boot medium info hob
  Length = sizeof (OS_BOOT_OPTION_LIST) + sizeof (OS_BOOT_OPTION) * PcdGet32 (PcdOsBootOptionNumber);
  OsBootOptionInfo = BuildGuidHob (&gOsBootOptionGuid, Length);
  if (OsBootOptionInfo != NULL) {
    ZeroMem (OsBootOptionInfo, Length);
    OsBootOptionInfo->Revision    = 1;
    OsBootOptionInfo->ResetReason = GetResetReason ();
    PlatformUpdateHobInfo (&gOsBootOptionGuid, OsBootOptionInfo);
  }

  // Update seed list hob
  SeedListInfoHob = (SEED_LIST_INFO_HOB *)GetGuidHobData (NULL, NULL, &gSeedListInfoHobGuid);
  if (SeedListInfoHob != NULL) {
    PlatformUpdateHobInfo (&gSeedListInfoHobGuid, SeedListInfoHob);
  }

  // Build boot loader services hob
  Length          = sizeof (BOOT_LOADER_SERVICES_LIST) + sizeof (SERVICE_COMMON_HEADER *) * PcdGet32 (PcdServiceNumber);
  HobServicesList = BuildGuidHob (&gBootLoaderServiceGuid, Length);
  if (HobServicesList != NULL) {
    ZeroMem (HobServicesList, Length);
    HobServicesList->Revision = 1;
    PldServicesList  = &HobServicesList->ServiceList;
    CoreServicesList = (SERVICES_LIST *)GetServiceListPtr ();
    for (Index = 0; Index < CoreServicesList->Count; Index++) {
      if (PldServicesList->Count < PcdGet32 (PcdServiceNumber)) {
        if (CoreServicesList->Header[Index] != NULL) {
          PldServicesList->Header[PldServicesList->Count] = CoreServicesList->Header[Index];
          PldServicesList->Count++;
        }
      }
    }
  }

  // Build device table Hob
  DeviceTable  = (PLT_DEVICE_TABLE *)LdrGlobal->DeviceTable;
  Length = sizeof (PLT_DEVICE_TABLE) + sizeof (PLT_DEVICE) * DeviceTable->DeviceNumber;
  DeviceTableHob = BuildGuidHob (&gDeviceTableHobGuid, Length);
  if (DeviceTableHob != NULL) {
    CopyMem (DeviceTableHob, DeviceTable, Length);
  }

  // Build SMMRAM info Hob
  SmmInfoHob = BuildGuidHob (&gSmmInformationGuid, sizeof (LDR_SMM_INFO));
  if (SmmInfoHob != NULL) {
    ZeroMem (SmmInfoHob, sizeof (LDR_SMM_INFO));
    PlatformUpdateHobInfo (&gSmmInformationGuid, SmmInfoHob);
  }

  // Build Performance Hob
  Count  = LdrGlobal->PerfData.PerfIndex;
  Length = sizeof (PERFORMANCE_INFO) + sizeof (UINT64) * Count;
  PerformanceInfo = BuildGuidHob (&gLoaderPerformanceInfoGuid, Length);
  if (PerformanceInfo != NULL) {
    PerformanceInfo->Revision = 1;
    PerformanceInfo->Count = (UINT16)LdrGlobal->PerfData.PerfIndex;
    PerformanceInfo->Frequency = LdrGlobal->PerfData.FreqKhz;
    PerformanceInfo->Flags = 0;
    CopyMem (PerformanceInfo->TimeStamp, LdrGlobal->PerfData.TimeStamp, sizeof (UINT64) * Count);
  }

  // Build Loader Platform info Hob
  Length       = sizeof (LOADER_PLATFORM_INFO);
  LoaderPlatformInfo = BuildGuidHob (&gLoaderPlatformInfoGuid, Length);
  if (LoaderPlatformInfo != NULL) {
    ZeroMem (LoaderPlatformInfo, Length);
    LoaderPlatformInfo->Revision = 2;
    LoaderPlatformInfo->BootPartition = GetCurrentBootPartition ();
    LoaderPlatformInfo->BootMode = GetBootMode ();
    LoaderPlatformInfo->PlatformId = GetPlatformId ();
    LoaderPlatformInfo->LdrFeatures = LdrGlobal->LdrFeatures;
    SysCpuInfo = MpGetInfo ();
    LoaderPlatformInfo->CpuCount = SysCpuInfo->CpuCount;
    PlatformUpdateHobInfo (&gLoaderPlatformInfoGuid, LoaderPlatformInfo);
  }

  // Build MP CPU task info Hob
  if (GetPayloadId () == 0) {
    Length        = sizeof (SYS_CPU_TASK_HOB);
    SysCpuTaskHob = BuildGuidHob (&gLoaderMpCpuTaskInfoGuid, Length);
    ZeroMem (SysCpuTaskHob, sizeof (SYS_CPU_TASK_HOB));
    if (SysCpuTaskHob != NULL) {
      SysCpuTaskHob->SysCpuTask = (UINTN) MpGetTask();
    }
  }

  //
  // Build HOBs for universal payload
  //

  // ACPI table HOB
  UNIVERSAL_PAYLOAD_ACPI_TABLE   *AcpiHob;
  AcpiHob = BuildGuidHob (&gUniversalPayloadAcpiTableGuid, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
  if (AcpiHob != NULL) {
    ZeroMem (AcpiHob, sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE));
    AcpiHob->Header.Revision = UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION;
    AcpiHob->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_ACPI_TABLE);
    AcpiHob->Rsdp            = S3Data->AcpiBase;
  }

  // SMBIOS table HOB
  UNIVERSAL_PAYLOAD_SMBIOS_TABLE   *SmbiosHob;
  SmbiosHob = BuildGuidHob (&gUniversalPayloadSmbiosTableGuid, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE));
  if (SmbiosHob != NULL) {
    ZeroMem (SmbiosHob, sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE));
    SmbiosHob->Header.Revision  = UNIVERSAL_PAYLOAD_SMBIOS_TABLE_REVISION;
    SmbiosHob->Header.Length    = sizeof (UNIVERSAL_PAYLOAD_SMBIOS_TABLE);
    SmbiosHob->SmBiosEntryPoint = PcdGet32 (PcdSmbiosTablesBase);
  }

  // Build serial port hob
  PldSerialPortInfo = BuildGuidHob (&gUniversalPayloadSerialPortInfoGuid, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
  if (PldSerialPortInfo != NULL) {
    ZeroMem (PldSerialPortInfo, sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO));
    PldSerialPortInfo->Header.Revision = UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION;
    PldSerialPortInfo->Header.Length   = sizeof (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO);
    PldSerialPortInfo->UseMmio         = FALSE;
    PldSerialPortInfo->RegisterBase    = 0x3F8;
    PldSerialPortInfo->BaudRate        = 115200;
    PldSerialPortInfo->RegisterStride  = 1;
    PlatformUpdateHobInfo (&gUniversalPayloadSerialPortInfoGuid, PldSerialPortInfo);
  }

  // SMM register HOB
  UNIVERSAL_PAYLOAD_SMM_REGISTERS   *SmmRegisterHob;
  Length = sizeof (UNIVERSAL_PAYLOAD_SMM_REGISTERS) + 5 * sizeof (UNIVERSAL_PAYLOAD_GENERIC_REGISTER);
  SmmRegisterHob = BuildGuidHob (&gPldSmmRegisterInfoGuid, Length);
  if (SmmRegisterHob != NULL) {
    ZeroMem (SmmRegisterHob, Length);
    PlatformUpdateHobInfo (&gPldSmmRegisterInfoGuid, SmmRegisterHob);
  }

  // SMM memory HOB
  EFI_SMRAM_HOB_DESCRIPTOR_BLOCK   *SmmMemoryHob;
  Length = sizeof (EFI_SMRAM_HOB_DESCRIPTOR_BLOCK) + sizeof (EFI_SMRAM_DESCRIPTOR);
  SmmMemoryHob = BuildGuidHob (&gEfiSmmSmramMemoryGuid, Length);
  if (SmmMemoryHob != NULL) {
    ZeroMem (SmmMemoryHob, Length);
    PlatformUpdateHobInfo (&gEfiSmmSmramMemoryGuid, SmmMemoryHob);
  }

  // SPI Flash HOB
  SPI_FLASH_INFO   *SpiFlashInfoHob;
  SpiFlashInfoHob = BuildGuidHob (&gSpiFlashInfoGuid, sizeof (SPI_FLASH_INFO));
  if (SpiFlashInfoHob != NULL) {
    ZeroMem (SpiFlashInfoHob,  sizeof (SPI_FLASH_INFO));
    SpiFlashInfoHob->SpiAddress.Address = 0;
    PlatformUpdateHobInfo (&gSpiFlashInfoGuid, SpiFlashInfoHob);
  }

  // SPI NV variable HOB
  NV_VARIABLE_INFO   *NvVariableHob;
  NvVariableHob = BuildGuidHob (&gNvVariableInfoGuid, sizeof (NV_VARIABLE_INFO));
  if (NvVariableHob != NULL) {
    ZeroMem (NvVariableHob,  sizeof (NV_VARIABLE_INFO));
    PlatformUpdateHobInfo (&gNvVariableInfoGuid, NvVariableHob);
  }

  //
  // Build CPU memory space and IO space hob
  //
  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
    PhysicalAddressBits = (UINT8) RegEax;
  } else {
    PhysicalAddressBits  = 36;
  }
  BuildCpuHob (PhysicalAddressBits, 16);

  //
  // Report Local APIC range
  //
  ResourceAttribute = EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED |\
                      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE | EFI_RESOURCE_ATTRIBUTE_TESTED;
  BuildResourceDescriptorHob (EFI_RESOURCE_MEMORY_MAPPED_IO, ResourceAttribute, 0xFEC80000, SIZE_512KB);
  BuildMemoryAllocationHob ( 0xFEC80000, SIZE_512KB, EfiMemoryMappedIO);

  return LdrGlobal->LdrHobList;
}

/**
  Print the stack/HOB and heap usage information.

**/
VOID
EFIAPI
PrintStackHeapInfo (
  VOID
  )
{
  LOADER_GLOBAL_DATA             *LdrGlobal;
  EFI_HOB_HANDOFF_INFO_TABLE     *HandOffHob;
  UINT32                          StackBot;

  LdrGlobal = (LOADER_GLOBAL_DATA *)GetLoaderGlobalDataPointer();
  HandOffHob = (EFI_HOB_HANDOFF_INFO_TABLE *)LdrGlobal->LdrHobList;

  StackBot = DetectUsedStackBottom ((UINT32)HandOffHob->EfiMemoryTop,
                                    (UINT32)HandOffHob->EfiMemoryTop - (UINT32)HandOffHob->EfiFreeMemoryBottom);
  DEBUG ((
           DEBUG_INFO,
           "Stage2 stack: 0x%X (stack used 0x%X, HOB used 0x%X, 0x%X free)\n",
           (UINT32)HandOffHob->EfiMemoryTop - (UINT32)HandOffHob->EfiMemoryBottom,
           (UINT32)HandOffHob->EfiMemoryTop - StackBot,
           (UINT32)HandOffHob->EfiFreeMemoryBottom - (UINT32)HandOffHob->EfiMemoryBottom,
           StackBot - (UINT32)HandOffHob->EfiFreeMemoryBottom
           ));

  DEBUG ((
           DEBUG_INFO,
           "Stage2 heap: 0x%X (0x%X used, 0x%X free)\n",
           LdrGlobal->MemPoolEnd - LdrGlobal->MemPoolStart,
           LdrGlobal->MemPoolEnd - LdrGlobal->MemPoolCurrTop,
           LdrGlobal->MemPoolCurrTop - LdrGlobal->MemPoolStart
           ));
}


/**
  Initialize services so that payload can consume.

**/
VOID
EFIAPI
InitializeService (
  VOID
  )
{
  LOADER_GLOBAL_DATA             *LdrGlobal;
  SERVICES_LIST                  *ServiceList;

  // Initialize HOB
  LdrGlobal = (LOADER_GLOBAL_DATA *)GetLoaderGlobalDataPointer();

  // Zero out all services installed in previous stages.
  // Those service might have been expired and stage2 needs to reinstall if required.
  ServiceList = (SERVICES_LIST *)LdrGlobal->ServicePtr;
  if (ServiceList != NULL) {
    ZeroMem (ServiceList->Header, sizeof (SERVICE_COMMON_HEADER *) * ServiceList->Count);
  }

  // Register platform service
  RegisterService ((VOID *)&mPlatformService);

}
