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
/** package.c
 *
 *  This file for packaging some images to one package.
 *  The package is named r3.upk.
 *
 *  02/22/2005	        T.Qiu	
 *			Initial creation.
 *  11/07/2007          T.Qiu
 *                      change follow the new UPK structure
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "package.h"

#define SZ_7M  0x700000
#define SZ_8K  0x2000

#define RETRYTIMES  15
#define VER_LIMIT_LEN	14
#define VER_HW2_LEN	4

static package_header_t p_head;
static image_info_t     i_info[10];

static uint32 hw_flag = 0x55AAAA55; /* for judging if have hw */

static void print_image_info(image_info_t *iif)
{
  printf("iif->i_type: %x\n",        iif->i_type);
  printf("iif->i_imagesize: %x\n",   iif->i_imagesize);
  printf("iif->i_startaddr_p: %x\n", iif->i_startaddr_p);
  printf("iif->i_startaddr_f: %x\n", iif->i_startaddr_f);
  printf("iif->i_endaddr_f: %x\n",   iif->i_endaddr_f);
  printf("iif->i_name: %s\n",        iif->i_name);
  printf("iif->i_version: %s\n",     iif->i_version);
}

static void print_head_info(void)
{
  package_header_t *phd = &p_head;

  printf("phd->p_headsize: %x\n", phd->p_headsize);
  printf("phd->p_reserve: %x\n",  phd->p_reserve);
  printf("phd->p_headcrc: %x\n",  phd->p_headcrc);
  printf("phd->p_datasize: %x\n", phd->p_datasize);
  printf("phd->p_datacrc: %x\n",  phd->p_datacrc);
  printf("phd->p_name: %s\n",     phd->p_name);
  printf("phd->p_vuboot: %s\n",   phd->p_vuboot);
  printf("phd->p_vkernel: %s\n",  phd->p_vkernel);
  printf("phd->p_vrootfs: %s\n",  phd->p_vrootfs);
  printf("phd->p_imagenum: %x\n", phd->p_imagenum);
}

static void print_version_info(version_info *ver_t)
{
  printf("ver_t->upk_desc: %s\n", ver_t->upk_desc);
  printf("ver_t->pack_id: %s\n",  ver_t->pack_id);
  printf("ver_t->hw1_ver: %s\n",  ver_t->hw1_ver);
  printf("ver_t->hw2_ver: %s\n",  ver_t->hw2_ver);
  printf("ver_t->os_ver : %s\n",  ver_t->os_ver);
  printf("ver_t->app_ver: %s\n",  ver_t->app_ver);
}

