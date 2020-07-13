/*
 * extfram.h
 *
 *  Created on: 2020年7月8日
 *      Author: akai
 *  Range : 0x00000 - 0xFFFFF (20bit) //8Mb
 *          0x00000 - 0x7FFFF // 4Mb
 *  Clock : up to 20 MHz (8Mb)
 *          up to 40 MHz (4Mb)
 */
//                   MSP430FR5994
//                 -----------------
//                |             P1.0|-> Comms LED
//                |                 |
//                |                 |
//                |                 |
//                |             P6.0|-> Data Out (UCA3SIMO)
//                |                 |
//                |             P6.1|<- Data In (UCSOA3MI)
//                |                 |
//                |             P6.2|-> Serial Clock Out (UCA3	CLK)
//                |                 |
//                |             P5.3|-> Slave Chip Select (GPIO)

#ifndef JAPARILIB_EXTFRAM_H_
#define JAPARILIB_EXTFRAM_H_

#define FRAM_8MB
#define EXTFRAM_USE_DMA
#define UCA3

#ifdef UCA3

#define DMA3TSEL__SPITXIFG DMA3TSEL__UCA3TXIFG
#define DMA4TSEL__SPIRXIFG DMA4TSEL__UCA3RXIFG
#define SPITXBUF UCA3TXBUF
#define SPIRXBUF UCA3RXBUF
#define SPISTATW UCA3STATW
#define SPICTLW0 UCA3CTLW0
#define SPIIFG UCA3IFG
#define SPIBRW UCA3BRW
#define SPISEL0 P6SEL0
#define SPISEL1 P6SEL1
#endif

#ifdef UCB1
//Does not support DMA read,
#define DMA3TSEL__SPITXIFG DMA3TSEL__UCB1TXIFG
#define SPITXBUF UCB1TXBUF
#define SPIRXBUF UCB1RXBUF
#define SPISTATW UCB1STATW
#define SPICTLW0 UCB1CTLW0
#define SPIIFG UCB1IFG
#define SPIBRW UCB1BRW
#define SPISEL0 P5SEL0
#define SPISEL1 P5SEL1
#endif


#include <stdint.h>

#define SLAVE_CS_OUT    P5OUT
#define SLAVE_CS_DIR    P5DIR
#define SLAVE_CS_PIN    BIT3


#define COMMS_LED_OUT   P1OUT
#define COMMS_LED_DIR   P1DIR
#define COMMS_LED_PIN   BIT0

typedef union Data {
    unsigned char byte[4];
    unsigned long L;
}SPI_ADDR;

//WREN Set write enable latch 0000 0110b
#define CMD_WREN 0x06
//WRDI Reset write enable latch 0000 0100b
#define CMD_WRDI 0x04
//RDSR Read Status Register 0000 0101b
#define CMD_RDSR 0x05
//WRSR Write Status Register 0000 0001b
#define CMD_WRSR 0x01
//READ Read memory data 0000 0011b
#define CMD_READ 0x03
//FSTRD Fast read memory data 0000 1011b
#define CMD_FSTRD 0x0b
//WRITE Write memory data 0000 0010b
#define CMD_WRITE 0x02
//SLEEP Enter sleep mode 1011 1001b
#define CMD_SLEEP 0xb9
//RDID Read device ID 1001 1111b
#define CMD_RDID 0x9f
//SSWR Special Sector Write 42h 0100 0010b
#define CMD_SSWR 0x42
//SSRD Special Sector Read 4Bh 0100 1011b
#define CMD_SSRD 0x4b
//RUID Read Unique ID 4Ch 0100 1100b
#define CMD_RUID 0x4c
//DPD Enter Deep Power-Down BAh 1011 1010b
#define CMD_DPD 0xba
//HBN Enter Hibernate mode B9h 1011 1001b
#define CMD_HBN 0xb9

#define DUMMY   0xFF

void eraseFRAM(){
	uint8_t val;
#ifdef FRAM_8MB
	unsigned long cnt = 0xfffff;
#else
	unsigned long cnt = 0x7ffff;
#endif

	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_WREN;
		while(SPISTATW & 0x1);
	SLAVE_CS_OUT |= SLAVE_CS_PIN;

    SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
    	SPITXBUF = CMD_WRITE;
    	while(SPISTATW & 0x1);
    	SPITXBUF=0x00;
    	while(SPISTATW & 0x1);
    	SPITXBUF=0x00;
    	while(SPISTATW & 0x1);
    	SPITXBUF=0x00;
		while(cnt--){
			while(SPISTATW & 0x1);
			SPITXBUF = 0xff;
		}
		COMMS_LED_OUT |=0x1;
		val = SPIRXBUF; //Clean the overrun flag
	SLAVE_CS_OUT |= SLAVE_CS_PIN;
}

