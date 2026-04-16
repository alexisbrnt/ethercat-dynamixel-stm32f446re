#include "esc_coe.h"
#include "utypes.h"
#include <stddef.h>


static const char acName1000[] = "Device Type";
static const char acName1008[] = "Device Name";
static const char acName1009[] = "Hardware Version";
static const char acName100A[] = "Software Version";
static const char acName1018[] = "Identity Object";
static const char acName1018_00[] = "Max SubIndex";
static const char acName1018_01[] = "Vendor ID";
static const char acName1018_02[] = "Product Code";
static const char acName1018_03[] = "Revision Number";
static const char acName1018_04[] = "Serial Number";
static const char acName1600[] = "ID_RX";
static const char acName1600_00[] = "Max SubIndex";
static const char acName1600_01[] = "ID_RX";
static const char acName1601[] = "control_mode";
static const char acName1601_00[] = "Max SubIndex";
static const char acName1601_01[] = "control_mode";
static const char acName1602[] = "torque_enabled";
static const char acName1602_00[] = "Max SubIndex";
static const char acName1602_01[] = "torque_enabled";
static const char acName1603[] = "LED_STATE";
static const char acName1603_00[] = "Max SubIndex";
static const char acName1603_01[] = "LED_STATE";
static const char acName1604[] = "goal_position";
static const char acName1604_00[] = "Max SubIndex";
static const char acName1604_01[] = "goal_position";
static const char acName1605[] = "target_velocity";
static const char acName1605_00[] = "Max SubIndex";
static const char acName1605_01[] = "target_velocity";
static const char acName1A00[] = "ID_TX";
static const char acName1A00_00[] = "Max SubIndex";
static const char acName1A00_01[] = "ID_TX";
static const char acName1A01[] = "state";
static const char acName1A01_00[] = "Max SubIndex";
static const char acName1A01_01[] = "state";
static const char acName1A02[] = "present_position";
static const char acName1A02_00[] = "Max SubIndex";
static const char acName1A02_01[] = "present_position";
static const char acName1A03[] = "present_velocity";
static const char acName1A03_00[] = "Max SubIndex";
static const char acName1A03_01[] = "present_velocity";
static const char acName1A04[] = "present_current";
static const char acName1A04_00[] = "Max SubIndex";
static const char acName1A04_01[] = "present_current";
static const char acName1A05[] = "present_temperature";
static const char acName1A05_00[] = "Max SubIndex";
static const char acName1A05_01[] = "present_temperature";
static const char acName1A06[] = "baudrate";
static const char acName1A06_00[] = "Max SubIndex";
static const char acName1A06_01[] = "baudrate";
static const char acName1A07[] = "operating_mode";
static const char acName1A07_00[] = "Max SubIndex";
static const char acName1A07_01[] = "operating_mode";
static const char acName1C00[] = "Sync Manager Communication Type";
static const char acName1C00_00[] = "Max SubIndex";
static const char acName1C00_01[] = "Communications Type SM0";
static const char acName1C00_02[] = "Communications Type SM1";
static const char acName1C00_03[] = "Communications Type SM2";
static const char acName1C00_04[] = "Communications Type SM3";
static const char acName1C12[] = "Sync Manager 2 PDO Assignment";
static const char acName1C12_00[] = "Max SubIndex";
static const char acName1C12_01[] = "PDO Mapping";
static const char acName1C12_02[] = "PDO Mapping";
static const char acName1C12_03[] = "PDO Mapping";
static const char acName1C12_04[] = "PDO Mapping";
static const char acName1C12_05[] = "PDO Mapping";
static const char acName1C12_06[] = "PDO Mapping";
static const char acName1C13[] = "Sync Manager 3 PDO Assignment";
static const char acName1C13_00[] = "Max SubIndex";
static const char acName1C13_01[] = "PDO Mapping";
static const char acName1C13_02[] = "PDO Mapping";
static const char acName1C13_03[] = "PDO Mapping";
static const char acName1C13_04[] = "PDO Mapping";
static const char acName1C13_05[] = "PDO Mapping";
static const char acName1C13_06[] = "PDO Mapping";
static const char acName1C13_07[] = "PDO Mapping";
static const char acName1C13_08[] = "PDO Mapping";
static const char acName6000[] = "ID_TX";
static const char acName6001[] = "state";
static const char acName6002[] = "present_position";
static const char acName6003[] = "present_velocity";
static const char acName6004[] = "present_current";
static const char acName6005[] = "present_temperature";
static const char acName6006[] = "baudrate";
static const char acName6007[] = "operating_mode";
static const char acName7000[] = "ID_RX";
static const char acName7001[] = "control_mode";
static const char acName7002[] = "torque_enabled";
static const char acName7003[] = "LED_STATE";
static const char acName7004[] = "goal_position";
static const char acName7005[] = "target_velocity";

