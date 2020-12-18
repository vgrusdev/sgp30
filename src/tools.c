#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <math.h>
#include "tools.h"

// #define DEBUG

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

void user_delay_ms(uint32_t period)
{
  usleep(period*1000);
}

void user_sleep_usec(uint32_t duration_us) {
	usleep(duration_us);
}

int8_t user_i2c_read(int fd, const uint8_t *data, uint16_t len)
{

  int ret;
  uint8_t reg = 0;

  if ((ret = write(fd, &reg, 1)) != 1) {
	  fprintf(stderr, "user_i2c_read: Read init error (writing address). ret = %d\n", ret);
	  return ret;
  }
  if ((ret = read(fd, (void*)data, len)) != len) {
	  fprintf(stderr, "user_i2c_read: Read error. ret = %d\n", ret);
	  return ret;
  }
#ifdef DEBUG
  int i;
  uint8_t *b;
  fprintf(stderr, "user_i2c_read (fd = %d, reg = 0x%02x, data..., len = %d\n", fd, reg, len);
  for (b = (uint8_t*)data, i = 0; i < len; ++i, ++b) {
          fprintf(stderr, "0x%02x ", *b);
  }
  fprintf(stderr,"\n");
#endif
  return 0;
}

int8_t user_i2c_write(int fd, const uint8_t *data, uint16_t len)
{
  int ret;

#ifdef DEBUG
  int i;
  uint8_t *b;
  fprintf(stderr, "user_i2c_write (fd = %d, data..., len = %d)\n", fd, len);
  for (b = (uint8_t *)data, i = 0; i < len; ++i, ++b) {
	  fprintf(stderr, "0x%02x ", *b);
  }
  fprintf(stderr,"\n");
  fflush(stdout);
#endif
  if ((ret = write(fd, data, len)) < 0 ) {
	  fprintf(stderr, "user_i2c_write: Write error. ret = %d\n", ret);
          return ret;
  }
  return 0;
}

int user_i2c_init(char *iic_dev, uint8_t reg_addr) {

  int fd;
  int ret;

  if ((fd = open(iic_dev, O_RDWR)) < 0) {
    fprintf(stderr, "sgp30_init: Failed to open the i2c bus \"%s\"", iic_dev);
    return -1;
  }
  if ((ret = ioctl(fd, I2C_SLAVE, reg_addr)) < 0) {
    fprintf(stderr, "sgp30_init: Failed to acquire bus access and/or talk to slave. dev=\"%s\" reg_addr=%02x ret=%d\n", iic_dev, reg_addr, ret);
    return -1;
  }

#ifdef DEBUG
  fprintf(stderr,"sgp30_init: SGP30 Init success.\n");
#endif
  return fd;

}


uint8_t user_generate_crc(uint8_t *data, uint16_t count) {

    uint16_t current_byte;
    uint8_t  crc = CRC8_INIT;
    uint8_t  crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}

int8_t user_check_crc(uint8_t *data, uint16_t count, uint8_t checksum) {

    if (user_generate_crc(data, count) != checksum)
        return STATUS_FAIL;
    return STATUS_OK;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

sqlite3 *db_init(char *dbname) {

   sqlite3 *db;
   char *sql;
   char *err_msg = 0;
   int rc;

   if ((rc = sqlite3_open(dbname, &db)) != SQLITE_OK ) {
      fprintf(stderr, "db_init: Can't open database: \'%s\': %s\n", dbname, sqlite3_errmsg(db));

      sqlite3_close(db);
      return(NULL);
   }
#ifdef DEBUG
   fprintf(stderr, "db_init: Opened database successfully: \'%s\'\n", dbname);
#endif

   sql = "DROP TABLE IF EXISTS CO2;"
         "CREATE TABLE CO2(id INT NOT NULL, tvoc_ppb INT, co2_eq_ppm INT, \
					    scaled_ethanol_signal INT, scaled_h2_signal INT, \
					    ts TIMESTAMP NOT NULL);"
         "INSERT INTO  CO2 VALUES(1, 0, 0, 0, 0, CURRENT_TIMESTAMP);";

   if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "db_init: Failed to create CO2 table: %s\n", err_msg);
     sqlite3_free(err_msg);
     sqlite3_close(db);
     return(NULL);
   }
#ifdef DEBUG
   fprintf(stderr, "db_init: CO2 table created successfully\n");
#endif

   return(db);
}


