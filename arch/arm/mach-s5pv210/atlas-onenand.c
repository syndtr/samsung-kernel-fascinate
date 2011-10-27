/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/onenand.h>

#include "atlas.h"


static struct mtd_partition atlas_partition_info[] = {

   /*This is partition layout from the oneNAND it SHOULD match the pitfile on the second page of the NAND.
   It will work if it doesn't but beware to write below the adress 0x01200000 there are the bootloaders.
   Currently we won't map them, but we should keep that in mind for later things like flashing bootloader
   from Linux. There is a partition 'efs' starting @ 0x00080000 40 256K pages long, it contains data for
   the modem like IMSI we don't touch it for now, but we need the data from it, we create a partition
   for that and copy the data from it. For this you need a image from it and mount it as vfat or copy
   it on a kernel with rfs support on the phone.
   
   Partitions on the lower NAND adresses:
   
   0x00000000 - 0x0003FFFF = first stage bootloader
   0x00040000 - 0x0007FFFF = PIT for second stage bootloader
   0x00080000 - 0x00A7FFFF = EFS: IMSI and NVRAM for the modem
   0x00A80000 - 0x00BBFFFF = second stage bootloader
   0x00BC0000 - 0x00CFFFFF = backup of the second stage bootloader (should be loaded if the other fails, unconfirmed!)
   0x00D00000 - 0x011FFFFF = PARAM.lfs config the bootloader
   
   #########################################################################################
   #########################################################################################
   ###### NEVER TOUCH THE FIRST 2 256k PAGES! THEY CONTAIN THE FIRST STAGE BOOTLOADER ######
   #########################################################################################
   #########################################################################################*/ 
                                                                   
	{
		.name		= "boot",
		.offset		= (72*SZ_256K),
		.size		= (30*SZ_256K),
	},
	{
		.name		= "recovery",
		.offset		= (102*SZ_256K),
		.size		= (30*SZ_256K),
	},
	{
		.name		= "system",
		.offset		= (132*SZ_256K),
		.size		= (1100*SZ_256K),
	},
	{
		.name		= "cache",
		.offset		= (1232*SZ_256K),
		.size		= (100*SZ_256K),
	},
	{
		.name		= "datadata",
		.offset		= (1332*SZ_256K),
		.size		= (672*SZ_256K),
	},
	{
		.name           = "efs",
		.offset         = (2*SZ_256K),
		.size           = (40*SZ_256K),
	},
	{       /* The reservoir area is used by Samsung's Block Management Layer (BML)
	           to map good blocks from this reservoir to bad blocks in user
	           partitions. A special tool (bml_over_mtd) is needed to write
	           partition images using bad block mapping.
	           Currently, this is required for flashing the "boot" partition,
	           as Samsung's stock bootloader expects BML partitions.*/
		.name		= "reservoir",
		.offset		= (2004*SZ_256K),
		.size		= (44*SZ_256K), //2047
	},
};

static struct onenand_platform_data onenand_pdata = {
	.parts		= atlas_partition_info,
	.nr_parts	= ARRAY_SIZE(atlas_partition_info),
};

void onenand_init()
{
	s5pc110_onenand_set_platdata(&onenand_pdata);
}
