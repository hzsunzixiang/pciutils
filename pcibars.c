/*
 *	pcibars - Programm to get informations about the BARs of a
 *	PCI(e) device
 *
 *	Written by Johannes 4Linux and put to public domain. You can do
 *	with it anything you want, but I don't give you any warranty.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/pci.h"

/** Array with size units */
char *sizes[] = {"Bytes", "kBytes", "MBytes", "GBytes", "None"};

/** struct contains all the information about a PCI BAR */
struct bar_info{
	int type;	/* 0: BAR not used, 1: 32 bit IO BAR, 2: 32 bit memory BAR, 3: 64 bit memory BAR */
	u64 size;	/* Size of the BAR */
	char *unit;	/* Unit of BAR size */
	int prefetchable;
	u64 addr;	/* Memory or IO Address the BAR is mapped to */
};


/* Prototypes */
struct pci_dev * search_device(struct pci_access *pacc, u8 bus, u8 slot, u8 func);
int convert_hexstring(char *hexstring);
void print_bar_info(struct pci_dev *dev);
void get_bar_size(u32 size, struct bar_info *bar);

/**
 * @brief breaks down the size in Bytes to the biggest available unit
 *
 * @param size		BAR size in bytes
 * @param bar_info	where to store the size
 */
void get_bar_size(u32 size, struct bar_info *bar){
		int i=0;
		if(bar == NULL)
			return;
		bar->size = size;
		while(bar->size > 1023){
			bar->size = bar->size /1024;
			i++;
		}
		if(i > 4)
			i = 4;

		bar->unit = sizes[i];
}

/**
 * @brief Prints out some informations about the BARs in a PCI(e) device
 *
 * @param dev:	device to get BAR infos from
 */
void print_bar_info(struct pci_dev *dev){
	struct bar_info bars[6];
	int i, bar_limit = 6;
	u32 reg_val, reg_val2;
	u64 tmp;
	char *pref[] = {"not ", ""};

	if(dev == NULL)
		return;

	if((pci_read_byte(dev, PCI_HEADER_TYPE) & 0x7f) > 0)
		bar_limit = 2;

	for(i=0;i<bar_limit; i++){
		bars[i].addr = pci_read_long(dev, PCI_BASE_ADDRESS_0 + 4*i); // 32bits

		/* Is BAR used */
		if(bars[i].addr == 0){
			bars[i].type = 0; /* BAR not used */
			printf("BAR %i is not used\n", i);
			continue;
		}

		/* Read out the information */
		pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i, 0xffffffff);
		reg_val = pci_read_long(dev, PCI_BASE_ADDRESS_0 + 4*i);

		if((reg_val & 0x1) == 1){
			/* IO BAR */
			bars[i].type = 1;

			printf("reg_val :%0x\n", reg_val);
			reg_val &= ~(1);			/* Clear first bit */
			printf("reg_val after reg_val &= ~(1) :%0x\n", reg_val);
			reg_val = ~(reg_val);			/* Invert value */
			printf("reg_val after reg_val = ~(reg_val):%0x\n", reg_val);
			reg_val += 1;				/* Add one to get size in bytes*/
			printf("reg_val after reg_val += 1:%0x\n", reg_val);
			get_bar_size(reg_val, &bars[i]);	/* Convert size */

			/* Write Address back */
			pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i, (uint32_t) bars[i].addr);
		}
		else{
			/* Memory BAR */
			bars[i].type = 2;

			/* Is prefetchable? */
			bars[i].prefetchable = (reg_val & (1<<3)) > 0;

			/* 
			   * 3<<1 左移一位 获取 第1和2位
			   * 求值，再右移
			 */
			if(((reg_val & (3<<1))>>1) == 2){
				/* 64 bit BAR */
				bars[i].addr = bars[i].addr + ( (u64) pci_read_long(dev, PCI_BASE_ADDRESS_0 + 4*i+4) << 32);
				pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i +4, 0xffffffff);
				reg_val2 = pci_read_long(dev, PCI_BASE_ADDRESS_0 + 4*i+4);

				tmp = reg_val + (((uint64_t) reg_val2) << 32);	/* get size as a 64 bit value */
				tmp &= ~(0xf);					/* Clear first 4 bits */
				tmp = ~(tmp);					/* Invert value */
				tmp += 1;					/* Add one to get the size in bytes */
				get_bar_size(tmp, &bars[i]);			/* Convert size */

				/* Write Address back */
				pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i, (uint32_t) (bars[i].addr & 0xffffffff));
				pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i+4, (uint32_t) (bars[i].addr >> 32));
			}
			else {
				/* 32 bit BAR */
				bars[i].type = 3;

			    printf("reg_val :%0x\n", reg_val);
				reg_val &= ~(0xf);			/* Clear frst 4 bits */
    			printf("reg_val after reg_val &= ~(0xf) :%0x\n", reg_val);

				reg_val= ~(reg_val);			/* Invert value */
			    printf("reg_val after reg_val= ~(reg_val):%0x\n", reg_val);
				reg_val += 1;				/* Add one to get the size in bytes */
			    printf("reg_val after reg_val += 1:%0x\n", reg_val);
				get_bar_size(reg_val, &bars[i]);	/* Convert size */

				/* Write Address back */
				pci_write_long(dev, PCI_BASE_ADDRESS_0 + 4*i, (uint32_t) bars[i].addr);

			}
		}

		/* Print out BAR infos */
		switch(bars[i].type){
			case 0:
				break;
			case 1:
				printf("BAR %i is a 32 bit IO BAR with a size of %d %s and is mapped to 0x%08x\n", i, (uint32_t) bars[i].size, bars[i].unit, (uint32_t) bars[i].addr);
				break;
			case 2:
				printf("BAR %i is a 32 bit memory BAR with a size of %d %s and is mapped to 0x%08x. The memory is %sprefetchable\n", i, (uint32_t) bars[i].size, bars[i].unit,(uint32_t)  bars[i].addr, pref[bars[i].prefetchable]);
				break;
			case 3:
				printf("BAR %i is a 64 bit memory BAR with a size of %d %s and is mapped to 0x%08lx. The memory is %sprefetchable\n", i,(uint32_t)  bars[i].size, bars[i].unit, bars[i].addr, pref[bars[i].prefetchable]);
				i++;
				break;
			default:
				break;
		}
	}



}

