/*	bitset.h

	The following is a general-purpose set library originally developed
	by Hank Dietz and enhanced by Terence Parr to allow dynamic sets.

	Sets are now structs containing the #words in the set and
	a pointer to the actual set words.

	1987 by Hank Dietz

	Modified by:
		Terence Parr
		Purdue University
		October 1989

		Added ANSI prototyping Dec. 1992 -- TJP
*/

/* Define usable bits for */
#define BytesPerWord		sizeof(unsigned)
#define	WORDSIZE_IN_BITS	(sizeof(unsigned)*8)
#define LogWordSize     	(WORDSIZE_IN_BITS==64?6:5)

#define	SETSIZE(a) ((a).n<<LogWordSize)		/* Maximum items per set */
#define	MODWORD(x) ((x) & (WORDSIZE_IN_BITS-1))		/* x % WORDSIZE */
#define	DIVWORD(x) ((x) >> LogWordSize)		/* x / WORDSIZE */
#define	nil	(~((unsigned) 0))	/* An impossible set member all bits on (big!) */

typedef struct _set {
	unsigned int n;		/* Number of words in set */
	unsigned *setword;
} set;

#define set_init	{0, NULL}
#define set_null(a)	((a).setword==NULL)

#define	NumBytes(x)		(((x)>>3)+1)						/* Num bytes to hold x */
#define	NumWords(x)		((((unsigned)(x))>>LogWordSize)+1)	/* Num words to hold x */


/* M a c r o s */

/* make arg1 a set big enough to hold max elem # of arg2 */
#define set_new(a,_max) \
if (((a).setword=(unsigned *)calloc(NumWords(_max),BytesPerWord))==NULL) \
        fprintf(stderr, "set_new: Cannot allocate set with max of %d\n", _max); \
        (a).n = NumWords(_max);

#define set_free(a)									\
	{if ( (a).setword != NULL ) free((char *)((a).setword));	\
	(a) = empty;}

extern void set_size( unsigned );
extern unsigned int set_deg( set );
extern set set_or( set, set );
extern set set_and( set, set );
extern set set_dif( set, set );
extern set set_of( unsigned );
extern void set_ext( set *, unsigned int );
extern set set_not( set );
extern bool set_equ( set, set );
extern int set_sub( set, set );
extern unsigned set_int( set );
extern int set_el( unsigned, set );
extern int set_nil( set );
extern char * set_str( set );
extern set set_val( register char * );
extern void set_orel( unsigned, set * );
extern void set_orin( set *, set );
extern void set_rm( unsigned, set );
extern void set_clr( set );
extern set set_dup( set );
extern void set_PDQ( set, register unsigned * );
extern unsigned *set_pdq( set );
extern void _set_pdq( set a, register unsigned *q );
extern unsigned int set_hash( set, register unsigned int );
extern unsigned int set_find_region( set a, unsigned int n );
extern void set_print(set a);

extern set empty;
