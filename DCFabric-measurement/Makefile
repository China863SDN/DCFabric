CC = gcc
LIB = -L ../include -L ../libreg\
	-L../librule \
	-L../libua \
	-lrule \
	-lua \
	-lreg \
	-lpthread \
	-lm
main:measurement.o sensor_struct.o data_struct.o net.o
	$(CC) -o main measurement.o sensor_struct.o data_struct.o net.o $(LIB)

measurement.o:measurement.c 
	$(CC) -c measurement.c
sensor_struct.o:sensor_struct.c
	$(CC) -c $^
data_struct.o:data_struct.c 
	$(CC) -c $^
net.o:net.c
	$(CC) -c $^
	
clean:
	rm main *.o
