
	/* portlist.h */

#ifndef	_portlist_h_
#define	_portlist_h_

#define	_SFR_IO8( p )   (0x20+(p)),1
#define	_SFR_IO16( p )  (0x20+(p)),2
#define	_PIC_IO8( p )   (p),1
#define	_PIC_IO8E( p )  (p),(1|EX_PORT)
#define	_PIC_IO8a( p )  (p),4				// Alias

#define	QQ		0x10	//�ǂ��Q�Ƃ���|�[�g�Ɉ��t����.
#define	EX_PORT 0x20	//18F4550�Ŋg������Ă���|�[�g.



#define	A_uint8_t	0x0001
#define	A_uint16_t	0x0002
#define	A_uint32_t	0x0004
#define	A_MASK		0x0007

#define	A_char		0x0001
#define	A_short		0x0002
#define	A_long		0x0004

#define	A__I		0x0100
#define	A__O		0x0200
#define	A__IO		0x0300
#define	A_CAN_TxMailBox_TypeDef		4
#define	A_CAN_FIFOMailBox_TypeDef		4
#define	A_CAN_FilterRegister_TypeDef		4

typedef	struct {
	char *name;
	int   adrs;
	int	  attr;	// bit0:8bit , bit1:16bit ,bit4:quickviewflag
} PortList;


/* portlist.c */
int str_casecmpx (char *s, char *t, int *pmask);

//	�|�[�g����(�����������e����)��Ԓn�ɕϊ�����.
int portAddress (char *s);

//	�|�[�g����(�����������e����)��Ԓn�ɕϊ�����.(bit�w������e����)
int portAddress_b (char *s, int *pmask);

//	�Ԓn���|�[�g����(�����������e����)�ɕϊ�����.
char *GetPortName(int adrs);

void PrintPortNameList (void);
char *radix2str (char *buf, int d);
void PrintPortColumn (void);
void PrintPortAll (int mask);

int	InitPortList(void);

//	�|�[�g����(�����������e����)�𐳂������̂ɕϊ�.
char *realPortName(char *s);
//	�|�[�g����(�����������e����)����|�[�g�����l(�A�N�Z�X���Ȃ�)�𓾂�.
int getPortAttrib(char *s);

//	�|�[�g����(�������╔������������e����)�𐳂������̂ɕϊ�.
char *suggestPortName(char *s);

//	�|�[�g�̌^���(A_uint8_t�`A_uint32_t�܂ł�enum)���A�A�N�Z�X��(�o�C�g�T�C�Y)�ɕϊ�.
int	type2size(int type);

#endif	//_portlist_h_

