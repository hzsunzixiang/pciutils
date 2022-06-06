#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#define IO_CONFIG 0xf8
#define IO_BYTE_1 0xfc

// https://www.youtube.com/watch?v=iZk1cDsIiiU&list=PLCGpd0Do5-I1hZpk8zi9Zh7SCnHrIQlgT&index=10
int main(int argc, char *argv[]) {
	int i, fd;
	uint32_t offset, config[5]; 
	uint8_t *bar0, value;

	/* Let's open the BAR's reosurce file */
	fd = open("/sys/bus/pci/devices/0000:04:08.0/resourceo", O_RDWR|O_SYNC);
	if(fd < 0) {
		perror("Error opening BAR's resource file");
		return -1;
	}
	bar0 = mmap(NULL, 256, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); 
    close(fd);
	/* Let's check if we have a valid pointer */ 
	if(bar0 == MAP_FAILED) {
		perror("Memory mapping of BAR failed"); 
		return -1;
	}

	/* Let's take care of the offset */
	fd = open("/sys/bus/pci/devices/0000:04:08.0/config", O_RDONLY);

	if(fd < 0) {
		perror("Error opening BAR's resource file"); 
        return -1;
	}
	i = read(fd, config, 0x14);

    close(fd);

	if(i != 0x14) {
		perror("Error reading PCI config header"); 
        munmap(bar0, 256); 
        return -1;
	}
	/* Calculate the offset */
	offset =(config[4] & 0xfffffff0) % 4096;

	/* Adjust the bar's pointer */ 
    bar0 = bar0 + offset;

	/* Let's access the card */
	*(bar0 + IO_CONFIG) = 0x0; /* Sets all Bytes to input */

	/* Let's read GPIO Byte 1 values */ 
    value = *(bar0 + IO_BYTE_1);

	for(i=0; i<4; i++)
		printf("IO %d:%s\n", i, ((value & (1<<i)) > 0)? "True": "False");

	/* Let's check whether we have to write the outputs */ 
    if(argc > 1) {
		value = (uint8_t) atoi(argv[1]);
		/* Set Byte 0 to output */
        *(bar0 + IO_CONFIG) = 0x1;

		/* Set the LEDs */
		*(bar0 + IO_BYTE_1) = (~value) << 5;
	}
	munmap(bar0, 256);
	return 0;        
}
