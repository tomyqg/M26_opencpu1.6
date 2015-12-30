/*****************************************************************************
 *
 * Filename:
 * ---------
 *   app_tokenizer.h 
 *
 * Project:
 * --------
 * --------
 *
 * Description:
 * ------------
 * ------------
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/


#ifndef __APP_TOKENIZER_H__
#define __APP_TOKENIZER_H__

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

#define  TOKENIZER_READER_MAX_SIZE  100
typedef struct {
    s32     length;
	u8      in[ TOKENIZER_READER_MAX_SIZE];
} TokenizerReader;

typedef struct {
    const u8*  p;
    const u8*  end;
} Token;

#define  MAX_TOKENS  10
typedef struct {
    int     count;
    Token   tokens[ MAX_TOKENS ];
} Tokenizer;

/*********************************************************************
 * VARIABLES
 */
 
/*********************************************************************
 * FUNCTIONS
 */

void tokenizer_reader_init( TokenizerReader* r );
s32 tokenizer_init( Tokenizer* t, const u8* p, const u8* end );
Token tokenizer_get( Tokenizer* t, s32 index );

#endif // End-of __APP_GPS_H__ 