/**
 * @brief searches for a device with a given Bus-, Slot- and Functionnumber
 *
 * @param pacc: PCI Access
 * @param bus: Busnumber
 * @param slot: Slotnumber
 * @param func: Functionnumber
 *
 * @return NULL: No device found
 *		   pointer to device
 */
struct pci_dev * search_device(struct pci_access *pacc, u8 bus, u8 slot, u8 func) {
  struct pci_dev *dev;
  for (dev=pacc->devices; dev; dev=dev->next){	/* Iterate over all devices */
		if( (dev->bus == bus) && (dev->dev == slot) && (dev->func == func))
			return dev;
  }
	  return NULL;
}

/**
 * @brief converts a hexadecimal string to an integer
 *
 * @param hexstring:	String to convert
 *
 * @return:				converted hexstring as integer
 */
int convert_hexstring(char *hexstring) {
	int number;
	if(strstr(hexstring, "0x") == NULL)
		number = (int) strtol(hexstring, NULL, 16);
	else
		number = (int) strtol(hexstring, NULL, 0);
	return number;
}


int main(int argc, char *argv[])
{
  struct pci_access *pacc;
  struct pci_dev *dev;
  u8 bus, slot, func;

  /* Check arguments */
  if(argc != 4){
	  printf("Three Arguments must be passed!\n");
	  printf("Usage: %s [bus] [device] [function]\n", argv[0]);
	  printf("With:\n");
	  printf("\tbus:\tBusnumber of device to print PCI Header\n");
	  printf("\tdevice:\tDevicenumber of device to print PCI Header\n");
	  printf("\tbus:\tFunctionnumber of device to print PCI Header\n");
	  return -1;
  }

  pacc = pci_alloc();		/* Get the pci_access structure */
  /* Set all options you want -- here we stick with the defaults */
  pci_init(pacc);		/* Initialize the PCI library */
  pci_scan_bus(pacc);		/* We want to get the list of devices */

  /* Get numbers */ 
  bus = convert_hexstring(argv[1]);
  slot = convert_hexstring(argv[2]);
  func = convert_hexstring(argv[3]);

  /* Check if device with the passed numbers exist */
  dev = search_device(pacc, bus, slot, func);
  
  if(dev == NULL) {
	  printf("No device found with %x:%x:%x\n", bus, slot, func);
	  return -1;
  }

  /* If device exists, print header */
  print_bar_info(dev);
  
  pci_cleanup(pacc);		/* Close everything */
  return 0;
}
