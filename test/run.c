#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

int set_interface_attribs (int fd, int speed, int parity){
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0){
		//error_message ("error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
                                        // no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0){
		//error_message ("error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block){
	struct termios tty;
    memset (&tty, 0, sizeof tty);
    if(tcgetattr (fd, &tty) != 0){
		//error_message ("error %d from tggetattr", errno);
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    //if(tcsetattr (fd, TCSANOW, &tty) != 0)
		//error_message ("error %d setting term attributes", errno);
}

typedef union _Block {
	unsigned char buf[2];
	unsigned short int num;
}Block;


int main(){
	Block temp;
	int i;
	double degree, height, distance;
	char *portname = "/dev/rfcomm0";
	
	int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);

	if (fd < 0){
		//error_message ("error %d opening %s: %s", errno, portname, strerror (errno));
		return;
	}

	set_interface_attribs (fd, B460800, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	
	//write (fd, "hello!\n", 7);           // send 7 character greeting
	
	//usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
										// receive 25:  approx 100 uS per char transmit
	char start_scan[7] = {"\0"};
	char get_scan_data[4] = {"\0"};
	char get_data[100] = {"\0"};
	char get_status[8] = {"\0"};
	
	
	start_scan[0] = 0x07;
	start_scan[1] = 0x10;
	start_scan[2] = 0xD0;
	start_scan[3] = 0x02;
	start_scan[4] = 0x78;
	start_scan[5] = 0x00;
	start_scan[6] = 0x00;
	
	get_scan_data[0] = 0x04;
	get_scan_data[1] = 0x30;
	get_scan_data[2] = 0x00;
	get_scan_data[3] = 0x00;
	
	//int n = read (fd, buf, sizeof buf);  // read up to 100 characters if ready to read
	
	
	write(fd, start_scan, 7);
	read(fd, get_data, 3);
	
	sleep(5);
	
	get_status[0] = 0x02;
	get_status[1] = 0x02;
	write(fd, get_status, 2);
	read(fd, get_status, 8);
	while (((get_status[2] & 0xFF) & 0x08) != 0x00) {
		get_status[0] = 0x02;
		get_status[1] = 0x02;
		write(fd, get_status, 2);
		usleep(5000);
		read(fd, get_status, 8);
		
	}
	int ret;
	while(1){
		write(fd, get_scan_data, 4);
		
		
		ret = read(fd, get_data, 11);
		
		//
		if(ret < 2){
			continue;
		}
		if((get_data[1] & 0xFF) == 0xFF){
			continue;
		}
		if((get_scan_data[2] & 0xFF) == 0xff){
			get_scan_data[2] = 0x00;
			get_scan_data[3]++;
		}
		else if(((get_scan_data[2] & 0xFF) == 0xff) && ((get_scan_data[3] & 0xFF) == 0xff)){
			get_scan_data[2] = 0x00;
			get_scan_data[3] = 0x00;
		}
		else {
			get_scan_data[2]++;
		}
		
		//
		if(((get_data[4] & 0xFF) & 0x10) != 0x00){
			get_scan_data[2] = 0x00;
			get_scan_data[3] = 0x00;
			continue;
		}
		if(((get_data[4] & 0xFF) & 0x01) == 0x00){
			break;
		}
		
		
		
		//for(i = 0; i < 11; i++){
		//	printf("%02x ", get_data[i] & 0xFF);
		//}
		//printf("\n");
		temp.buf[0] = get_data[5] & 0xFF;
		temp.buf[1] = get_data[6] & 0xFF;
		while(temp.num >= 1800){
			temp.num -= 1800;
		}
		degree = 2 * M_PI * temp.num / 1800;
		//printf("%f ", degree);
		
		temp.buf[0] = get_data[7] & 0xFF;
		temp.buf[1] = get_data[8] & 0xFF;
		
		height = temp.num;
		//printf("%f ", height / 1000);
		
		temp.buf[0] = get_data[9] & 0xFF;
		temp.buf[1] = get_data[10] & 0xFF;
		
		distance = temp.num;
		if(distance > 165.0){
			distance = 0.0;
		}
		else {
			distance = 165.0 - distance;
			printf("%f, %f, %f\n", distance, degree, height / 142.0);
		}
		
		
		
		
		
	}
	//read(fd, get_data, 2);
	//printf("%s\n", get_data);

	
	
	return 0;
	
	
}
