
#ifndef SOSSE_FSTREAM_H
#define SOSSE_FSTREAM_H

#include "..\export_new\types.h"

//! Stream struct.
typedef struct s_fstream {
	iu32 start;	//!< EEPROM start address of the stream.
	iu16 size;	//!< Data size of the stream.
	iu16 pos;		//!< Current position in the stream.
} S_FSTREAM;

bool fstream_read( S_FSTREAM *fs, iu8 *dst, iu8 len );

bool fstream_test( const S_FSTREAM *fs, iu16 len );

#endif /* SOSSE_FSTREAM_H */

