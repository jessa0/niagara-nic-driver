/* AVR Register Address (dec)*/
#define TIMEOUT_SCALER_A                0
#define TIMEOUT_SCALER_B                1
#define TIMEOUT_SCALER_C                2
#define TIMEOUT_SCALER_D                3

#define CURRENT_MODE_REG_A              49
#define CURRENT_MODE_REG_B              55
#define CURRENT_MODE_REG_C              37
#define CURRENT_MODE_REG_D              43

#define FIBER_DEFAULT_REG_A             48
#define FIBER_DEFAULT_REG_B             54
#define FIBER_DEFAULT_REG_C             36
#define FIBER_DEFAULT_REG_D             42

#define COPPER_DEFAULT_REG_A            50
#define COPPER_DEFAULT_REG_B            56
#define COPPER_DEFAULT_REG_C            38
#define COPPER_DEFAULT_REG_D            44

#define RLY_STA_ADDR_A                  47
#define RLY_STA_ADDR_B                  53
#define RLY_STA_ADDR_C                  35
#define RLY_STA_ADDR_D                  41

#define OEM_ID_REG                      65
#define PRODUCT_ID_REG                  66
#define PRODUCT_REV_REG                 67
#define FW_ID_REG                       68

#define TEST_MODE_REG_NEW               31
#define RLY_TEST_REG_NEW                30
#define TEST_MODE_REG_OLD               46
#define RLY_TEST_REG_OLD                45

#define POWER_OFF_MODE_REG_A		51
#define POWER_OFF_MODE_REG_B		57
#define POWER_OFF_MODE_REG_C		39
#define POWER_OFF_MODE_REG_D		45

/* Current Mode Register Values*/
#define FORCE_BYPASS_COPPER             0x04
#define FORCE_ACTIVE_COPPER             0x02
#define FORCE_NOLINK_COPPER             0x05
#define FORCE_NORMAL_COPPER             0x70

#define FORCE_BYPASS_FIBER              0x8B
#define FORCE_ACTIVE_FIBER              0x83
#define FORCE_NOLINK_FIBER              0xAB
#define FORCE_NORMAL_FIBER              0xF0

/* Default Fiber Mode Values*/
#define FIBER_MODE_0                    0x93
#define FIBER_MODE_1                    0x9b
#define FIBER_MODE_2                    0x83
#define FIBER_MODE_3                    0xa3
#define FIBER_MODE_4                    0x8b
#define FIBER_MODE_5                    0xab

/* Default Copper Mode Values*/
#define COPPER_MODE_0                   0x00
#define COPPER_MODE_1                   0x01
#define COPPER_MODE_2                   0x02
#define COPPER_MODE_3                   0x03
#define COPPER_MODE_4                   0x04
#define COPPER_MODE_5                   0x05

/* Relay State Values*/
#define COPPER_BYPASS_STATE             0x01
#define COPPER_ACTIVE_STATE             0x02
#define COPPER_NOLINK_STATE             0x03
#define FIBER_BYPASS_STATE              0x10
#define FIBER_ACTIVE_STATE              0x20
#define FIBER_NOLINK_STATE              0x30

/* Relay Bank Position*/
#define POSITION_A                      0x01
#define POSITION_B                      0x02
#define POSITION_C                      0x04
#define POSITION_D                      0x08

/* Product IDs*/
#define N2264_ID                        0x38
#define N2265_ID                        0x39
#define N2285_ID                        0x44
#define N2264L_ID                       0x50
#define N2284_ID                        0x51
#define N2266_ID                        0x52
#define N2664_ID                        0x54
#define N2283_ID                        0x56
#define N2352_ID                        0x57
#define N2711_ID                        0x61
#define N2710_ID                        0x62
#define N32285_ID                       0x92
#define N2282_ID                        0x36
#define N32265_ID                       0x8A
#define N2264L_ID                       0x50
#define N42264_ID                       0xB5
#define N32284_ID                       0xB6
#define N32710_ID                       0x84
#define N32710TX_ID                     0xFE
