#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>

#include "sensirion_configuration.h"
#include "tools.h"
#include "sgp_featureset.h"
#include "sgp30.h"

//#include <sys/types.h>

#include <sqlite3.h>
#include <sys/time.h>
#include <unistd.h>

// #define DEBUG

//

int main(int argc, char* argv[])
{
  int fd;
  int16_t rslt;
  int ret;

  sqlite3 *db = NULL;

  struct timeval currtime;
  struct timeval nexttime;
  struct timeval steptime;
  struct timeval difftime;
  
  struct timeval nextbasetime;
  struct timeval stepbasetime;

  stepbasetime.tv_sec = 60*60;
  stepbasetime.tv_usec= 0;

  struct timeval nextabshumtime;
  struct timeval stepabshumtime;

  stepabshumtime.tv_sec = 1*60;
  stepabshumtime.tv_usec= 0;


  int opt = 0;

  char *IIC_Dev = "/dev/i2c-1";
  char *dbfname = NULL;
  char *pidfname = NULL;
  char *baselinefile = NULL;
  char *dbfname_h = NULL;
  int   timeinterval = 1;
  int   daemonflag = 0;
  int   errflag = 0;

  FILE *fl;
  int   nf; 

  FILE *pf;
  FILE *fb;

  u32 baseline;
  u32 absolute_humidity;

  struct comp_data comp_data;

  u16 feature_set_version;
  u8  product_type;

#define DEF_PID_FILE "/var/run/sgp30.pid"

  while ((opt = getopt(argc, argv, "di:f:t:p:b:h:")) != -1) {
    switch(opt) {
      case 'i':
        IIC_Dev = optarg;
        break;
      case 'f':
        dbfname = optarg;
        break;
      case 't':
        timeinterval = atoi(optarg);
        break;
      case 'p':
	pidfname = optarg;
        break;
      case 'd':
        daemonflag = 1;
        break;
      case 'b':
        baselinefile = optarg;
        break;
      case 'h':
        dbfname_h = optarg;
        break;
      case '?':
      default :
        errflag = 1;
        break;
    }
  }

  if (daemonflag) {

    fl = popen("logger -t sgp30","w");
    if (fl == NULL) {
        fprintf(stderr, "sgp30: Can not redirect STDOUT to logger\n");
        exit(1);
    }
    int nf;
    nf = fileno(fl);
    dup2(nf,STDOUT_FILENO);
    dup2(nf,STDERR_FILENO);
    close(nf);
    fclose(stdin);
    fflush(stdout);
    if (daemon(0, 1)) {
        fprintf(stderr, "sgp30: Can not daemonise the process\n");
        exit(1);
    }
  }

  if (errflag) {
    fprintf(stderr, "Usage: %s [-i iic_dev] [-f dbfile] [-t interval] [-d] [-p pidfile] [-b baselinefile] [-h dbfile-humidity]\n", basename(argv[0]));
    exit(1);
  }
  if (daemonflag) {

    if (pidfname == NULL)
      pidfname = DEF_PID_FILE;

   if (dbfname == NULL) {
      dbfname = "/var/lib/pidata/co2.db";
    }

    fprintf(stderr, "sgp30 runs in backgroud\n");
    fprintf(stderr, "  argv[0] = %s\n", basename(argv[0]));
    fprintf(stderr, "  IIC_Dev = %s\n", IIC_Dev);
    fprintf(stderr, "  dbfname = %s\n", (dbfname != NULL) ? dbfname : "N/A");
    fprintf(stderr, "  timeout = %d\n", timeinterval);
    fprintf(stderr, "  daemonize = %s\n", (daemonflag == 1) ? "Yes" : "No");
    fprintf(stderr, "  pidfile = %s\n", (pidfname != NULL) ? pidfname : "N/A");
    fprintf(stderr, "  baselinefile = %s\n", (baselinefile != NULL) ? baselinefile : "N/A");
    fprintf(stderr, "  dbfname_h = %s\n", (dbfname_h != NULL) ? dbfname_h : "N/A");

    if ((pf = fopen(pidfname, "w+")) == NULL) {
      fprintf(stderr, "pid file open error: %s\n", pidfname);
    } else {
      if (fprintf(pf, "%u", getpid()) < 0) {
        fprintf(stderr, "pid file write error: %s\n", pidfname);
      }
      fclose(pf);
    }
  }

  int err1;
  int err2;
  int err3;
  int err4;

  err1 = 0;
  err2 = 0;
  err3 = 0;
  err4 = 0;

  steptime.tv_sec = timeinterval;
  steptime.tv_usec = 0;

  if (dbfname != NULL) {
    if ((db = db_init(dbfname)) == NULL) {
      exit(1);
    }
  }

  if ((fd = sgp_probe(IIC_Dev)) < 0) {
    fprintf (stderr, "sgp_probe error\n");
    exit (-1);
  }

  if (baselinefile != NULL) {
    if ((fb = fopen(baselinefile, "r")) != NULL) {
      if ((ret = fscanf(fb,"%ul", &baseline)) == 1) {
        if ((ret = sgp_set_iaq_baseline(fd, baseline)) != 0) {
          fprintf(stderr, "sgp baseline setup fail, ret = %d\n", ret);
        } else {
          fprintf(stderr, "sgp baseline setup success\n");
        }
      } else {
        fprintf(stderr, "sgp baseline file is empty\n");
      }
      fclose(fb);
    } else {
      fprintf(stderr, "sgp baseline file read open error\n");
    }
  }


  if (dbfname_h != NULL) {

    if ((ret = get_absolute_humidity(dbfname_h, &absolute_humidity)) == 0) {
      if ((ret = sgp_set_absolute_humidity(fd, absolute_humidity)) != 0) {
        fprintf(stderr, "sgp_set_absolute_humidity fail, ret = %d\n", ret);
        err4 = 1;
      } else {
        fprintf(stderr, "sgp_set_absolute_humidity success\n");
      }
    } else {
      fprintf(stderr, "can not get abs_humidity, ret = %d\n", ret);
      err4 = 1;
    }

    gettimeofday(&currtime, NULL);
    timeradd(&currtime, &stepabshumtime, &nextabshumtime);
  }


  gettimeofday(&currtime, NULL);
  timeradd(&currtime, &stepbasetime, &nextbasetime);


  while (1) {

    timeradd(&currtime, &steptime, &nexttime);

    /* Delay while the sensor completes a measurement */
//    dev.delay_ms(700);


    if ((ret = sgp_measure_iaq_blocking_read(fd, &comp_data.tvoc_ppb, 
					       &comp_data.co2_eq_ppm)) != 0) {
	comp_data.tvoc_ppb = 0; 
	comp_data.co2_eq_ppm = 0;
	if (err1 == 0) {
          fprintf (stderr, "sgp30: sgp_measure_iaq_blocking_read error, ret = %d\n", ret);
	  ++err1;
	}
    } else {
 	err1 = 0;
    }

    if ((ret = sgp_measure_signals_blocking_read(fd, &comp_data.scaled_ethanol_signal, 
						   &comp_data.scaled_h2_signal)) != 0) {
	comp_data.scaled_ethanol_signal = 0;
	comp_data.scaled_h2_signal = 0;
	if (err2 == 0) {
          fprintf (stderr, "sgp30: sgp_measure_signals_blocking_read error, ret = %d\n", ret);
	  ++err2;
	}
    } else {
	err2 = 0;
    }

    if (daemonflag == 0) {
	printf ("  tvoc_ppb = %d, co2_eq_ppm = %d, scaled_ethanol_signal = %d, scaled_h2_signal = %d\n",
             comp_data.tvoc_ppb, 
	     comp_data.co2_eq_ppm, 
	     comp_data.scaled_ethanol_signal, 
	     comp_data.scaled_h2_signal);
    }


    if (dbfname != NULL) {
      if ((ret = db_update_sgp30_data (db, &comp_data)) != 0) {
	fprintf(stderr, " sgp30: db_update_sgp30_data error. Stop updating DB. ret = %d\n", ret);
	dbfname = NULL;
      }
    }

    if (baselinefile != NULL) {

      gettimeofday(&currtime, NULL);
      if (timercmp(&currtime, &nextbasetime, > ) ) {

        timeradd(&currtime, &stepbasetime, &nextbasetime);
        if ((ret = sgp_get_iaq_baseline(fd, &baseline)) == 0) {
          if ((fb = fopen(baselinefile, "w")) != NULL) {
            if ((ret = fprintf(fb, "%u\n", baseline)) >= 0) {
              err3 = 0;
            } else {
              if (err3 == 0) {
	        fprintf(stderr, "baseline save file write error, ret = %d\n", ret);
	        err3 = 1;
	      }
            }
	    fclose(fb);
          } else {
	    if (err3 == 0) {
	      fprintf(stderr, "baseline save file open error, ret = %d\n", ret);
	      err3 = 1;
	    }
          }
        } else {
          if (err3 == 0) {
	    fprintf(stderr, "sgp_get_iaq_baseline error, ret = %d\n", ret);
	    err3 = 1;
	  }
        }

      }    /* timercmp()  */
    }    /*  if (baselinefile != NULL)  */


    if (dbfname_h != NULL) {

      gettimeofday(&currtime, NULL);
      if (timercmp(&currtime, &nextabshumtime, > ) ) {

        timeradd(&currtime, &stepabshumtime, &nextabshumtime);

        if ((ret = get_absolute_humidity(dbfname_h, &absolute_humidity)) == 0) {
          if ((ret = sgp_set_absolute_humidity(fd, absolute_humidity)) != 0) {
            if (err4 == 0) {
              fprintf(stderr, "sgp_set_absolute_humidity fail, ret = %d\n", ret);
              err4 = 1;
            }
          } else {
            err4 = 0;
          }
        } else {
	  if (err4 == 0) {
            fprintf(stderr, "can not get abs_humidity, ret = %d\n", ret);
            err4 = 1;
	  }
        }
      }

    }  /*  if (dbfname_h != NULL)  */




    gettimeofday(&currtime, NULL);
    if (timercmp(&nexttime, &currtime, >) ) {
      timersub(&nexttime, &currtime, &difftime);
      sleep(difftime.tv_sec);
      usleep(difftime.tv_usec);
    } else {
//      dev.delay_ms(70);
    }
    currtime.tv_sec = nexttime.tv_sec;
    currtime.tv_usec= nexttime.tv_usec;
  }  /* wile(1) */

}
