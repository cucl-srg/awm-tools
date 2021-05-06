
all: portnumber awmreduced

clean:
	${RM} *.o ./lib/*.so ./bin/*


awmreduced:	
	gcc -I./include/ -o ./lib/libawmreduced.so ./src/awmreduced.c -shared -export-dynamic
	gcc -I./include/ -L./lib/ -o ./lib/libpacketgiver.so ./src/packetgiver.c -lawmreduced -shared -export-dynamic
	gcc -ggdb -I./include/ -L./lib/ -o ./bin/awmreduced ./src/packetgivercmd.c -lpacketgiver -lpcap -lawmreduced -lm 

portnumber: 
	gcc -I./include/ -o ./lib/libportnumber.so ./src/portnumber.c -shared -export-dynamic
	gcc -I./include/ -L./lib/ -o ./lib/libpacketgiver.so ./src/packetgiver.c -lportnumber -shared -export-dynamic
	gcc -ggdb -I./include/ -L./lib/ -Wunused -o ./bin/portnumber ./src/packetgivercmd.c -lpacketgiver -lpcap -lportnumber -lm 




