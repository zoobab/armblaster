/*
 *	�V���{���������ʂ�����\����
 */

typedef struct {
	long   adrs;		/* �_���A�h���X  */
	char  *name;		/* �V���{����	 */
} SYMBOL;

#define	Ulong	unsigned long
int	find_symbol_by_name(SYMBOL *symbol,char *name  );
int	find_symbol_by_adrs(SYMBOL *symbol,long address);

#define	SYMLEN	48		/* �V���{�����̒����̍ő咷 */
