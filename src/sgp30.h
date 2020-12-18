/*
    Copyright (c) 2017, Sensirion AG
    All rights reserved.

    Modified by vgrusdev for home use.
*/

#ifndef SGP30_H
#define SGP30_H
#include "sensirion_configuration.h"
#include "tools.h"

extern int sgp_probe(char *iic_dev);
s16 sgp_iaq_init(int fd);

const char* sgp_get_driver_version(void);
u8 sgp_get_configured_address(void);
s16 sgp_get_feature_set_version(u16* feature_set_version, u8* product_type);

s16 sgp_get_iaq_baseline(int fd, u32* baseline);
s16 sgp_set_iaq_baseline(int fd, u32 baseline);

s16 sgp_measure_iaq_blocking_read(int fd, u16* tvoc_ppb, u16* co2_eq_ppm);
s16 sgp_measure_iaq(int fd);
s16 sgp_read_iaq(int fd, u16* tvoc_ppb, u16* co2_eq_ppm);

s16 sgp_measure_tvoc_blocking_read(int fd, u16* tvoc_ppb);
s16 sgp_measure_tvoc(int fd);
s16 sgp_read_tvoc(int fd, u16* tvoc_ppb);

s16 sgp_measure_co2_eq_blocking_read(int fd, u16* co2_eq_ppm);
s16 sgp_measure_co2_eq(int fd);
s16 sgp_read_co2_eq(int fd, u16* co2_eq_ppm);

s16 sgp_measure_signals_blocking_read(int fd, u16* scaled_ethanol_signal,
                                      u16* scaled_h2_signal);
s16 sgp_measure_signals(int fd);
s16 sgp_read_signals(int fd, u16* scaled_ethanol_signal, u16* scaled_h2_signal);

s16 sgp_measure_test(int fd, u16* test_result);

s16 sgp_set_absolute_humidity(int fd, u32 absolute_humidity);

#endif /* SGP30_H */