const _objd SDO1000[] =
{
  {0x0, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1000, 5001, NULL},
};
const _objd SDO1008[] =
{
  {0x0, DTYPE_VISIBLE_STRING, 88, ATYPE_RO, acName1008, 0, "LAN9252 SPI"},
};
const _objd SDO1009[] =
{
  {0x0, DTYPE_VISIBLE_STRING, 40, ATYPE_RO, acName1009, 0, "0.0.1"},
};
const _objd SDO100A[] =
{
  {0x0, DTYPE_VISIBLE_STRING, 40, ATYPE_RO, acName100A, 0, "0.0.1"},
};
const _objd SDO1018[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1018_00, 4, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1018_01, 0, NULL},
  {0x02, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1018_02, 0, NULL},
  {0x03, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1018_03, 1, NULL},
  {0x04, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1018_04, 1, &Obj.serial},
};
const _objd SDO1600[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1600_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1600_01, 0x70000008, NULL},
};
const _objd SDO1601[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1601_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1601_01, 0x70010008, NULL},
};
const _objd SDO1602[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1602_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1602_01, 0x70020008, NULL},
};
const _objd SDO1603[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1603_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1603_01, 0x70030008, NULL},
};
const _objd SDO1604[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1604_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1604_01, 0x70040020, NULL},
};
const _objd SDO1605[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1605_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1605_01, 0x70050020, NULL},
};
const _objd SDO1A00[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A00_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A00_01, 0x60000008, NULL},
};
const _objd SDO1A01[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A01_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A01_01, 0x60010008, NULL},
};
const _objd SDO1A02[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A02_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A02_01, 0x60020020, NULL},
};
const _objd SDO1A03[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A03_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A03_01, 0x60030020, NULL},
};
const _objd SDO1A04[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A04_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A04_01, 0x60040010, NULL},
};
const _objd SDO1A05[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A05_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A05_01, 0x60050010, NULL},
};
const _objd SDO1A06[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A06_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A06_01, 0x60060008, NULL},
};
const _objd SDO1A07[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1A07_00, 1, NULL},
  {0x01, DTYPE_UNSIGNED32, 32, ATYPE_RO, acName1A07_01, 0x60070008, NULL},
};
const _objd SDO1C00[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C00_00, 4, NULL},
  {0x01, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C00_01, 1, NULL},
  {0x02, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C00_02, 2, NULL},
  {0x03, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C00_03, 3, NULL},
  {0x04, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C00_04, 4, NULL},
};
const _objd SDO1C12[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C12_00, 6, NULL},
  {0x01, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_01, 0x1600, NULL},
  {0x02, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_02, 0x1601, NULL},
  {0x03, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_03, 0x1602, NULL},
  {0x04, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_04, 0x1603, NULL},
  {0x05, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_05, 0x1604, NULL},
  {0x06, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C12_06, 0x1605, NULL},
};
const _objd SDO1C13[] =
{
  {0x00, DTYPE_UNSIGNED8, 8, ATYPE_RO, acName1C13_00, 8, NULL},
  {0x01, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_01, 0x1A00, NULL},
  {0x02, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_02, 0x1A01, NULL},
  {0x03, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_03, 0x1A02, NULL},
  {0x04, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_04, 0x1A03, NULL},
  {0x05, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_05, 0x1A04, NULL},
  {0x06, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_06, 0x1A05, NULL},
  {0x07, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_07, 0x1A06, NULL},
  {0x08, DTYPE_UNSIGNED16, 16, ATYPE_RO, acName1C13_08, 0x1A07, NULL},
};
const _objd SDO6000[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_TXPDO, acName6000, 1, &Obj.ID_TX},
};
const _objd SDO6001[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_TXPDO, acName6001, 0, &Obj.state},
};
const _objd SDO6002[] =
{
  {0x0, DTYPE_INTEGER32, 32, ATYPE_RO | ATYPE_TXPDO, acName6002, 0, &Obj.present_position},
};
const _objd SDO6003[] =
{
  {0x0, DTYPE_INTEGER32, 32, ATYPE_RO | ATYPE_TXPDO, acName6003, 0, &Obj.present_velocity},
};
const _objd SDO6004[] =
{
  {0x0, DTYPE_INTEGER16, 16, ATYPE_RO | ATYPE_TXPDO, acName6004, 0, &Obj.present_current},
};
const _objd SDO6005[] =
{
  {0x0, DTYPE_INTEGER16, 16, ATYPE_RO | ATYPE_TXPDO, acName6005, 0, &Obj.present_temperature},
};
const _objd SDO6006[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_TXPDO, acName6006, 0, &Obj.baudrate},
};
const _objd SDO6007[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_TXPDO, acName6007, 0, &Obj.operating_mode},
};
const _objd SDO7000[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_RXPDO, acName7000, 1, &Obj.ID_RX},
};
const _objd SDO7001[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_RXPDO, acName7001, 0, &Obj.control_mode},
};
const _objd SDO7002[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_RXPDO, acName7002, 0, &Obj.torque_enabled},
};
const _objd SDO7003[] =
{
  {0x0, DTYPE_UNSIGNED8, 8, ATYPE_RO | ATYPE_RXPDO, acName7003, 0, &Obj.LED_STATE},
};
const _objd SDO7004[] =
{
  {0x0, DTYPE_INTEGER32, 32, ATYPE_RO | ATYPE_RXPDO, acName7004, 0, &Obj.goal_position},
};
const _objd SDO7005[] =
{
  {0x0, DTYPE_INTEGER32, 32, ATYPE_RO | ATYPE_RXPDO, acName7005, 0, &Obj.target_velocity},
};

