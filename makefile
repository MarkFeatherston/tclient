all: tclient

tclient: main_window.o settings_window.o error_window.o tftp.o tclient.o quit_dialog.o
	gcc -g main_window.o settings_window.o error_window.o tftp.o tclient.o quit_dialog.o `pkg-config --libs gtk+-2.0 libcurl` -o tclient

main_window.o: main_window.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c main_window.c `pkg-config --cflags gtk+-2.0` -o main_window.o

settings_window.o: settings_window.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c settings_window.c `pkg-config --cflags gtk+-2.0` -o settings_window.o
 
tftp.o: tftp.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c tftp.c `pkg-config --cflags libcurl gtk+-2.0` -o tftp.o

error_window.o: error_window.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c error_window.c `pkg-config --cflags gtk+-2.0` -o error_window.o

tclient.o: tclient.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c tclient.c `pkg-config --cflags gtk+-2.0` -o tclient.o
	
quit_dialog.o: quit_dialog.c error_window.h tftp.h main_window.h settings_window.h quit_dialog.h tclient.h
	gcc -g -c quit_dialog.c `pkg-config --cflags gtk+-2.0` -o quit_dialog.o
	
clean:
	-rm *.o tclient
