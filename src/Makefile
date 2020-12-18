# build helloworld executable when user executes "make"
sgp30: main.o test-sgp30.o sgp30.o tools.o sgp30_featureset.o sql-select-co2.o
	$(CC) $(LDFLAGS) -lsqlite3 -lm -ldl main.o sgp30.o tools.o sgp30_featureset.o -o sgp30
	$(CC) $(LDFLAGS) -lsqlite3 -lm test-sgp30.o sgp30.o tools.o sgp30_featureset.o -o test-sgp30
	$(CC) $(LDFLAGS) -lsqlite3 sql-select-co2.o -o sql-select-co2
main.o: main.c sgp30.h sgp_featureset.h sensirion_configuration.h tools.h
	$(CC) $(CFLAGS) -c main.c
test-sgp30.o: test-sgp30.c sgp30.h sgp_featureset.h sensirion_configuration.h tools.h
	$(CC) $(CFLAGS) -c test-sgp30.c
sgp30.o: sgp30.c sgp30.h sgp_featureset.h sensirion_configuration.h tools.h
	$(CC) $(CFLAGS) -c sgp30.c
tools.o: tools.c sgp30.h sensirion_configuration.h tools.h
	$(CC) $(CFLAGS) -c tools.c
sgp30_featureset.o: sgp30_featureset.c sgp_featureset.h tools.h
	$(CC) $(CFLAGS) -c sgp30_featureset.c
sql-select-co2.o: sql-select-co2.c
	$(CC) $(CFLAGS) -c sql-select-co2.c 

# remove object files and executable when user executes "make clean"
clean:
	rm test-sgp30.o sgp30.o tools.o test-sgp30
	rm main.o sgp30
	rm sql-select-co2.o sql-select-co2
