#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sensirion_configuration.h"
#include "tools.h"
#include "sgp_featureset.h"
#include "sgp30.h"

void main () {

  int fd;
  int16_t ret;

  u16 feature_set_version;
  u8  product_type;

  uint16_t tvoc_ppb;
  uint16_t co2_eq_ppm;
  uint16_t scaled_ethanol_signal;
  uint16_t scaled_h2_signal;

  if ((fd = sgp_probe("/dev/i2c-1")) < 0) {
    fprintf (stderr, "sgp_probe error\n");
    exit (-1);
  }

  if ((ret = sgp_get_feature_set_version(&feature_set_version, &product_type)) != 0) {
	  fprintf (stderr, "sgp_get_feature_set_version error\n");
	  exit (-1);
  }

  printf ("feature_set_version = %d, product_type = %d\n", feature_set_version, product_type);

while (1) {
  if ((ret = sgp_measure_iaq_blocking_read(fd, &tvoc_ppb, &co2_eq_ppm)) != 0) {
          fprintf (stderr, "sgp_measure_iaq_blocking_read error\n");
	  exit (-1);
  }

  if ((ret = sgp_measure_signals_blocking_read(fd, &scaled_ethanol_signal, &scaled_h2_signal)) != 0) {
          fprintf (stderr, "sgp_measure_signals_blocking_read error\n");
          exit (-1);
  }
//  printf ("sgp_measure_iaq_blocking_read...\n");
  printf ("  tvoc_ppb = %d, co2_eq_ppm = %d, scaled_ethanol_signal = %d, scaled_h2_signal = %d\n",
             tvoc_ppb, co2_eq_ppm, scaled_ethanol_signal, scaled_h2_signal);
  sleep(2);

}

  exit (0);
}