int db_update_sgp30_data (sqlite3 *db, struct comp_data *data) {

   char *sql;
   char *err_msg = 0;
   int rc;

   if ((sql = make_sql_update_sgp30(data)) == NULL) {
      sqlite3_close(db);
      return(-1);
   }
#ifdef DEBUG
   fprintf(stderr, "db_update_sgp30_data: SQL Update string: \'%s\'\n", sql);
#endif

   if ((rc = sqlite3_exec(db, sql, 0, 0, &err_msg)) != SQLITE_OK ) {
     fprintf(stderr, "db_update_sgp30_data. Failed to update CO2 table: %s\n", err_msg);
     fprintf(stderr, "db_update_sgp30_data. SQL = \'%s\'\n", sql);

     free(sql);
     sqlite3_free(err_msg);
     sqlite3_close(db);
     return(-2);
   }

#ifdef DEBUG
   fprintf(stderr, "db_update_sgp30_data: Updated database successfully\n");
#endif

   free(sql);
//   sqlite3_close(db);
   return(0);

}

/* {} */

char * make_sql_update_sgp30(struct comp_data *data) {

  int size = 0;
  char *p = NULL;


  const char *fmt = "UPDATE CO2 SET tvoc_ppb=%d, co2_eq_ppm=%d, scaled_ethanol_signal=%d, scaled_h2_signal=%d, ts=CURRENT_TIMESTAMP  where id=1;";

           /* Determine required size */

  if ((size = snprintf((char*)NULL, 0, fmt, data->tvoc_ppb,
				            data->co2_eq_ppm,
				            data->scaled_ethanol_signal, 
				            data->scaled_h2_signal)) < 0)
    return NULL;

               /* ++ For '\0' */
  if ((p = malloc(++size)) == NULL) {
    fprintf(stderr, "make_sql_update_sgp30: malloc() error\n");
    return NULL;
  }

  if ((size = snprintf(p, size, fmt, data->tvoc_ppb,
                                     data->co2_eq_ppm,
                                     data->scaled_ethanol_signal,
                                     data->scaled_h2_signal)) < 0) {
    free(p);
    return NULL;
  }
  return p;
}

/*  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   */

struct env_data {
  double temp;
  double rhum;
};  


static int callback_abshum(void *data, int argc, char **argv, char **azColName){
   int i;

   if (argc >= 2) {
     ((struct env_data*)data)->temp = strtod(argv[0], 0);
     ((struct env_data*)data)->rhum = strtod(argv[1], 0); 
   } else {
     ((struct env_data*)data)->temp = 0.0;
     ((struct env_data*)data)->rhum = 0.0;
     return -1;
   }
  return 0;
}


int get_temp_rhum(char *dbfname_h, struct env_data *env_data) {

  sqlite3 *db;
  char    *sql;
  char    *err_msg = 0;
  sqlite3_stmt *res;
  int ret;

  if ((ret = sqlite3_open(dbfname_h, &db)) != SQLITE_OK) {
#ifdef DEBUG
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
#endif
    return -1;
  }

  sql = "SELECT temp, rhum from Env;";

  ret = sqlite3_exec(db, sql, callback_abshum, (void *)env_data, &err_msg);

  if (ret != SQLITE_OK) {
#ifdef DEBUG
    fprintf(stderr, "SELECT error: %s\n", err_msg);
#endif
    sqlite3_free(err_msg);
    sqlite3_close(db);

    return -2;
  }

#ifdef DEBUG
   fprintf(stderr, "SELECT done successfully\n");
   fprintf(stderr, "  temp = %f, rhum = %f\n", env_data->temp, env_data->rhum);
#endif

  sqlite3_close(db);
}


int get_absolute_humidity(char *dbfname_h, u32 *absolute_humidity) {

  struct env_data env_data;
  int ret;

  double t;
  double h;
  double abs;

  if ((ret = get_temp_rhum(dbfname_h, &env_data)) != 0) {
    return -1;
  }
  t = env_data.temp;
  h = env_data.rhum;

  abs = 216 * (  ( (h/100.0)*6.112 * exp((17.62*t) / (243.12+t)) ) / (273.15 + t)  ) * 1000;
  *absolute_humidity = lround(abs);
#ifdef DEBUG
   fprintf(stderr, "Absolute humidity = %d\n", *absolute_humidity);
#endif
  if ((*absolute_humidity >=0) && (*absolute_humidity < 256000 )) {
    return 0;
  } else {
    *absolute_humidity = 0;
  }
  return 0;

}  /*  get_absolute_humidity()  */

