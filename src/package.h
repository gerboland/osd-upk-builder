/*
 *  Copyright(C) 2005 Neuros Technology International LLC. 
 *               <www.neurostechnology.com>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that, in addition to its 
 *  original purpose to support Neuros hardware, it will be useful 
 *  otherwise, but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *****************************************************************************/
/** package.h
 *
 * Some structure for package.c
 *
 * 02/22/2002	        T.Qiu	
 *			Initial creation.
 */

#ifndef PACKAGE_H
#define PACKAGE_H

//#define VERSION "3.01"
#define FLASH_16M 1
#define NAMELEN   32
#define VERLEN    20 /* should same as the uboot */
#define DESCLEN   40
#define SIGNATURELEN 52

/* image type*/
#define IH_TYPE_INVALID		0	/* Invalid Image		*/
#define IH_TYPE_STANDALONE	1	/* Standalone Program		*/
#define IH_TYPE_KERNEL		2	/* OS Kernel Image		*/
#define IH_TYPE_RAMDISK		3	/* RAMDisk Image		*/
#define IH_TYPE_MULTI		4	/* Multi-File Image		*/
#define IH_TYPE_FIRMWARE	5	/* Firmware Image		*/
#define IH_TYPE_SCRIPT		6	/* Script file			*/
#define IH_TYPE_FILESYSTEM	7	/* Filesystem Image (any type)	*/
#define IH_TYPE_UBOOT           8       /* UBOOT FILE                   */
#define IH_TYPE_CRAMFS          9       /* CRAMFS Image                 */
#define IH_TYPE_COMPRESS        10      /* Compress files               */

/* addr in flash */
#if FLASH_16M
#define UBOOT_ADDR_START      0x00100000
#define UBOOT_ADDR_END        0x0013FFFF
#define KERNEL_ADDR_START     0x00160000
#define KERNEL_ADDR_END       0x002DFFFF
#define CRAMFS_ADDR_START1    0x002E0000
#define CRAMFS_ADDR_END1      0x009DFFFF
#define CRAMFS_ADDR_START2    0x009E0000
#define CRAMFS_ADDR_END2      0x00FFFFFF
#define JFFS_ADDR_START       0x01000000
#define JFFS_ADDR_END         0x010FFFFF
#else
#define UBOOT_ADDR_START      0x00100000
#define UBOOT_ADDR_END        0x0013FFFF
#define KERNEL_ADDR_START     0x00160000
#define KERNEL_ADDR_END       0x0027FFFF
#define CRAMFS_ADDR_START     0x00280000
#define CRAMFS_ADDR_END       0x0088FFFF
#define JFFS_ADDR_START       0x00890000
#define JFFS_ADDR_END         0x008EFFFF
#define RAMDISK_ADDR_START    0x008F0000
#define RAMDISK_ADDR_END      0x008FFFFF
#endif

#define UBOOT_FILE_NAME   "u-boot.bin"
#define KERNEL_FILE_NAME  "uImage"
#define CRAMFS_FILE_NAME  "root.cramfs"
#define SCRIPT_FILE_NAME  "env.img"

#define UBOOT_VER_FILE    "u-boot.version"
#define KERNEL_VER_FILE   "uImage.version"
#define ROOTFS_VER_FILE   "rootfs.version"
#define EXTAPP_VER_FILE   "extapp.version"

#define HW1_VER_FILE      "hw1.version"
#define HW2_VER_FILE      "hw2.version"

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;
typedef unsigned long  uint64;

#define SWAP_LONG(x) \
	((uint32)( \
		(((uint32)(x) & (uint32)0x000000ffUL) << 24) | \
		(((uint32)(x) & (uint32)0x0000ff00UL) <<  8) | \
		(((uint32)(x) & (uint32)0x00ff0000UL) >>  8) | \
		(((uint32)(x) & (uint32)0xff000000UL) >> 24) ))

#define SWAP_SHORT(x) \
         ((uint16)(   \
	         (((uint16)(x) & (uint16)0x00ff) << 8) | \
	         (((uint16)(x) & (uint16)0xff00) >> 8))

#define     ntohl(a)	SWAP_LONG(a)
#define     htonl(a)	SWAP_LONG(a)
#define     ntohs(a)    SWAP_SHORT(a)
#define     htons(a)    SWAP_SHORT(a)

typedef struct packet_header{
  uint32    p_headsize;       /* package header size         */
  uint32    p_reserve;        /* Bit[1:0] for indicate 8M or 16M, other reserve */
  uint32    p_headcrc;        /* package header crc checksum */
  uint32    p_datasize;       /* package data size           */
  uint32    p_datacrc;        /* package data crc checksum   */
  uint8     p_name[NAMELEN];  /* package name                */
  uint8     p_vuboot[VERLEN]; /* version of uboot which depend on */
  uint8     p_vkernel[VERLEN];/* version of kernel which depend on*/
  uint8     p_vrootfs[VERLEN];/* version of rootfs which depend on*/
  uint32    p_imagenum;       /* num of the images in package*/
                              /* follow is image info */
}package_header_t;

typedef struct image_info{
  uint32    i_type;           /* image type                */
  uint32    i_imagesize;      /* size of image             */
  uint32    i_startaddr_p;    /* start address in packeage */
  uint32    i_startaddr_f;    /* start address in flash    */
  uint32    i_endaddr_f;      /* end address in flash      */
  uint8     i_name[NAMELEN];  /* image name                */
  uint8     i_version[VERLEN];/* image version             */
}image_info_t;

typedef struct image_header {
	uint32	        ih_magic;	/* Image Header Magic Number	*/
	uint32	        ih_hcrc;	/* Image Header CRC Checksum	*/
	uint32	        ih_time;	/* Image Creation Timestamp	*/
	uint32	        ih_size;	/* Image Data Size		*/
	uint32	        ih_load;	/* Data	 Load  Address		*/
	uint32	        ih_ep;		/* Entry Point Address		*/
        uint32	        ih_dcrc;	/* Image Data CRC Checksum	*/
	uint8		ih_os;		/* Operating System		*/
	uint8		ih_arch;	/* CPU architecture		*/
	uint8		ih_type;	/* Image Type			*/
	uint8		ih_comp;	/* Compression Type		*/
	uint8		ih_name[NAMELEN];	/* Image Name		*/
} image_header_t;

#define NEUROS_UPK_SIGNATURE "Neuros Technology International LLC"
typedef struct signature_struct{
    uint8    string[SIGNATURELEN];
    uint32   strcrc;
}signature_t;

#define PACKAGE_ID "neuros-osd"
typedef struct version_struct{
  uint8  upk_desc[DESCLEN];
  uint8  pack_id[NAMELEN];
  uint8  hw1_ver[VERLEN];
  uint8  hw2_ver[VERLEN];
  uint8  os_ver [VERLEN];
  uint8  app_ver[VERLEN];
}version_info;


extern unsigned long crc32 (unsigned long, const unsigned char *, unsigned int);

#endif

