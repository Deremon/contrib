/* iso9660.h: */

#ifndef _ISO9660_H_
#define _ISO9660_H_

#include "generic.h"
#include <aros/macros.h>

typedef struct directory_record {
  unsigned char         length;
  unsigned char         ext_attr_length;
#if !AROS_BIG_ENDIAN
  unsigned long         extent_loc;
  unsigned long         extent_loc_m;
#else
  unsigned long         extent_loc_i;
  unsigned long         extent_loc;
#endif
#if !AROS_BIG_ENDIAN
  unsigned long		data_length;
  unsigned long		data_length_m;
#else
  unsigned long		data_length_i;
  unsigned long		data_length;
#endif
  unsigned char		year;
  unsigned char		month;
  unsigned char		day;
  unsigned char		hour;
  unsigned char		minute;
  unsigned char		second;  
  char			tz;
  unsigned char		flags;
  unsigned char		file_unit_size;
  unsigned char		gap_size;
#if !AROS_BIG_ENDIAN
  unsigned short	sequence;
  unsigned short	sequence_m;
#else
  unsigned short	sequence_i;
  unsigned short	sequence;
#endif
  unsigned char		file_id_length;
  char			file_id[1];
} __attribute__((packed) )directory_record;

typedef char time_and_date[17];

typedef struct prim_vol_desc {
  unsigned char		type;
  char			id[5];
  unsigned char		version;
  char			pad1;
  char			system_id[32];
  char			volume_id[32];
  char			pad2[8];
#if !AROS_BIG_ENDIAN
  unsigned long		space_size;
  unsigned long		space_size_m;
#else
  unsigned long		space_size_i;
  unsigned long		space_size;
#endif
  char			pad3[32];
#if !AROS_BIG_ENDIAN
  unsigned short	set_size;
  unsigned short	set_size_m;
#else
  unsigned short	set_size_i;
  unsigned short	set_size;
#endif
#if !AROS_BIG_ENDIAN
  unsigned short	sequence;
  unsigned short	sequence_m;
#else
  unsigned short	sequence_i;
  unsigned short	sequence;
#endif
#if !AROS_BIG_ENDIAN
  unsigned short	block_size;
  unsigned short	block_size_m;
#else
  unsigned short	block_size_i;
  unsigned short	block_size;
#endif
#if !AROS_BIG_ENDIAN
  unsigned long		path_size;
  unsigned long		path_size_m;
#else
  unsigned long		path_size_i;
  unsigned long		path_size;
#endif
#if !AROS_BIG_ENDIAN
  unsigned long         table;
  unsigned long         opt_table;
  unsigned long         m_table;
  unsigned long         opt_m_table;
#else
  unsigned long         i_table;
  unsigned long         opt_i_table;
  unsigned long         table;
  unsigned long         opt_table;
#endif
  directory_record      root;
  char			volume_set_id[128];
  char			publisher_id[128];
  char			data_preparer[128];
  char			application_id[128];
  char			copyright[37];
  char			abstract_file_id[37];
  char			bibliographic_id[37];
  time_and_date		vol_creation;
  time_and_date		vol_modification;
  time_and_date		vol_expiration;
  time_and_date		vol_effective;
  unsigned char		file_structure_version;
  char			pad4;
  char			application_use[512];
  char			reserved[653];
} prim_vol_desc;

/* for internal use only: */

typedef struct iso_vol_info {
  prim_vol_desc		pvd;
  int			skip;
  unsigned short	blockshift;
} t_iso_vol_info;

typedef struct iso_obj_info {
  directory_record	*dir;
  unsigned long		parent_loc; /* for files only */
} t_iso_obj_info;

t_bool Iso_Init_Vol_Info(VOLUME *p_volume, int p_skip, t_ulong p_offset);
t_bool Uses_Iso_Protocol(CDROM *p_cdrom, t_ulong *p_offset);
t_bool Uses_High_Sierra_Protocol(CDROM *p_cdrom);
directory_record *Get_Directory_Record
	(VOLUME *p_volume, unsigned long p_location, unsigned long p_offset);
CDROM_OBJ *Iso_Create_Directory_Obj(VOLUME *p_volume, unsigned long p_location);

#endif /* _ISO9660_H_ */
