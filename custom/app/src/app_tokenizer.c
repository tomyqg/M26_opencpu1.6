/*********************************************************************
 *
 * Filename:
 * ---------
 *   app_tokenizer.c 
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
 *====================================================================
 *             HISTORY
 *--------------------------------------------------------------------
 * 
 *********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ql_stdlib.h"
#include "app_tokenizer.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */
 
void tokenizer_reader_init( TokenizerReader* r )
{
    Ql_memset( r, 0, sizeof(*r) );
}

s32 tokenizer_init( Tokenizer* t, const u8* p, const u8* end )
{
    s32 count = 0;

    // the initial '$' is optional
    if (p < end && p[0] == '$')
        p += 1;
    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }
    //get rid of '#' at the end of the sentecne
    if (end >= p+1 && end[-1] == '#') {
        end -= 1;
    }

    while (p < end) {
        u8* q = p;
        q = memchr(p, ',', end-p);
        if (q == NULL){
            q = end;
		}
        if (q >= p) {
            if (count < MAX_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end){
            q += 1;
		}
        p = q;
    }

    if(end[-1] == ',')
    {
		if (count < MAX_TOKENS) {
        	t->tokens[count].p   = end;
            t->tokens[count].end = end;
            count += 1;
        }
    }

    t->count = count;
    return count;
}

Token tokenizer_get( Tokenizer* t, s32 index )
{
    Token  tok;
    static const char* dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else {
        tok = t->tokens[index];
    }
    return tok;
}
 