static int pack_firmware(FILE *fp_w, uint32 offst, int num, char *name[])
{
  FILE *fp_r;
  int i, j;
  uint32 curptr, extcrc;
  char ch;
  package_header_t *phd = &p_head;
  image_info_t     *iif;
  struct stat statbuf;
  int isfirst = 1, ithave = 0;

  /* read version file */
  if((fp_r = fopen(UBOOT_VER_FILE, "rb")) == NULL)
    {
      printf("Can't open uboot version file: %s\n", UBOOT_VER_FILE);
      return(-1);
    }
  j=0;
  while(1)
    {
      if(feof(fp_r)) break;
      if(j > VER_LIMIT_LEN+1) 
      {
	   printf("uboot version can't be longer than 14\n");
	   goto bail;
      }
      phd->p_vuboot[j] = fgetc(fp_r);
      if((phd->p_vuboot[j]==0x0d) || (phd->p_vuboot[j]==0x0a))
	phd->p_vuboot[j] = '\0';
      j++;
    }
  fclose(fp_r);

  if((fp_r = fopen(KERNEL_VER_FILE, "rb")) == NULL)
    {
      printf("Can't open kernel version file: %s\n", KERNEL_VER_FILE);
      return(-1);
    }
  j=0;
  while(1)
    {
      if(feof(fp_r)) break;
      if(j > VER_LIMIT_LEN+1)
      {
	   printf("kernel version can't be longer than 14\n");
	   goto bail;
      }
      phd->p_vkernel[j]=fgetc(fp_r);
      if((phd->p_vkernel[j]==0x0d) || (phd->p_vkernel[j]==0x0a)) 
	 phd->p_vkernel[j] = '\0';
      j++; 
    }
  fclose(fp_r);

  if((fp_r = fopen(ROOTFS_VER_FILE, "rb")) == NULL)
    {
      printf("Can't open rootfs version file: %s\n", ROOTFS_VER_FILE);
      return(-1);
    }
  j=0;
  while(1)
    {
      if(feof(fp_r)) break;
      if(j > VER_LIMIT_LEN+1)
      {
	   printf("rootfs version can't be longer than 14\n");
	   goto bail;
      }
      phd->p_vrootfs[j] = fgetc(fp_r);
      if((phd->p_vrootfs[j]==0x0d) ||(phd->p_vrootfs[j]==0x0a)) 
	phd->p_vrootfs[j] = '\0';
      j++;
    }
  fclose(fp_r);

  /* if rootfs size bigger than 7M, split it to two*/
  for(i=0; i<num; i++)
    {
      if(strncmp(name[i], CRAMFS_FILE_NAME, strlen(CRAMFS_FILE_NAME)) == 0)
	{
	    ithave=1;
	    break;
	}
    }
  if(ithave)
    {
      if(stat(CRAMFS_FILE_NAME, &statbuf)<0)
	{
	  printf("can't stat root.cramfs\n");
	  phd->p_imagenum = (uint8)num;
	}
      else if( statbuf.st_size > (CRAMFS_ADDR_END2+ 1 - CRAMFS_ADDR_START1) )
	{
	    printf("Error: the %s size is larger than the flash assigned to it!!!\n", CRAMFS_FILE_NAME);
	    return -1;
	}
      else if(statbuf.st_size > SZ_7M)
	{
	  printf("root.cramfs size bigger than SZ_7M\n");
	  phd->p_imagenum = (uint8)num+1;
	  for(j=num; j>i; j--)
	      name[j] = name[j-1];
	}
      else phd->p_imagenum = (uint8)num;
    }
  else phd->p_imagenum = (uint8)num;
  phd->p_headsize = sizeof(package_header_t) + phd->p_imagenum * sizeof(image_info_t);

  /* Bit[1:0] use to indicate 8M or 16M flash package */
#if FLASH_16M
  phd->p_reserve  = 0x02; 
#else
  phd->p_reserve  = 0x01;
#endif
  phd->p_reserve |= 0x04;

  phd->p_datasize = 0;
  phd->p_datacrc  = 0;
  phd->p_headcrc  = 0;

  curptr = phd->p_headsize + sizeof(version_info);

  for(i=0; i < phd->p_imagenum; i++)
    {
      /* image info */
      iif = &i_info[i];
      if(strncmp(name[i], CRAMFS_FILE_NAME, strlen(CRAMFS_FILE_NAME)) == 0)
	{
	  iif->i_type = IH_TYPE_CRAMFS;
	  strncpy((char *)iif->i_name, CRAMFS_FILE_NAME, NAMELEN-1);

	  if((fp_r = fopen(ROOTFS_VER_FILE, "rb")) == NULL)
	    {
	      printf("Can't open kernel version file: %s\n", ROOTFS_VER_FILE);
	      break;
	    }
	  for(j = 0; j < sizeof(iif->i_version); j++)
	    {
	      if(feof(fp_r)) break;
	      iif->i_version[j] = fgetc(fp_r);
	      if((iif->i_version[j]==0x0d) || (iif->i_version[j]==0x0a))
		 iif->i_version[j] = '\0';
	    }
	  fclose(fp_r);
	}
      else if(strncmp(name[i], KERNEL_FILE_NAME, strlen(KERNEL_FILE_NAME)) == 0)
	{
	  iif->i_type = IH_TYPE_KERNEL;
	  strncpy((char *)iif->i_name, KERNEL_FILE_NAME, NAMELEN-1);

	  if((fp_r = fopen(KERNEL_VER_FILE, "rb")) == NULL)
	    {
	      printf("Can't open kernel version file: %s\n", KERNEL_VER_FILE);
	      break;
	    }
	  for(j = 0; j < sizeof(iif->i_version); j++)
	    {
	      if(feof(fp_r)) break;
	      iif->i_version[j] = fgetc(fp_r);
	      if((iif->i_version[j]==0x0d) ||(iif->i_version[j]==0x0a))
		iif->i_version[j] = '\0';
	    }
	  fclose(fp_r);
	}
      else if(strncmp(name[i], UBOOT_FILE_NAME, strlen(UBOOT_FILE_NAME)) == 0)
	{
	  iif->i_type = IH_TYPE_UBOOT;
	  strncpy((char *)iif->i_name, UBOOT_FILE_NAME, NAMELEN-1);

	  if((fp_r = fopen(UBOOT_VER_FILE, "rb")) == NULL)
	    {
	      printf("Can't open uboot version file: %s\n", UBOOT_VER_FILE);
	      break;
	    }
	  for(j = 0; j < sizeof(iif->i_version); j++)
	    {
	      if(feof(fp_r)) break;
	      iif->i_version[j] = fgetc(fp_r);
	      if((iif->i_version[j]==0x0d)|| (iif->i_version[j]==0x0a))
		 iif->i_version[j] = '\0';
	    }
	  fclose(fp_r);
	}
      else if(strncmp(name[i], SCRIPT_FILE_NAME, strlen(SCRIPT_FILE_NAME)) == 0)
	{
	  iif->i_type = IH_TYPE_SCRIPT;
	  strncpy((char *)iif->i_name, SCRIPT_FILE_NAME, NAMELEN-1);
	}
      else
	{
	    iif->i_type = IH_TYPE_COMPRESS;
	    strncpy((char *)iif->i_name, name[i], NAMELEN-1);
	    if((fp_r = fopen(EXTAPP_VER_FILE, "rb")) == NULL)
	      {
		  printf("Can't open extapp version file: %s\n", EXTAPP_VER_FILE);
		  break;
	      }
	    j=0;
	    while(1)
	      {
		  if(feof(fp_r)) break;
		  if(j > VER_LIMIT_LEN+1)
		  {
		       printf("\nextapp version can't be longer than 14\n");
		       goto bail;
		  }
		  iif->i_version[j] = fgetc(fp_r);
		  if((iif->i_version[j]==0x0d)|| (iif->i_version[j]==0x0a))
		      iif->i_version[j] = '\0';
		  j++;
	      }
	    fclose(fp_r);
	}

      /* address in flash*/
      switch(iif->i_type)
	{
	case IH_TYPE_CRAMFS:
	  if(isfirst)
	    {
	      iif->i_startaddr_f = CRAMFS_ADDR_START1;
	      iif->i_endaddr_f   = CRAMFS_ADDR_END1;
	    }
	  else
	    {
	      iif->i_startaddr_f = CRAMFS_ADDR_START2;
	      iif->i_endaddr_f   = CRAMFS_ADDR_END2;	    
	    }
	  break;
	case IH_TYPE_KERNEL:
	  iif->i_startaddr_f = KERNEL_ADDR_START;
	  iif->i_endaddr_f   = KERNEL_ADDR_END;
	  break;       
	case IH_TYPE_UBOOT:
	  iif->i_startaddr_f = UBOOT_ADDR_START;
	  iif->i_endaddr_f   = UBOOT_ADDR_END;
	  break;
	case IH_TYPE_SCRIPT:
	case IH_TYPE_COMPRESS:
	  break;
	default:
	  printf("un-handle image type\n");
	  break;
	}

      /* write whole image to package and calculate the imagesize*/
      iif->i_imagesize = 0;
      /* images file */
      if((fp_r = fopen(name[i], "rb")) == NULL)
	{
	  printf("can't open file: %s\n", name[i]);
	  break;
	}

      if(iif->i_type == IH_TYPE_CRAMFS && !isfirst)
	  fseek(fp_r, SZ_7M, SEEK_SET);

      fseek(fp_w, offst+curptr,SEEK_SET);
      extcrc = 0;
      while(!feof(fp_r))
	{
	  ch = fgetc(fp_r);
	  fputc(ch, fp_w);
	  if(iif->i_type != IH_TYPE_COMPRESS)
	      phd->p_datacrc = crc32(phd->p_datacrc,(uint8 *)&ch, 1);
	  else
	      extcrc = crc32(extcrc,(uint8 *)&ch, 1);
	  iif->i_imagesize ++;
	  if(iif->i_type == IH_TYPE_CRAMFS)
	    {
	      if(isfirst && (iif->i_imagesize >= SZ_7M))
		{
		  isfirst = 0;
		  break;
		}
	    }
	}
      fclose(fp_r);
      if(iif->i_type == IH_TYPE_COMPRESS)
	{
	    /* write ext app crc */
	    if(fwrite(&extcrc, sizeof(extcrc), 1, fp_w) != 1)
	      {
		  printf("can not write ext crc into package");
		  return(0);
	      }
	    iif->i_imagesize += sizeof(extcrc);
	}

      iif->i_startaddr_p = curptr;
      curptr += iif->i_imagesize;
      if(iif->i_type != IH_TYPE_COMPRESS)
	  phd->p_datasize += iif->i_imagesize;
      
      print_image_info(iif); /* print iff*/

       /*write image info */
      fseek(fp_w, offst+sizeof(package_header_t)+i*sizeof(image_info_t), 0);
      if(fwrite(iif, sizeof(image_info_t), 1, fp_w) != 1) 
	{
	  printf("can not write iif into package\n");
	  break;
	}
    }

  /* write package head*/
  phd->p_headcrc = crc32(phd->p_headcrc, (uint8 *)phd, sizeof(package_header_t));
  phd->p_headcrc = crc32(phd->p_headcrc, (uint8 *)i_info, phd->p_imagenum*sizeof(image_info_t));

  print_head_info();  /* print phd */

  fseek(fp_w, offst, SEEK_SET);
  if(fwrite((uint8 *)phd, sizeof(package_header_t), 1, fp_w) != 1)
    {
      printf("can not write head into package");
      return(-1);
    }
  return 0;

bail:
  fclose(fp_r);

  return -1;
}

