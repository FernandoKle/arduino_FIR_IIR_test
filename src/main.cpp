/*! Test de tiempos y presision de punto fijo, flotante y flotante hecho a mano
Por: Fernando Kleinubing
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // uint8_t int8_t entre otros tipos
#include <avr/io.h>
#include <util/delay.h> // _delay_ms(tiempo_en_ms)
#include <avr/interrupt.h>

// https://github.com/Pharap/FixedPointsArduino
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

// TIPO DE FILTRO A USAR
//#define FIR // comentar para usar IIR

#ifndef FIR
	#define IIR
#endif

// Tipo de dato a usar: Q-15
#ifdef FIR
typedef SFixed<1, 14> fixed_type;

#else
// los IIR necesitan mas precision y parte entera
typedef SFixed<5, 26> fixed_type;
#endif

// Numero de coeficientes FIR
#define N 9 
// Numero de coeficientes IIR
#define N_IIR 5
// buffer de recepcion de datos DEBE SER POTENCIA DE 2
#define BUFFER_SIZE 64

#ifdef FIR
// Filtro FIR
fixed_type w[N] = 
{
0.014407925284062201,
0.0438627666677571,
0.12021193169441212,
0.2025343520793387,
0.2379660485488598,
0.2025343520793387,
0.12021193169441212,
0.0438627666677571,
0.014407925284062201
};

#else
// Filtro IIR
// Polos - A
fixed_type a[N_IIR] = 
{
1.0,
-3.180638548874719,
3.8611943489942133,
-2.112155355110969,
0.43826514226197977
};

// Ceros - B
fixed_type b[N_IIR] = 
{
0.00041659920440659937,
0.0016663968176263975,
0.002499595226439596,
0.0016663968176263975,
0.00041659920440659937
};
#endif

fixed_type x[BUFFER_SIZE] = {0};
uint8_t x_index = 0 ;

#ifdef IIR
fixed_type y[BUFFER_SIZE] = {0};
uint8_t y_index = 0 ;
#endif

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

#ifdef FIR
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
		// Si BUFFER_SIZE NO es potencia de 2
		// Usar: x[ (x_index + N - 1 - i) % BUFFER_SIZE] ;
		// Si BUFFER_SIZE es potencia de 2
		// Usar: x[ (x_index + N - 1 - i) & (BUFFER_SIZE - 1) ] ;

		y += w[i] * x[ (x_index + N - 1 - i) & (BUFFER_SIZE - 1) ] ;
	}

	return y ;
}

#else
fixed_type
filtro_IIR ()
{
	fixed_type aux = 0 ;

	#pragma unroll
	for (uint8_t i = 0; i<N_IIR; i++)
	{
		aux += b[i] * x[ (x_index + N - 1 - i) & (BUFFER_SIZE - 1) ]
			 - a[i] * y[ (y_index + N - 1 - i) & (BUFFER_SIZE - 1) ];
	}
	
	y[y_index] = aux ;
	y_index = (y_index + 1) % BUFFER_SIZE;

	return aux ;
}
#endif
// Punto fijo 0~1 a entero 0~255
uint8_t inline
escalar (fixed_type val)
{
	#ifdef FIR
	// Para SFixed<1, 14> 
	uint16_t internal = val.getInternal();
	uint8_t salida = internal >> 7 ;

	#else
	// Para SFixed<5, 26>
	uint32_t internal = val.getInternal();
	uint8_t salida = internal >> 19 ;
	#endif

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

	// Desplaza el valor de 8 bits 0~255 
	// de tal forma que 255 (max) se mapee a 1.0 (max)

	#ifdef FIR
	// Para SFixed<1, 14> 
	// 0000000011111111 << 7 => 01.11111110000000
	uint16_t dato = UDR0 ; 
    x[x_index] = fixed_type::fromInternal(dato << 7);

	#else
	// Para SFixed<5, 26>
	// 00000000000000000000000011111111 << 19 => 000001.11111110000000000000000000
	uint32_t dato = UDR0 ; 
    x[x_index] = fixed_type::fromInternal(dato << 19);
	#endif

	// Actualizar el índice y asegurar que no exceda el tamaño del buffer
    x_index = (x_index + 1) % BUFFER_SIZE; 

	// Procesar y enviar
	#ifdef FIR
	fixed_type pre_salida = filtro_FIR();
	#else
	fixed_type pre_salida = filtro_IIR();
	#endif

	uint8_t salida = escalar( pre_salida );

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
