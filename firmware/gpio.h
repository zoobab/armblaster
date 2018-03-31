#ifndef	gpio_h_
#define	gpio_h_

//	LOW = 0 Volt HIGH=3.3Volt
enum {
	LOW  = 0,
	HIGH = 1,
};

//	PORT DIRECTION
enum {
	INPUT = 0,
	OUTPUT= 1,
};

//	PORT PIN ASSIGN
enum {
	PA0	= 0	,
	PA1		,
	PA2		,
	PA3		,
	PA4		,
	PA5		,
	PA6		,
	PA7		,

	PA8		,
	PA9		,
	PA10	,
	PA11	,
	PA12	,
	PA13	,
	PA14	,
	PA15	,
};

enum {
	PB0	=16	,
	PB1		,
	PB2		,
	PB3		,
	PB4		,
	PB5		,
	PB6		,
	PB7		,

	PB8		,
	PB9		,
	PB10	,
	PB11	,
	PB12	,
	PB13	,
	PB14	,
	PB15	,
};

enum {
	PC0	=32	,
	PC1		,
	PC2		,
	PC3		,
	PC4		,
	PC5		,
	PC6		,
	PC7		,

	PC8		,
	PC9		,
	PC10	,
	PC11	,
	PC12	,
	PC13	,
	PC14	,
	PC15	,
};

//	SET PORT DIRECTION
void	pinMode(int	pin, int mode);

//	OUTPUT PORT
void	digitalWrite(int pin, int value);

//	INPUT  PORT
int		digitalRead(int pin);

#endif
