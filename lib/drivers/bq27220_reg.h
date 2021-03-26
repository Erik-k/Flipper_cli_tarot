#pragma once

#define BQ27220_ADDRESS 0xAA
#define BQ27220_I2C_TIMEOUT 50

#define CommandControl 0x00
#define CommandAtRate 0x02
#define CommandAtRateTimeToEmpty 0x04
#define CommandTemperature 0x06
#define CommandVoltage 0x08
#define CommandBatteryStatus 0x0A
#define CommandCurrent 0x0C
#define CommandRemainingCapacity 0x10
#define CommandFullChargeCapacity 0x12
#define CommandAverageCurrent 0x14
#define CommandTimeToEmpty 0x16
#define CommandTimeToFull 0x18
#define CommandStandbyCurrent 0x1A
#define CommandStandbyTimeToEmpty 0x1C
#define CommandMaxLoadCurrent 0x1E
#define CommandMaxLoadTimeToEmpty 0x20
#define CommandRawCoulombCount 0x22
#define CommandAveragePower 0x24
#define CommandInternalTemperature 0x28
#define CommandCycleCount 0x2A
#define CommandStateOfCharge 0x2C
#define CommandStateOfHealth 0x2E
#define CommandChargeVoltage 0x30
#define CommandChargeCurrent 0x32
#define CommandBTPDischargeSet 0x34
#define CommandBTPChargeSet 0x36
#define CommandOperationStatus 0x3A
#define CommandDesignCapacity 0x3C
#define CommandSelectSubclass 0x3E
#define CommandMACData 0x40
#define CommandMACDataSum 0x60
#define CommandMACDataLen 0x61
#define CommandAnalogCount 0x79
#define CommandRawCurrent 0x7A
#define CommandRawVoltage 0x7C
#define CommandRawIntTemp 0x7E

#define Control_CONTROL_STATUS 0x0000
#define Control_DEVICE_NUMBER 0x0001
#define Control_FW_VERSION 0x0002
#define Control_BOARD_OFFSET 0x0009
#define Control_CC_OFFSET 0x000A
#define Control_CC_OFFSET_SAVE 0x000B
#define Control_OCV_CMD 0x000C
#define Control_BAT_INSERT 0x000D
#define Control_BAT_REMOVE 0x000E
#define Control_SET_SNOOZE 0x0013
#define Control_CLEAR_SNOOZE 0x0014
#define Control_SET_PROFILE_1 0x0015
#define Control_SET_PROFILE_2 0x0016
#define Control_SET_PROFILE_3 0x0017
#define Control_SET_PROFILE_4 0x0018
#define Control_SET_PROFILE_5 0x0019
#define Control_SET_PROFILE_6 0x001A
#define Control_CAL_TOGGLE 0x002D
#define Control_SEALED 0x0030
#define Control_RESET 0x0041
#define Control_EXIT_CAL 0x0080
#define Control_ENTER_CAL 0x0081
#define Control_ENTER_CFG_UPDATE 0x0090
#define Control_EXIT_CFG_UPDATE_REINIT 0x0091
#define Control_EXIT_CFG_UPDATE 0x0092
#define Control_RETURN_TO_ROM 0x0F00

#define AddressFullChargeCapacity 0x929D
#define AddressDesignCapacity 0x929F
#define AddressEMF 0x92A3
#define AddressC0 0x92A9
#define AddressR0 0x92AB
#define AddressT0 0x92AD
#define AddressR1 0x92AF
#define AddressTC 0x92B1
#define AddressC1 0x92B1
#define AddressStartDOD0 0x92BD
#define AddressStartDOD10 0x92BF
#define AddressStartDOD20 0x92C1
#define AddressStartDOD30 0x92C3
#define AddressStartDOD40 0x92C5
#define AddressStartDOD50 0x92C7
#define AddressStartDOD60 0x92C9
#define AddressStartDOD70 0x92CB
#define AddressStartDOD80 0x92CD
#define AddressStartDOD90 0x92CF
#define AddressStartDOD100 0x92D1
