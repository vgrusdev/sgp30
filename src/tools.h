/*
    Copyright (c) 2017, Sensirion AG
    All rights reserved.

    Modified by vgrusdev for home use.
*/

#ifndef TOOLS_H
#define TOOLS_H

#include <stdio.h>
#include <sqlite3.h>
/* #include <sys/types> */
#include <stdint.h>
#include "sgp30.h"
#include "sensirion_configuration.h"


 struct comp_data {
      uint16_t tvoc_ppb;
      uint16_t co2_eq_ppm;
      uint16_t scaled_ethanol_signal;
      uint16_t scaled_h2_signal;
  };



int user_i2c_init(char *iic_dev, uint8_t reg_addr);
int8_t user_i2c_read(int fd, const uint8_t *data, uint16_t len);
int8_t user_i2c_write(int fd, const uint8_t *data, uint16_t len);

void user_sleep_usec(uint32_t duration_us);
void user_delay_ms(uint32_t period);

uint8_t user_generate_crc(uint8_t *data, uint16_t count);
int8_t user_check_crc(uint8_t *data, uint16_t count, uint8_t checksum);

sqlite3 *db_init(char *dbname);
int db_update_sgp30_data (sqlite3 *db, struct comp_data *data);
char * make_sql_update_sgp30(struct comp_data *data);

int get_absolute_humidity(char *dbfname_h, u32 *absolute_humidity);


/*
#define SGP30_I2C_ADDR_PRIM         0x58
*/

#define STATUS_OK 0
#define STATUS_FAIL (-1)

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef BIG_ENDIAN
#define be16_to_cpu(s) (s)
#define be32_to_cpu(s) (s)
#define be64_to_cpu(s) (s)
#else
#define be16_to_cpu(s) (((u16)(s) << 8) | (0xff & ((u16)(s)) >> 8))
#define be32_to_cpu(s) (((u32)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((u64)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((u64)be32_to_cpu((s) >> 32))))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#endif

#define CRC8_POLYNOMIAL             0x31
#define CRC8_INIT                   0xFF
#define CRC8_LEN                    1

// u8 sensirion_common_generate_crc(u8* data, u16 count);

// s8 sensirion_common_check_crc(u8* data, u16 count, u8 checksum);


#endif /* TOOLS_H */
