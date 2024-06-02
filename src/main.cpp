/*! Test de tiempos y presision de punto fijo, flotante y flotante hecho a mano
Por: Fernando Kleinubing
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // uint8_t int8_t entre otros tipos
#include <avr/io.h>
#include <util/delay.h> // _delay_ms(tiempo_en_ms)
#include <avr/interrupt.h>

#include <FixedPoints.h>
#include <FixedPointsCommon.h>

#define F_CPU           16000000
#define USART_BAUDRATE  9600
#define UBRR_VALUE      (((F_CPU/(USART_BAUDRATE*16UL)))-1)

#define sbi(p,b) ( p |= _BV(b) )
#define cbi(p,b) ( p &= ~(_BV(b)) )
#define tbi(p,b) ( p ^= _BV(b) )

#define is_high(p,b) ( (p & _BV(b)) == _BV(b) )
#define is_low(p,b) ( (p & _BV(b)) == 0 )

// Tipo de dato a usar: Q-15
typedef SFixed<1, 14> fixed_type;

// coeficientes
#define N 33 
// buffer de recepcion de datos DEBE SER POTENCIA DE 2
#define BUFFER_SIZE 128

// Globals
fixed_type w[N] = 
{
0.0015115451098377376,
0.0018825913790288383,
0.002483585545241963,
0.0031160595343624215,
0.0033433106148237557,
0.0025399291880564756,
-1.558506734552068e-17,
-0.004914128527513414,
-0.012611514421013636,
-0.023134852490067574,
-0.03607727546279237,
-0.05057652387209304,
-0.0653946653020385,
-0.0790744643582674,
-0.09014753340161663,
-0.09735795708930048,
0.8987464021635272,
-0.09735795708930048,
-0.09014753340161663,
-0.0790744643582674,
-0.0653946653020385,
-0.05057652387209304,
-0.03607727546279239,
-0.023134852490067574,
-0.012611514421013636,
-0.004914128527513413,
-1.558506734552067e-17,
0.0025399291880564756,
0.0033433106148237557,
0.0031160595343624215,
0.0024835855452419653,
0.0018825913790288383,
0.0015115451098377376
};

fixed_type x[BUFFER_SIZE] = {0};
uint16_t rx_index = 0 ;

void 
serial_init ()
{
	// Enable TX RX and RX interrupt
	// no parity, 1 stop bit, 8-bit data
	UBRR0 = UBRR_VALUE;				
	UCSR0B |= (1<<TXEN0);			
	UCSR0B |= (1<<RXEN0);			
	UCSR0B |= (1<<RXCIE0);			
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ01); 	
}

void inline 
serial_send (uint8_t data)
{
	while( !(UCSR0A & (1<<UDRE0)) );
	UDR0 = data;
}

fixed_type
filtro_FIR ()
{
	fixed_type y = 0 ;

	// utilizar loop unroll si N < 512
	#if N < 512 
		#pragma unroll
	#endif
	for (uint16_t i = 0; i<N; i++)
	{
		// Si el BUFFER_SIZE NO es potencia de 2
		//y += w[i] * x[ (rx_index + N - 1 - i) % BUFFER_SIZE] ;
		// Si el BUFFER_SIZE es potencia de 2
		y += w[i] * x[ (rx_index + N - 1 - i) & (BUFFER_SIZE - 1) ] ;
	}

	return y ;
}

// Punto fijo 0~1 a entero 0~255
uint8_t inline
escalar (fixed_type val)
{
	//float escalado = val * 255.0f;
	//uint8_t salida = static_cast<uint8_t>(escalado);
	uint16_t internal = val.getInternal();
	uint8_t salida = internal >> 7 ;

	if (salida < 0)
    	return 0;
	if (salida > 255)
    	return 255;

	return salida;
}

// @=================== Interrupciones =================@ \\

ISR (USART_RX_vect) 
{
	// Recibir, procesar y almacenar
	uint16_t dato = UDR0 ; // esperando valores en rango 0~255

	// Desplaza el valor de 8 bits 0~255 
	// de tal forma que 255 (max) se mapee a 1.0 (max)
	// Para SFixed<1, 14> 
	// 0000000011111111 << 7 => 01.11111110000000
    x[rx_index] = fixed_type::fromInternal(dato << 7);

	// Actualizar el índice y asegurar que no exceda el tamaño del buffer
    rx_index = (rx_index + 1) % BUFFER_SIZE; 

	// Procesar y enviar
	uint8_t salida = escalar( filtro_FIR() );

	serial_send( salida );
}

// @======================= MAIN =======================@ \\
int 
main ()
{
	// Setup
	serial_init();
	sei();
	
	// Loop
	while(1)
	{
	}
}