static uint32 pack_hw(FILE *fp_w, char *name[])
{
     FILE *fp_r;
     char ch;
     uint32 hw_len = 0, hw2_crc = 0, hw1_len =0, hw2_len=0;
     int i;
     
     if((fp_r = fopen(name[0], "rb")) ==NULL)
     {
	  printf("can't open %s\n", name[0]);
	  return (0);
     }
     else
     {/* write the first one into package */
	  while(!feof(fp_r)) 
	  {
	       fputc(fgetc(fp_r), fp_w);
	       hw1_len++;
	  }
	  fclose(fp_r);
     }
     /* fill the space with 0 first */
     if(fwrite(&hw2_crc, sizeof(hw2_crc), 1, fp_w) != 1 )
     {
	  printf("can't not write hw2_crc1 into package\n");
	  return (0);
     }
     if(fwrite(&hw2_len, sizeof(hw2_len), 1, fp_w) != 1 )
     {
	  printf("can't not write hw2_len1 into package\n");
	  return(0);
     }
     
     if((fp_r = fopen(name[1], "rb")) ==NULL)
     {
	  printf("can't open %s\n", name[1]);
	  return(0);
     }
     else
     {/* write the second one into package */
	  while(!feof(fp_r))
	  {
	       ch = fgetc(fp_r);
	       fputc(ch, fp_w);
	       hw2_crc = crc32(hw2_crc, (uint8 *)&ch, 1);
	       hw2_len++;
	  }
	  fclose(fp_r);
     }
     //printf("hw2_crc = %x\n", hw2_crc);
     /* write the actual value */
     fseek(fp_w, hw1_len, SEEK_SET); 
     if(fwrite(&hw2_crc, sizeof(hw2_crc), 1, fp_w) != 1 )
     {
	  printf("can't not write hw2_crc2 into package\n");
	  return(0);
     }
     if(fwrite(&hw2_len, sizeof(hw2_len), 1, fp_w) != 1 )
     {
	  printf("can't not write hw2_len2 into package\n");
	  return(0);
     }
     
     hw_len = hw1_len+hw2_len+sizeof(hw2_crc)+sizeof(hw2_len);
     printf("hw_len = %x\n", hw_len);
     fseek(fp_w, hw_len, SEEK_SET);
     
     for(i=0; i< RETRYTIMES; i++)
     {
	  if( hw_len < (i*SZ_8K) )
	  {
	       int j;
	       for(j=hw_len; j<(i*SZ_8K); j++)
		    fputc(0, fp_w);
	       hw_len = (i*SZ_8K);
	       break;
	  }
	  else if( hw_len == (i*SZ_8K) ) 
	       break;
     }
     if(i == RETRYTIMES) 
     {
	  printf("Oops, the hw parts is too big!\n");
	  return 0;
     }
     
     return hw_len;
}

