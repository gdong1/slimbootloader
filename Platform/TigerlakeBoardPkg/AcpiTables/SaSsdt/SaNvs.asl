//
// Automatically generated by GenNvs ver 2.4.6
// Please DO NOT modify !!!
//

/**@file

  Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

  //
  // Define SA NVS Area operation region.
  //


  OperationRegion(SANV,SystemMemory, 0xFFFF0000,0xAA55)
  Field(SANV,AnyAcc,Lock,Preserve)
  {  Offset(0),      ASLB, 32, // Offset(0),     IGD OpRegion base address
  Offset(4),      IMON, 8,  // Offset(4),     IMON Current Value
  Offset(5),      IGDS, 8,  // Offset(5),     IGD State (Primary Display = 1)
  Offset(6),      IBTT, 8,  // Offset(6),     IGD Boot Display Device
  Offset(7),      IPAT, 8,  // Offset(7),     IGD Panel Type CMOS option
  Offset(8),      IPSC, 8,  // Offset(8),     IGD Panel Scaling
  Offset(9),      IBIA, 8,  // Offset(9),     IGD BIA Configuration
  Offset(10),     ISSC, 8,  // Offset(10),    IGD SSC Configuration
  Offset(11),     IDMS, 8,  // Offset(11),    IGD DVMT Memory Size
  Offset(12),     IF1E, 8,  // Offset(12),    IGD Function 1 Enable
  Offset(13),     HVCO, 8,  // Offset(13),    HPLL VCO
  Offset(14),     GSMI, 8,  // Offset(14),    GMCH SMI/SCI mode (0=SCI)
  Offset(15),     PAVP, 8,  // Offset(15),    IGD PAVP data
  Offset(16),     CADL, 8,  // Offset(16),    Current Attached Device List
  Offset(17),     CSTE, 16, // Offset(17),    Current Display State
  Offset(19),     NSTE, 16, // Offset(19),    Next Display State
  Offset(21),     NDID, 8,  // Offset(21),    Number of Valid Device IDs
  Offset(22),     DID1, 32, // Offset(22),    Device ID 1
  Offset(26),     DID2, 32, // Offset(26),    Device ID 2
  Offset(30),     DID3, 32, // Offset(30),    Device ID 3
  Offset(34),     DID4, 32, // Offset(34),    Device ID 4
  Offset(38),     DID5, 32, // Offset(38),    Device ID 5
  Offset(42),     DID6, 32, // Offset(42),    Device ID 6
  Offset(46),     DID7, 32, // Offset(46),    Device ID 7
  Offset(50),     DID8, 32, // Offset(50),    Device ID 8
  Offset(54),     DID9, 32, // Offset(54),    Device ID 9
  Offset(58),     DIDA, 32, // Offset(58),    Device ID 10
  Offset(62),     DIDB, 32, // Offset(62),    Device ID 11
  Offset(66),     DIDC, 32, // Offset(66),    Device ID 12
  Offset(70),     DIDD, 32, // Offset(70),    Device ID 13
  Offset(74),     DIDE, 32, // Offset(74),    Device ID 14
  Offset(78),     DIDF, 32, // Offset(78),    Device ID 15
  Offset(82),     DIDX, 32, // Offset(82),    Device ID for eDP device
  Offset(86),     NXD1, 32, // Offset(86),    Next state DID1 for _DGS
  Offset(90),     NXD2, 32, // Offset(90),    Next state DID2 for _DGS
  Offset(94),     NXD3, 32, // Offset(94),    Next state DID3 for _DGS
  Offset(98),     NXD4, 32, // Offset(98),    Next state DID4 for _DGS
  Offset(102),    NXD5, 32, // Offset(102),   Next state DID5 for _DGS
  Offset(106),    NXD6, 32, // Offset(106),   Next state DID6 for _DGS
  Offset(110),    NXD7, 32, // Offset(110),   Next state DID7 for _DGS
  Offset(114),    NXD8, 32, // Offset(114),   Next state DID8 for _DGS
  Offset(118),    NXDX, 32, // Offset(118),   Next state DID for eDP
  Offset(122),    LIDS, 8,  // Offset(122),   Lid State (Lid Open = 1)
  Offset(123),    KSV0, 32, // Offset(123),   First four bytes of AKSV (manufacturing mode)
  Offset(127),    KSV1, 8,  // Offset(127),   Fifth byte of AKSV (manufacturing mode)
  Offset(128),    BRTL, 8,  // Offset(128),   Brightness Level Percentage
  Offset(129),    ALSE, 8,  // Offset(129),   Ambient Light Sensor Enable
  Offset(130),    ALAF, 8,  // Offset(130),   Ambient Light Adjusment Factor
  Offset(131),    LLOW, 8,  // Offset(131),   LUX Low Value
  Offset(132),    LHIH, 8,  // Offset(132),   LUX High Value
  Offset(133),    ALFP, 8,  // Offset(133),   Active LFP
  Offset(134),    IPTP, 8,  // Offset(134),   IPU ACPI device type (0=Disabled, 1=AVStream virtual device as child of GFX)
  Offset(135),    EDPV, 8,  // Offset(135),   Check for eDP display device
  Offset(136),    HGMD, 8,  // Offset(136),   SG Mode (0=Disabled, 1=HG Muxed, 2=HG Muxless, 3=DGPU Only)
  Offset(137),    HGFL, 8,  // Offset(137),   HG Feature List
  Offset(138),    SGGP, 8,  // Offset(138),   PCIe0 GPIO Support (0=Disabled, 1=PCH Based, 2=I2C Based)
  Offset(139),    HRE0, 8,  // Offset(139),   PCIe0 HLD RST IO Expander Number
  Offset(140),    HRG0, 32, // Offset(140),   PCIe0 HLD RST GPIO Number
  Offset(144),    HRA0, 8,  // Offset(144),   PCIe0 HLD RST GPIO Active Information
  Offset(145),    PWE0, 8,  // Offset(145),   PCIe0 PWR Enable IO Expander Number
  Offset(146),    PWG0, 32, // Offset(146),   PCIe0 PWR Enable GPIO Number
  Offset(150),    PWA0, 8,  // Offset(150),   PCIe0 PWR Enable GPIO Active Information
  Offset(151),    P1GP, 8,  // Offset(151),   PCIe1 GPIO Support (0=Disabled, 1=PCH Based, 2=I2C Based)
  Offset(152),    HRE1, 8,  // Offset(152),   PCIe1 HLD RST IO Expander Number
  Offset(153),    HRG1, 32, // Offset(153),   PCIe1 HLD RST GPIO Number
  Offset(157),    HRA1, 8,  // Offset(157),   PCIe1 HLD RST GPIO Active Information
  Offset(158),    PWE1, 8,  // Offset(158),   PCIe1 PWR Enable IO Expander Number
  Offset(159),    PWG1, 32, // Offset(159),   PCIe1 PWR Enable GPIO Number
  Offset(163),    PWA1, 8,  // Offset(163),   PCIe1 PWR Enable GPIO Active Information
  Offset(164),    P2GP, 8,  // Offset(164),   PCIe2 GPIO Support (0=Disabled, 1=PCH Based, 2=I2C Based)
  Offset(165),    HRE2, 8,  // Offset(165),   PCIe2 HLD RST IO Expander Number
  Offset(166),    HRG2, 32, // Offset(166),   PCIe2 HLD RST GPIO Number
  Offset(170),    HRA2, 8,  // Offset(170),   PCIe2 HLD RST GPIO Active Information
  Offset(171),    PWE2, 8,  // Offset(171),   PCIe2 PWR Enable IO Expander Number
  Offset(172),    PWG2, 32, // Offset(172),   PCIe2 PWR Enable GPIO Number
  Offset(176),    PWA2, 8,  // Offset(176),   PCIe2 PWR Enable GPIO Active Information
  Offset(177),    P3GP, 8,  // Offset(177),   PCIe3 GPIO Support (0=Disabled, 1=PCH Based, 2=I2C Based)
  Offset(178),    HRE3, 8,  // Offset(178),   PCIe3 HLD RST IO Expander Number
  Offset(179),    HRG3, 32, // Offset(179),   PCIe3 HLD RST GPIO Number
  Offset(183),    HRA3, 8,  // Offset(183),   PCIe3 HLD RST GPIO Active Information
  Offset(184),    PWE3, 8,  // Offset(184),   PCIe3 PWR Enable IO Expander Number
  Offset(185),    PWG3, 32, // Offset(185),   PCIe3 PWR Enable GPIO Number
  Offset(189),    PWA3, 8,  // Offset(189),   PCIe3 PWR Enable GPIO Active Information
  Offset(190),    P3WK, 32, // Offset(190),   PCIe3 RTD3 Device Wake GPIO Number
  Offset(194),    DLPW, 16, // Offset(194),   Delay after power enable for PCIe
  Offset(196),    DLHR, 16, // Offset(196),   Delay after Hold Reset for PCIe
  Offset(198),    EECP, 8,  // Offset(198),   PCIe0 Endpoint Capability Structure Offset
  Offset(199),    XBAS, 32, // Offset(199),   Any Device's PCIe Config Space Base Address
  Offset(203),    GBAS, 16, // Offset(203),   GPIO Base Address
  Offset(205),    NVGA, 32, // Offset(205),   NVIG opregion address
  Offset(209),    NVHA, 32, // Offset(209),   NVHM opregion address
  Offset(213),    AMDA, 32, // Offset(213),   AMDA opregion address
  Offset(217),    LTRX, 8,  // Offset(217),   Latency Tolerance Reporting Enable
  Offset(218),    OBFX, 8,  // Offset(218),   Optimized Buffer Flush and Fill
  Offset(219),    LTRY, 8,  // Offset(219),   Latency Tolerance Reporting Enable
  Offset(220),    OBFY, 8,  // Offset(220),   Optimized Buffer Flush and Fill
  Offset(221),    LTRZ, 8,  // Offset(221),   Latency Tolerance Reporting Enable
  Offset(222),    OBFZ, 8,  // Offset(222),   Optimized Buffer Flush and Fill
  Offset(223),    LTRW, 8,  // Offset(223),   Latency Tolerance Reporting Enable
  Offset(224),    OBFA, 8,  // Offset(224),   Optimized Buffer Flush and Fill
  Offset(225),    SMSL, 16, // Offset(225),   SA Peg Latency Tolerance Reporting Max Snoop Latency
  Offset(227),    SNSL, 16, // Offset(227),   SA Peg Latency Tolerance Reporting Max No Snoop Latency
  Offset(229),    M64B, 64, // Offset(229),   Base of above 4GB MMIO resource
  Offset(237),    M64L, 64, // Offset(237),   Length of above 4GB MMIO resource
  Offset(245),    CPEX, 32, // Offset(245),   CPU ID info to get Family Id or Stepping
  Offset(249),    M32B, 32, // Offset(249),   Base of below 4GB MMIO resource
  Offset(253),    M32L, 32, // Offset(253),   Length of below 4GB MMIO resource
  Offset(257),    P0WK, 32, // Offset(257),   PCIe0 RTD3 Device Wake GPIO Number
  Offset(261),    P1WK, 32, // Offset(261),   PCIe1 RTD3 Device Wake GPIO Number
  Offset(265),    P2WK, 32, // Offset(265),   PCIe2 RTD3 Device Wake GPIO Number
  Offset(269),    VTDS, 8,  // Offset(269),   VT-d Enable/Disable
  Offset(270),    VTB1, 32, // Offset(270),   VT-d Base Address 1
  Offset(274),    VTB2, 32, // Offset(274),   VT-d Base Address 2
  Offset(278),    VTB3, 32, // Offset(278),   VT-d Base Address 3
  Offset(282),    VTB4, 32, // Offset(282),   VT-d Base Address 4 (iTBT PCIE0)
  Offset(286),    VTB5, 32, // Offset(286),   VT-d Base Address 5 (iTBT PCIE1)
  Offset(290),    VTB6, 32, // Offset(290),   VT-d Base Address 6 (iTBT PCIE2)
  Offset(294),    VTB7, 32, // Offset(294),   VT-d Base Address 7 (iTBT PCIE3)
  Offset(298),    VE1V, 16, // Offset(298),   VT-d Engine#1 Vendor ID
  Offset(300),    VE2V, 16, // Offset(300),   VT-d Engine#2 Vendor ID
  Offset(302),    RPIN, 8,  // Offset(302),   RootPort Number
  Offset(303),    RPBA, 32, // Offset(303),   RootPortAddress
  Offset(307),    CTHM, 8,  // Offset(307),   CPU Trace Hub Mode
  Offset(308),    SIME, 8,  // Offset(308),   Simics Environment information
  Offset(309),    THCE, 8,  // Offset(309),   TCSS XHCI Device Enable
  Offset(310),    TDCE, 8,  // Offset(310),   TCSS XDCI Device Enable
  Offset(311),    DME0, 8,  // Offset(311),   TCSS DMA 0 Device Enable
  Offset(312),    DME1, 8,  // Offset(312),   TCSS DMA 1 Device Enable
  Offset(313),    TRE0, 8,  // Offset(313),   TCSS ItbtPcieRp PCIE RP 0 Device Enable
  Offset(314),    TRE1, 8,  // Offset(314),   TCSS ItbtPcieRp PCIE RP 1 Device Enable
  Offset(315),    TRE2, 8,  // Offset(315),   TCSS ItbtPcieRp PCIE RP 2 Device Enable
  Offset(316),    TRE3, 8,  // Offset(316),   TCSS ItbtPcieRp PCIE RP 3 Device Enable
  Offset(317),    TPA0, 32, // Offset(317),   TCSS ItbtPcie Root Port address 0
  Offset(321),    TPA1, 32, // Offset(321),   TCSS ItbtPcie Root Port address 1
  Offset(325),    TPA2, 32, // Offset(325),   TCSS ItbtPcie Root Port address 2
  Offset(329),    TPA3, 32, // Offset(329),   TCSS ItbtPcie Root Port address 3
  Offset(333),    TCDS, 32, // Offset(333),   TCSS xDCI Power Down Scale Value, DWC_USB3_GCTL_INIT[31:19]
  Offset(337),    TCIT, 8,  // Offset(337),   TCSS xDCI Int Pin
  Offset(338),    TCIR, 8,  // Offset(338),   TCSS xDCI Irq Number
  Offset(339),    TRTD, 8,  // Offset(339),   TCSS RTD3
  Offset(340),    ITM0, 32, // Offset(340),   TCSS DMA0 RMRR address
  Offset(344),    ITM1, 32, // Offset(344),   TCSS DMA1 RMRR address
  Offset(348),    LTE0, 8,  // Offset(348),   Latency Tolerance Reporting Mechanism. <b>0: Disable</b>; 1: Enable.
  Offset(349),    LTE1, 8,  // Offset(349),   Latency Tolerance Reporting Mechanism. <b>0: Disable</b>; 1: Enable.
  Offset(350),    LTE2, 8,  // Offset(350),   Latency Tolerance Reporting Mechanism. <b>0: Disable</b>; 1: Enable.
  Offset(351),    LTE3, 8,  // Offset(351),   Latency Tolerance Reporting Mechanism. <b>0: Disable</b>; 1: Enable.
  Offset(352),    PSL0, 16, // Offset(352),   PCIE LTR max snoop Latency 0
  Offset(354),    PSL1, 16, // Offset(354),   PCIE LTR max snoop Latency 1
  Offset(356),    PSL2, 16, // Offset(356),   PCIE LTR max snoop Latency 2
  Offset(358),    PSL3, 16, // Offset(358),   PCIE LTR max snoop Latency 3
  Offset(360),    PNS0, 16, // Offset(360),   PCIE LTR max no snoop Latency 0
  Offset(362),    PNS1, 16, // Offset(362),   PCIE LTR max no snoop Latency 1
  Offset(364),    PNS2, 16, // Offset(364),   PCIE LTR max no snoop Latency 2
  Offset(366),    PNS3, 16, // Offset(366),   PCIE LTR max no snoop Latency 3
  Offset(368),    IMRY, 8,  // Offset(368),   IOM Ready
  Offset(369),    TIVS, 8,  // Offset(369),   TCSS IOM VccSt
  Offset(370),    PG0E, 8,  // Offset(370),   <0:Disabled, 1:Enabled>
  Offset(371),    PG1E, 8,  // Offset(371),   <0:Disabled, 1:Enabled>
  Offset(372),    PG2E, 8,  // Offset(372),   <0:Disabled, 1:Enabled>
  Offset(373),    PG3E, 8,  // Offset(373),   <0:Disabled, 1:Enabled>
  Offset(374),    VMDE, 8,  // Offset(374),   VMD Device Enable
  Offset(375),    DIDY, 32, // Offset(375),   Device ID for second LFP device
  Offset(379),    NXDY, 32, // Offset(379),   Next state DID for Second Display
  Offset(383),    SLTS, 8,  // Offset(383),   PCIe slot selection
  Offset(384),    VMR1, 8,  // Offset(384),   VMD PCH RP 1 to 8 <0:Disabled, 1:Enabled>
  Offset(385),    VMR2, 8,  // Offset(385),   VMD PCH RP 9 to 16 <0:Disabled, 1:Enabled>
  Offset(386),    VMR3, 8,  // Offset(386),   VMD PCH RP 17 to 24 <0:Disabled, 1:Enabled>
  Offset(387),    VMS0, 8,  // Offset(387),   VMD SATA PORT 0 <0:Disabled, 1:Enabled>
  Offset(388),    VMS1, 8,  // Offset(388),   VMD SATA PORT 1 <0:Disabled, 1:Enabled>
  Offset(389),    VMS2, 8,  // Offset(389),   VMD SATA PORT 2 <0:Disabled, 1:Enabled>
  Offset(390),    VMS3, 8,  // Offset(390),   VMD SATA PORT 3 <0:Disabled, 1:Enabled>
  Offset(391),    VMS4, 8,  // Offset(391),   VMD SATA PORT 4 <0:Disabled, 1:Enabled>
  Offset(392),    VMS5, 8,  // Offset(392),   VMD SATA PORT 5 <0:Disabled, 1:Enabled>
  Offset(393),    VMS6, 8,  // Offset(393),   VMD SATA PORT 6 <0:Disabled, 1:Enabled>
  Offset(394),    VMS7, 8,  // Offset(394),   VMD SATA PORT 7 <0:Disabled, 1:Enabled>
  Offset(395),    VMCP, 8,  // Offset(395),   VMD CPU RP      <0:Disabled, 1:Enabled>
  Offset(396),    CPRT, 8,  // Offset(396),   RTD3 Support for CPU PCIE.
  Offset(397),    CSLU, 32, // Offset(397),   Lane Used of each CSI Port <0:Not Configured, 1:x1, 2:x2, 3:x3 4:x4>
  Offset(401),    CSSP, 32, // Offset(401),   Speed of each CSI Port <0:Not configured, 1:<416GMbps, 2:<1.5Gbps, 3:<2.0Gbps, 4:<2.5Gbps, 5:<4Gbps, 6:>4Gbps>
  Offset(405),    MPGN, 8,  // Offset(405),   Max PEG port number
  Offset(406),    CMBM, 8,  // Offset(406),   Current Memory Boot Mode <0: BOOT_MODE_1LM(Default), 1: BOOT_MODE_2LM, 2: BOOT_MODE_PROVISION>
  Offset(407),    DPMS, 8,  // Offset(407),   Dynamic PMem Support <0: Disabled, 1:Enabled>
  Offset(408),    PMSA, 64, // Offset(408),   Private Pmem Starting address
  Offset(416),    PMRL, 64, // Offset(416),   Private Pmem Range Length
  Offset(424),    EEC3, 8,  // Offset(424),   PCIe3 Endpoint Capability Structure Offset
  Offset(425),    P0SC, 8,  // Offset(425),   PCIe0 RTD3 Device Source Clock Number
  Offset(426),    P1SC, 8,  // Offset(426),   PCIe1 RTD3 Device Source Clock Number
  Offset(427),    P2SC, 8,  // Offset(427),   PCIe2 RTD3 Device Source Clock Number
  Offset(428),    P3SC, 8,  // Offset(428),   PCIe2 RTD3 Device Source Clock Number
  Offset(429),    SBN0, 8,  // Offset(429),   PCIe0 Secondary Bus Number (PCIe0 Endpoint Bus Number)
  Offset(430),    SBN1, 8,  // Offset(430),   PCIe1 Secondary Bus Number (PCIe0 Endpoint Bus Number)
  Offset(431),    SBN2, 8,  // Offset(431),   PCIe2 Secondary Bus Number (PCIe0 Endpoint Bus Number)
  Offset(432),    SBN3, 8,  // Offset(432),   PCIe2 Secondary Bus Number (PCIe0 Endpoint Bus Number)
  Offset(433),    EEC1, 8,  // Offset(433),   PCIe1 Endpoint Capability Structure Offset
  Offset(434),    EEC2, 8,  // Offset(434),   PCIe2 Endpoint Capability Structure Offset
  Offset(435),    PBR1, 8,  // Offset(435),   Is bridge device behind PEG1
  Offset(436),    PBR2, 8,  // Offset(436),   Is bridge device behind PEG2
  Offset(437),    PBR3, 8,  // Offset(437),   Is bridge device behind PEG3
  Offset(438),    HGST, 8,  // Offset(438),   Slot selection between PCH/PEG
  Offset(439),    PDIW, 8,  // Offset(439),   DPin Dynamic Switch
  Offset(440),    PDI0, 16, // Offset(440),   DPin Dynamic Switch delay 0, unit is ms
  Offset(442),    PDI1, 16, // Offset(442),   DPin Dynamic Switch delay 1, unit is ms
  }