void initSPI()
{

    SPISEL0 |= 0x07;
    SPISEL1 &= 0xF8;
    SLAVE_CS_DIR |= SLAVE_CS_PIN;
    SLAVE_CS_OUT |= SLAVE_CS_PIN;

    //Clock Polarity: The inactive state is high
    //MSB First, 8-bit, Master, 3-pin mode, Synchronous
    SPICTLW0 = UCSWRST;                       // **Put state machine in reset**
    SPICTLW0 |= UCCKPL | UCMSB | UCSYNC
                       | UCMST | UCSSEL__SMCLK;      // 3-pin, 8-bit SPI Slave
    SPIBRW = 0; //FRAM SPEED control

    SPICTLW0 &= ~UCSWRST;                     // **Initialize USCI state machine**

    //All writes to the memory begin with a WREN opcode with CS being asserted and deasserted.
	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_WREN;
		while(SPISTATW & 0x1);
	SLAVE_CS_OUT |= SLAVE_CS_PIN;

	//Disable memory write protection (Just in case)
	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_WRSR;
		while(SPISTATW & 0x1);
		SPITXBUF = 0xC0;
		while(SPISTATW & 0x1);
	SLAVE_CS_OUT |= SLAVE_CS_PIN;



}
void SPI_READ(SPI_ADDR* A,uint8_t *dst, unsigned long len ){

	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_READ;
		while(SPISTATW & 0x1); //SPI BUSY
		SPITXBUF=A->byte[2];
		while(SPISTATW & 0x1);
		SPITXBUF=A->byte[1];
		while(SPISTATW & 0x1);
		SPITXBUF=A->byte[0];
		while(SPISTATW & 0x1);
#if defined EXTFRAM_USE_DMA && defined UCA3
		DMACTL1 |= DMA3TSEL__SPITXIFG;
		// Write dummy data to TX
		DMA3CTL = DMADT_0 + DMADSTINCR_0 + DMASRCINCR_0 +  DMADSTBYTE__BYTE  + DMASRCBYTE__BYTE + DMALEVEL__EDGE;
		DMA3SA = &SPIRXBUF;
		DMA3DA = &SPITXBUF;
		DMA3SZ = len;
		DMA3CTL |= DMAEN__ENABLE;
		// Read RXBUF -> dst
		DMACTL2 |= DMA4TSEL__SPIRXIFG;
		DMA4CTL = DMADT_0 + DMADSTINCR_3 + DMASRCINCR_0 +  DMADSTBYTE__BYTE  + DMASRCBYTE__BYTE + DMALEVEL__EDGE;
		DMA4SA = &SPIRXBUF;
		DMA4DA = dst;
		DMA4SZ = len;
		DMA4CTL |= DMAEN__ENABLE;
		//Trigger TX DMA
		SPIIFG &= ~UCTXIFG;
		SPIIFG |=  UCTXIFG;
		//Trigger RX DMA
		SPIIFG &= ~UCRXIFG;
		SPIIFG |=  UCRXIFG;
		while(DMA4CTL & DMAEN__ENABLE);

#else
		while(len--){
			SPITXBUF=0x00;
			while(SPISTATW & 0x1);  //SPI BUSY
			*dst++ = SPIRXBUF;
		}
#endif

	SLAVE_CS_OUT |= SLAVE_CS_PIN;
}

void SPI_WRITE(SPI_ADDR* A,uint8_t *src, unsigned long len ){
	//All writes to the memory begin with a WREN opcode with CS being asserted and deasserted.
	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_WREN;
		while(SPISTATW & 0x1); //SPI BUSY
	SLAVE_CS_OUT |= SLAVE_CS_PIN;

	SLAVE_CS_OUT &= ~(SLAVE_CS_PIN);
		SPITXBUF = CMD_WRITE;
		while(SPISTATW & 0x1);
		SPITXBUF=A->byte[2];
		while(SPISTATW & 0x1);
		SPITXBUF=A->byte[1];
		while(SPISTATW & 0x1);
		SPITXBUF=A->byte[0];
		while(SPISTATW & 0x1);

#ifdef EXTFRAM_USE_DMA
		//Triggered when TX is done
		DMACTL1 |= DMA3TSEL__SPITXIFG;
		DMA3CTL = DMADT_0 + DMADSTINCR_0 + DMASRCINCR_3 +  DMADSTBYTE__BYTE  + DMASRCBYTE__BYTE + DMALEVEL__EDGE;
		DMA3SA = src;
		DMA3DA = &SPITXBUF;
		DMA3SZ = len;
		DMA3CTL |= DMAEN__ENABLE;
		//Triger the DMA to invoke the first transfer
		SPIIFG &= ~UCTXIFG;
		SPIIFG |=  UCTXIFG;
		while(DMA3CTL & DMAEN__ENABLE);
#endif
#ifndef EXTFRAM_USE_DMA
		while(len--){
			SPITXBUF=*src++;
			while(SPISTATW & 0x1);
		}
#endif
		//clean overrun flag
		uint8_t val=SPIRXBUF;

	SLAVE_CS_OUT |= SLAVE_CS_PIN;
}
//Sample code
//initSPI();
//uint8_t ggg[16];
//SPI_ADDR A;
//A.L = 0;
//SPI_READ( &A, ggg, 16 );
//ggg[0]=4;ggg[1]=3;
//SPI_WRITE( &A, ggg, 16 );
//SPI_READ( &A, ggg, 16 );
#endif /* JAPARILIB_EXTFRAM_H_ */