static int pack_signature(FILE *fp_w)
{
    signature_t signature;

    memset((char *)&signature, 0, sizeof(signature_t));
    memcpy(signature.string, NEUROS_UPK_SIGNATURE, strlen(NEUROS_UPK_SIGNATURE));
    signature.strcrc = crc32(0, signature.string, sizeof(signature.string));
    if(fwrite((uint8 *)&signature, sizeof(signature_t), 1, fp_w) != 1)
      {
	  printf("can not write the signature into package\n");
	  return(0);
      }
    return sizeof(signature_t);
}

static int pack_ver_info(FILE *fp_w, uint32 offset, int flag, char *desc)
{
  version_info ver_t;
  FILE *fp_r;
  int i;
  
  memset((char *)&ver_t, 0, sizeof(version_info));

  if(strlen(desc) >= DESCLEN)
    {
      printf("The upk_desc is too long\n");
      return(-1);
    }
  strncpy((char *)ver_t.upk_desc, desc, DESCLEN-1);
  strncpy((char *)ver_t.pack_id, (char *)PACKAGE_ID, NAMELEN-1);
  strncpy((char *)ver_t.hw1_ver, "D.ev", VERLEN-1);
  strncpy((char *)ver_t.hw2_ver, "D.ev", VERLEN-1);
  strncpy((char *)ver_t.os_ver,  "0.00", VERLEN-1);
  strncpy((char *)ver_t.app_ver, "0.00", VERLEN-1);

  if(flag)
    {
      if((fp_r = fopen(HW1_VER_FILE, "rb")) == NULL)
	{
	  printf("Can't open HW1 version file: %s\n", HW1_VER_FILE);
	  return(-1);
	}
      for(i = 0; i < sizeof(ver_t.hw1_ver); i++)
	{
	  if(feof(fp_r)) break;
	  ver_t.hw1_ver[i] = fgetc(fp_r);
	  if((ver_t.hw1_ver[i]==0x0d) || (ver_t.hw1_ver[i]==0x0a))
	    ver_t.hw1_ver[i] = '\0';
	}
      fclose(fp_r);
      
      if((fp_r = fopen(HW2_VER_FILE, "rb")) == NULL)
	{
	  printf("Can't open HW2 version file: %s\n", HW2_VER_FILE);
	  return(-1);
	}
      i = 0;
      while(1)
	{
	  if(feof(fp_r)) break;
	  if(i > VER_HW2_LEN+1)
	  {
	       printf("hw version can't be longer than 4\n");
	       fclose(fp_r);
	       return (-1);
	  }
	  ver_t.hw2_ver[i] = fgetc(fp_r);
	  if((ver_t.hw2_ver[i]==0x0d) || (ver_t.hw2_ver[i]==0x0a))
	    ver_t.hw2_ver[i] = '\0';
	  i++;
	}
      fclose(fp_r);
    }
  
  if((fp_r = fopen(KERNEL_VER_FILE, "rb")) == NULL)
    {
      printf("Can't open OS version file: %s\n", KERNEL_VER_FILE);
      return(-1);
    }
  for(i = 0; i < sizeof(ver_t.os_ver); i++)
    {
      if(feof(fp_r)) break;
      ver_t.os_ver[i] = fgetc(fp_r);
      if((ver_t.os_ver[i]==0x0d) || (ver_t.os_ver[i]==0x0a))
	ver_t.os_ver[i] = '\0';
    }
  fclose(fp_r);

  if((fp_r = fopen(ROOTFS_VER_FILE, "rb")) == NULL)
    {
      printf("Can't open App version file: %s\n", ROOTFS_VER_FILE);
      return(-1);
    }
  for(i = 0; i < sizeof(ver_t.app_ver); i++)
    {
      if(feof(fp_r)) break;
      ver_t.app_ver[i] = fgetc(fp_r);
      if((ver_t.app_ver[i]==0x0d) || (ver_t.app_ver[i]==0x0a))
	ver_t.app_ver[i] = '\0';
    }
  fclose(fp_r);

  fseek(fp_w, offset, SEEK_SET);
  if(fwrite((uint8 *)&ver_t, sizeof(version_info), 1, fp_w) != 1)
    {
      printf("can not write the version struct into package\n");
      return(-1);
    }
  fseek(fp_w, 0, SEEK_END);
  if(fwrite((uint8 *)&ver_t, sizeof(version_info), 1, fp_w) != 1)
    {
      printf("can not write the version struct into package\n");
      return(-1);
    }

  print_version_info(&ver_t);

  return (0);
}