const _objectlist SDOobjects[] =
{
  {0x1000, OTYPE_VAR, 0, 0, acName1000, SDO1000},
  {0x1008, OTYPE_VAR, 0, 0, acName1008, SDO1008},
  {0x1009, OTYPE_VAR, 0, 0, acName1009, SDO1009},
  {0x100A, OTYPE_VAR, 0, 0, acName100A, SDO100A},
  {0x1018, OTYPE_RECORD, 4, 0, acName1018, SDO1018},
  {0x1600, OTYPE_RECORD, 1, 0, acName1600, SDO1600},
  {0x1601, OTYPE_RECORD, 1, 0, acName1601, SDO1601},
  {0x1602, OTYPE_RECORD, 1, 0, acName1602, SDO1602},
  {0x1603, OTYPE_RECORD, 1, 0, acName1603, SDO1603},
  {0x1604, OTYPE_RECORD, 1, 0, acName1604, SDO1604},
  {0x1605, OTYPE_RECORD, 1, 0, acName1605, SDO1605},
  {0x1A00, OTYPE_RECORD, 1, 0, acName1A00, SDO1A00},
  {0x1A01, OTYPE_RECORD, 1, 0, acName1A01, SDO1A01},
  {0x1A02, OTYPE_RECORD, 1, 0, acName1A02, SDO1A02},
  {0x1A03, OTYPE_RECORD, 1, 0, acName1A03, SDO1A03},
  {0x1A04, OTYPE_RECORD, 1, 0, acName1A04, SDO1A04},
  {0x1A05, OTYPE_RECORD, 1, 0, acName1A05, SDO1A05},
  {0x1A06, OTYPE_RECORD, 1, 0, acName1A06, SDO1A06},
  {0x1A07, OTYPE_RECORD, 1, 0, acName1A07, SDO1A07},
  {0x1C00, OTYPE_ARRAY, 4, 0, acName1C00, SDO1C00},
  {0x1C12, OTYPE_ARRAY, 6, 0, acName1C12, SDO1C12},
  {0x1C13, OTYPE_ARRAY, 8, 0, acName1C13, SDO1C13},
  {0x6000, OTYPE_VAR, 0, 0, acName6000, SDO6000},
  {0x6001, OTYPE_VAR, 0, 0, acName6001, SDO6001},
  {0x6002, OTYPE_VAR, 0, 0, acName6002, SDO6002},
  {0x6003, OTYPE_VAR, 0, 0, acName6003, SDO6003},
  {0x6004, OTYPE_VAR, 0, 0, acName6004, SDO6004},
  {0x6005, OTYPE_VAR, 0, 0, acName6005, SDO6005},
  {0x6006, OTYPE_VAR, 0, 0, acName6006, SDO6006},
  {0x6007, OTYPE_VAR, 0, 0, acName6007, SDO6007},
  {0x7000, OTYPE_VAR, 0, 0, acName7000, SDO7000},
  {0x7001, OTYPE_VAR, 0, 0, acName7001, SDO7001},
  {0x7002, OTYPE_VAR, 0, 0, acName7002, SDO7002},
  {0x7003, OTYPE_VAR, 0, 0, acName7003, SDO7003},
  {0x7004, OTYPE_VAR, 0, 0, acName7004, SDO7004},
  {0x7005, OTYPE_VAR, 0, 0, acName7005, SDO7005},
  {0xffff, 0xff, 0xff, 0xff, NULL, NULL}
};
