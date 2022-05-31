/*
 *	lspcicaps -- lists all capabilities of a PCI(e) device              
 *
 *	Written by Johannes 4Linux and put to public domain. You can do
 *	with it anything you want, but I don't give you any warranty.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib/pci.h"

/** Array contains all PCI Capabilities */
const char *pci_caps[] = {
		"Null Capability",						/* ID 0x0 */
		"PCI Power Management Interface",
		"Accelerated Graphics Port",
		"Vital Product Data",
		"Slot Identification", 
		"Message Signaled Interrupt",
		"Compact PCI Hot Swap",
		"PCI-X",
		"Hyper Transport",
		"Vendor-Specific",
		"Debug Port",
		"CompactPCI",
		"PCI Hot-Plug",
		"PCI Bridge Subsystem Vendor ID",
		"AGP 8x",
		"Secure Device",
		"PCI Express",
		"MSI-X",
		"Serial ATA Data",
		"Advanced Features",
		"Enhanced Allocation",
		"Flattening Portal Bridge",				/* ID: 0x15 */
		"Reserved"
};

/** Array contains all extended PCIe Capabilities */
const char *pcie_caps[] = {
	"Null Capability",
	"Advanced Error Reporting",
	"Virtual Channel",
	"Device Serial Number",
	"Power Budgeting",
	"Root Complex Link Declaration",
	"Root Complex Internal Link control",
	"Root Complex Event Collector Endpoint Association",
	"Multi-Function Virtual Channel",
	"Virtual Channel",
	"Vendor Specific Extended Capability",
	"Configuration Access Correlation",
	"Access Control Services",
	"Alternative Routing-ID Interpretation",
	"Address Translation Services",
	"Single Root I/O Virtualization",
	"Multi-Root I/O Virtualization",
	"Multicast",
	"Page Request Interface",
	"Reserved for AMD",
	"Resizable BAR",
	"Dynamic Power Allocation",
	"TPH, Requester",
	"Latency Tolerance Reporting",
	"Secondary PCI Express",
	"Protocol Mulitplexing",
	"Process Address Space",
	"LN Requester",
	"Downstream Port Containment",
	"L1 PM Substates",
	"Precision Time Measurement",
	"PCI Express over M-PHY",
	"FRS Queueing",
	"Readiness Time Reporting",
	"Designated Vendor-Specific Extended Capability",
	"VF Resizable BAR",
	"Data Link Feature",
	"Physical Layer 16.0 GT/s",
	"Lane Margining at the Receiver",
	"Hierarchy ID",
	"Native PCIe Enclosure Management",
	"Physical Layer 32.0 GT/s",
	"Alternate Protocol",
	"System Firmware Intermediary",
	"Reserved"
};

/* Prototypes */
struct pci_dev * search_device(struct pci_access *pacc, u8 bus, u8 slot, u8 func);
int convert_hexstring(char *hexstring);
void print_all_capabilities(struct pci_dev *dev);

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

/**
 * @brief searches a PCI(e) device for all possible capabilities 
 *
 * @param dev			PCI Device for checking capabilites
 */
void print_all_capabilities(struct pci_dev *dev) {
	u16 next_ptr = 0, cap_id = 0;
	u32 pcie_cap_id;
	u8 is_pcie = 0;

	if(dev == NULL)
			return;

	/* Check if the device has any PCI capabilites */
	next_ptr = pci_read_byte(dev, PCI_CAPABILITY_LIST);

	if(next_ptr == 0) {
		printf("PCI(e) device doesn't have any capabilities!\n");
		return;
	}

	/* Search for all PCI extended capabilites */

	while(next_ptr) {
		cap_id = pci_read_byte(dev, next_ptr);
		if(cap_id == 0xff){
			printf("Error reading Capability ID... Try again as root!\n");
			return;
		}

		/* Capability Pointer  -- 34H
		 * PCI Capability ID
		 * Next Capability Pointer 
		 */
		if(cap_id > 0x15) 
			cap_id = 0x16;
		printf("ID: 0x%x - %s\n", cap_id, pci_caps[cap_id]);
		next_ptr = pci_read_byte(dev, next_ptr + 0x1);
		if(cap_id == 0x10)
			is_pcie = 1;
	}

	/* If device is PCI Express device, read its PCIe capabilities */
	if(!is_pcie)
		return;
	printf("Printing PCI Express Capabilities.\n");

	next_ptr = 0x100;
	do {
		/* But now the capability header looks different:
		 * Bit 15:0 PCI Express Capability ID
		 * Bit 19:16 Capability Version Number
		 * Bit 31:20 Next Capability Offset
		 */
		pcie_cap_id = pci_read_long(dev, next_ptr);  // 64 bits
		printf("pcie_cap_id: %d.\n", pcie_cap_id);

		next_ptr = pcie_cap_id >> 20;
		// 注意这里是offset， 所以用移位
		// * Bit 31:20 Next Capability Offset
		cap_id = pcie_cap_id & 0xff;  //  8bit
		if(cap_id > 0x2c)
			cap_id = 0x2d;
		printf("ID: 0x%x - %s\n", cap_id, pcie_caps[cap_id]);
	} while(next_ptr);
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
  print_all_capabilities(dev);
  
  pci_cleanup(pacc);		/* Close everything */
  return 0;
}