int main(int argc, char *argv[])
{
  FILE *fp_w;
  uint32 hw_len = 0, siglen;
  int flag, img_pos;
  package_header_t *phd = &p_head;

  printf("\npackage tool version %s ", VERSION);
  #if FLASH_16M
  printf("for 16M board\n\n");
  #else
  printf("for 8M board\n\n");
  #endif

  if(argc < 4)
    {
      printf("usage: packet flag upk_desc package_name hw1 hw2 image1 image2 ...\n");
      return(-1);
    }

  if(strcmp(argv[1], "hh") == 0)  
    {
      flag = 1;    /* has hw */
      img_pos = 6;
    }
  else if(strcmp(argv[1], "nh") == 0) 
    {
      flag =0; /* has no hw*/
      img_pos =4;
    }
  else 
    {
      printf("ERROR:pass wrong flag\n");
      return(-1);
    }

  if(argc < img_pos+1)
    {
      if(flag)
	  printf("usage: packet flag upk_desc package_name hw1 hw2 image1 image2 ...\n");
      else 
	  printf("usage: packet flag upk_desc package_name image1 image2 ...\n");
      return(-1);
    }

  strncpy((char *)phd->p_name, argv[3], NAMELEN-1);
  if((fp_w = fopen((char *)phd->p_name, "wb+")) == NULL)
    {
      printf("Can't open %s\n",phd->p_name);
      return(-1);
    }

  /* packet hw to package */
  if(flag)
  {
       if((hw_len = pack_hw(fp_w, &argv[4])) == 0)
	    return (-1);
  }
  /* packet the new signature */
  if((siglen = pack_signature(fp_w)) == 0)
      return (-1);
  hw_len += siglen;
  /* packet firmware to package */
  if(pack_firmware(fp_w, hw_len, argc-img_pos, &argv[img_pos]) != 0) 
    return (-1);
  /* packet upk_desc and version info */
  if(pack_ver_info(fp_w, hw_len+phd->p_headsize, flag, argv[2]) != 0)
    return (-1);

  /* write hw flag and hw_len */
  fseek(fp_w, 0, SEEK_END);
  if(fwrite(&hw_flag, sizeof(hw_flag), 1, fp_w) != 1)
    {
      printf("can not write hw flag into package");
      return(-1);
    }
  if(fwrite(&hw_len, sizeof(hw_len), 1, fp_w) != 1)
    {
      printf("can not write hw_len into package");
      return(-1);
    }

  fclose(fp_w);

  return 0;
}


