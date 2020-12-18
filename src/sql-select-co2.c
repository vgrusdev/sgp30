#include <stdio.h>
#include <sqlite3.h> 
#include <unistd.h>
#include <string.h>

// #define DEBUG

static int callback(void *data, int argc, char **argv, char **azColName){
   int i;

   char hostname[24];

   if (gethostname(hostname, 24)) {
     strncpy (hostname, "dummy", sizeof(hostname));
   }
#ifdef DEBUG
   fprintf(stderr, "hostname = %s\n", hostname);
   fprintf(stderr, "data = %s\n", (const char*)data);
#endif

   if (argc >= 5) {
     printf("[{\"HostName\":\"%s\",\"%s\":%s,\"%s\":%s,\"%s\":%s,\"%s\":%s,\"%s\":\"%s\"}]",
	 hostname, azColName[0], argv[0] ? argv[0] : "NULL", 
		   azColName[1], argv[1] ? argv[1] : "NULL",
		   azColName[2], argv[2] ? argv[2] : "NULL",
		   azColName[3], argv[3] ? argv[3] : "NULL",
		   azColName[4], argv[4] ? argv[4] : "NULL");
     printf("\n");
   }
#ifdef DEBUG
   for(i = 0; i<argc; i++) {
      printf("\t%s = %s", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
#endif
   return 0;
}

int main(int argc, char* argv[]) {
   sqlite3 *db;
   char *sql;
   char *err_msg = 0;
   sqlite3_stmt *res;
   int rc;

   const char* data = "Callback function called";

   if (rc = sqlite3_open("/var/lib/pidata/co2.db", &db) != SQLITE_OK ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return(1);
   }
#ifdef DEBUG
   fprintf(stderr, "Opened database successfully\n");
#endif

   //  2020-07-13 23:13:30  local
   // sql = "SELECT temp, pres, rhum, datetime(ts, 'localtime') as ts  from Env;";
   // sql = "SELECT temp, pres, rhum, datetime(ts) as ts  from Env;";
   //  2020-07-13T23:13:30  local
   // sql = "SELECT temp, pres, rhum, strftime('%Y-%m-%dT%H:%M:%S', ts, 'localtime') as ts  from Env;";
   sql = "SELECT tvoc_ppb, co2_eq_ppm, scaled_ethanol_signal, scaled_h2_signal, strftime('%Y-%m-%dT%H:%M:%S', ts) as ts  from CO2;";

   printf("Content-Type: text/plain\n\n");

   rc = sqlite3_exec(db, sql, callback, (void*)data, &err_msg);

   if (rc != SQLITE_OK ) {
        
     fprintf(stderr, "SELECT error: %s\n", err_msg);
        
     sqlite3_free(err_msg);        
     sqlite3_close(db);
        
     return 1;
   } 
#ifdef DEBUG
   fprintf(stderr, "SELECT done successfully\n");
#endif

/* */

   sqlite3_close(db);
}

