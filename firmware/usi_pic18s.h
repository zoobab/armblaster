/*------------------------------------------------------------------------
 *		PIC18F用 usi_trans()関数.	SOFTWARE実装.
 *------------------------------------------------------------------------
関数一覧:

 void  portInit(void)			起動時Port初期化;
 void  ispConnect()             SPI接続;
 void  ispDisconnect()          SPI開放;
 uchar usi_trans(char cData)	SPI転送1バイト実行;
 
 *------------------------------------------------------------------------
 */

#if	0
//	ポート出力データ.    		  	JTAG  PIC   AVR
#define	PGC	 		PB11			// TCK  PGC =(AVR-SCK)
#define	PGM	 		PB10			// TDI  PGM =(AVR-MOSI)
#define	PGD	 		PB9				// TDO  PGD =(AVR-MISO)
#define	MCLR 		PB8				// TMS  MCLR=(AVR-RST)
#endif

//	ポート出力データ.    
#define	ISP_SCK		PGC			/* Target SCK */
#define	ISP_MISO	PGD			/* Target MISO */
#define	ISP_MOSI	PGM			/* Target MOSI */
#define	ISP_RST		MCLR		/* Target RESET */

#define	PORT_SCK	PGC			/* Target SCK */
#define	PORT_MISO	PGD			/* Target MISO */
#define	PORT_MOSI	PGM			/* Target MOSI */
#define	PORT_RST	MCLR		/* Target RESET */

static uchar usi_delay = 5;
//static uchar dummy_1 = 0;
//------------------------------------------------------------------------
//
//	wait:(tiny2313)
//		0 =  3clk    2 MHz
//		1 =  4clk  1.5 MHz
//		2 =  9clk  666 kHz
//		3 = 21clk  285 kHz
//		4 = 33clk  181 kHz
//	   10 =129clk   46 kHz
//	   20 =249clk   23 kHz
//	   50 =609clk  9.8 kHz
//
//		2以上は 9 + (12 * wait) clk （CALL-RETを採用した場合も同様）
//
void ispDelay(uchar d)
{
#if	0
	uchar i;
	for(i=2;i<d;i++) ;
#endif
	wait_u8s(d*8);
}
//------------------------------------------------------------------------
void SPI_MasterInit(void)
{
}

//------------------------------------------------------------------------
void SPI_MasterExit(void)
{
}

//------------------------------------------------------------------------
void wait_nop(void)
{
}

//------------------------------------------------------------------------
//uchar SPI_MasterTransmit(char cData)
//	SPI転送を１バイト分実行する.
//
//  SCK    _____ __~~ __~~ __~~ __~~ __~~ __~~ __~~ __~~  _____
//  MOSI   _____ < D7>< D6>< D5>< D4>< D3>< D2>< D1>< D0>
#if	0

#define	MOSI_OUT(dat,bit,rd) 						\
	digitalWrite(ISP_MOSI,data & (1<<bit));			\
	digitalWrite(ISP_SCK,1);						\
	if(digitalRead(PORT_MISO)) { rd |= (1<<bit); }	\
	digitalWrite(ISP_SCK,0);						\


//
//	1nop分のwait入りバリエーション.
//
#define	MOSI_OUT1(dat,bit,rd) \
	digitalWrite(ISP_MOSI,data & (1<<bit));			\
	digitalWrite(ISP_SCK,1);						\
	wait_nop();										\
	if(digitalRead(PORT_MISO)) { rd |= (1<<bit); }	\
	digitalWrite(ISP_SCK,0);						\
	wait_nop();										\


#endif


#if	0
//
//	最高速
//
uchar usi_trans0(uchar data)
{
	uchar r=0;
	MOSI_OUT(data,7,r);
	MOSI_OUT(data,6,r);
	MOSI_OUT(data,5,r);
	MOSI_OUT(data,4,r);
	MOSI_OUT(data,3,r);
	MOSI_OUT(data,2,r);
	MOSI_OUT(data,1,r);
	MOSI_OUT(data,0,r);
	return r;
}

//
//	1nop入り.
//
uchar usi_trans1(uchar data)
{
	uchar r=0;
	MOSI_OUT1(data,7,r);
	MOSI_OUT1(data,6,r);
	MOSI_OUT1(data,5,r);
	MOSI_OUT1(data,4,r);
	MOSI_OUT1(data,3,r);
	MOSI_OUT1(data,2,r);
	MOSI_OUT1(data,1,r);
	MOSI_OUT1(data,0,r);
	return r;
}
#endif

uchar usi_trans(uchar data)
{
	uchar i;
	uchar r=0;

#if	0
	if(usi_delay==0) return usi_trans0(data);
	if(usi_delay==1) return usi_trans1(data);
#endif
//	ISP_SCK = 0;
	digitalWrite(ISP_SCK,0);

	for(i=0;i<8;i++) {

//		if(  data & 0x80)  ISP_MOSI = 1;
//		if(!(data & 0x80)) ISP_MOSI = 0;
		digitalWrite(ISP_MOSI,data & 0x80);

		r += r;			// r<<=1;

		ispDelay(usi_delay);
		
		//if(PORT_MISO) { r|=1;}
		if(digitalRead(PORT_MISO)) { r|=1; }

		//ISP_SCK = 1;
		digitalWrite(ISP_SCK,1);

		data += data;	// data <<=1;
		ispDelay(usi_delay);

		//ISP_SCK = 0;
		digitalWrite(ISP_SCK,0);
	}
	return r;
}


//------------------------------------------------------------------------
void ispConnect(void) {
  /* reset device */
//	ISP_RST=0;	/* RST low */
//	ISP_SCK=0;  /* SCK low */
	digitalWrite(ISP_RST,0);
	digitalWrite(ISP_SCK,0);

  /* now set output pins */
//	DDR_RST=0;
//	DDR_SCK=0;
//	DDR_MOSI=0;
//	DDR_MISO=1;
	pinMode(ISP_RST ,OUTPUT);
	pinMode(ISP_SCK ,OUTPUT);
	pinMode(ISP_MOSI,OUTPUT);
	pinMode(ISP_MISO,INPUT);

  /* positive reset pulse > 2 SCK (target) */
  	ispDelay(100);	// ISP_OUT |= (1 << ISP_RST);    /* RST high */
//	ISP_RST = 1;
	digitalWrite(ISP_RST,1);

  	ispDelay(100);	// ISP_OUT &= ~(1 << ISP_RST);   /* RST low */

//	ISP_RST = 0;
	digitalWrite(ISP_RST,0);
}

//------------------------------------------------------------------------
static void ispDisconnect(void) {
  
  /* set all ISP pins inputs */
//	DDR_RST=1;
//	DDR_SCK=1;
//	DDR_MOSI=1;
	pinMode(ISP_RST ,INPUT);
	pinMode(ISP_SCK ,INPUT);
	pinMode(ISP_MOSI,INPUT);

  /* switch pullups off */
//	ISP_RST=0;
//	ISP_SCK=0;
//	ISP_MOSI=0;
	digitalWrite(ISP_RST,0);
	digitalWrite(ISP_SCK,0);
	digitalWrite(ISP_MOSI,0);
}

//------------------------------------------------------------------------
static	void ispSckPulse(void)
{
/* SCK high */
//	ISP_SCK = 1;
 	digitalWrite(ISP_SCK,1);
	ispDelay(100);
/* SCK Low */
//	ISP_SCK = 0;
	digitalWrite(ISP_SCK,0);
    ispDelay(100);
}

void usi_set_delay(uchar delay)
{
	usi_delay = delay;	// '-dN'
}
