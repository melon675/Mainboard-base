#ifndef _MODEM_H
#define _MODEM_H

#include "radio.h"
#include "hw.h"
#include "timeServer.h"
#include "delay.h"
#include "vcom.h"

#define RF_FREQUENCY                                434000000 // Hz
#define TX_OUTPUT_POWER                             20        // dBm
#define LORA_BANDWIDTH                              2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       8         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false	  //Tutaj daj true,
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
#define RX_UART_TIMEOUT_VALUE						300
#define BUFFER_SIZE                                 64 		  // Define the payload size here
#define BUFFER_SOCKETS								48


void Modem_Init( void );
uint8_t Modem_Send( void );

void OnTxDone( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void OnTxTimeout( void );
void OnRxTimeout( void );
void OnRxError( void );

void PutFifo( uint8_t* data );
void PushFifo( uint8_t* data );
uint8_t IsFifoFull( void );
uint8_t IsFifoEmpty( void );


#endif //_MODEM_H
