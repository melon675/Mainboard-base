#include "modem.h"

extern UART_HandleTypeDef huart2;
static RadioEvents_t RadioEvents;
TimerEvent_t timer_DMA_RX;
uint8_t radio_busy = false;


uint8_t UartRxBuffer[128] = {0};
uint8_t RadioTxBuffer[64] = {0};

uint8_t FIFO_Buffer[BUFFER_SOCKETS][BUFFER_SIZE]={0};
uint8_t FIFO_Socket_Count[BUFFER_SOCKETS] = {0};
uint8_t FIFO_IN = 0, FIFO_OUT = 0, FIFO_COUNT = 0;

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == huart2.Instance)
	{
		PutFifo(&UartRxBuffer[0]);	//Dodaj element do bufora
		//if(timer_DMA_RX.IsRunning == true)
		//	TimerReset(&timer_DMA_RX);		//Reset timera, jesli to kolejna ramka pakietu
		//else
		//	TimerStart(&timer_DMA_RX);		//Uruchom timer, jeœli to pierwsza ramka pakietu
	}
}

/* Odebranie kolejnej czeœci ramki RTCM */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == huart2.Instance)
	{
		PutFifo(&UartRxBuffer[64]);	//Dodaj element do bufora
		//if(timer_DMA_RX.IsRunning == true)
		//	TimerReset(&timer_DMA_RX);		//Reset timera, jesli to kolejna ramka pakietu
		//else
		//	TimerStart(&timer_DMA_RX);		//Uruchom timer, jeœli to pierwsza ramka pakietu
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	PutFifo(&UartRxBuffer[0]);
}

void HAL_SYSTICK_Callback(void)
{
	static uint8_t tick = 1;
	static uint32_t data_counter_act = 128, data_counter_prev = 128;
	data_counter_act = huart2.hdmarx->Instance->CNDTR;
	if(data_counter_act == 128)
	{
		tick = 1;
		data_counter_prev = 128;
		return;
	}
	if(data_counter_act != data_counter_prev)
	{
		tick = 1;
		data_counter_prev = data_counter_act;
		return;
	}
	if(tick != 0)
	{
		tick = (tick + 1) % 4;
		return;
	}
	HAL_UART_DMAStop(&huart2);
	if(data_counter_act > 64)
	{

		memset(&UartRxBuffer[128 - data_counter_act], 0, (data_counter_act - 64));
		PutFifo(&UartRxBuffer[0]);	//Dodaj element do bufora
	}
	else
	{
		memset(&UartRxBuffer[128 - data_counter_act], 0, (data_counter_act));
		PutFifo(&UartRxBuffer[64]);	//Dodaj element do bufora
	}
	data_counter_prev = data_counter_act;
	HAL_UART_Receive_DMA(&huart2, UartRxBuffer, 128);
	tick = 1;
}
/* Przerwanie timeout, koniec ramki RTCM */
void UART_DMA_Timeout( void )
{
	HAL_UART_DMAStop(&huart2);
	//PutFifo();	//Dodaj element do bufora
	HAL_UART_Receive_DMA(&huart2, UartRxBuffer, 128);
}

/* Inicjalizacja transmisji oraz odpowiednich Callbacków */
void Modem_Init( void )
{
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;

	Radio.Init( &RadioEvents );
	Radio.SetChannel( RF_FREQUENCY );
	Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
	                               LORA_SPREADING_FACTOR, LORA_CODINGRATE,
	                               LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
	                               true, 1, 255, LORA_IQ_INVERSION_ON, 3000000 );
	Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
	                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
	                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
	                               0, true, 1, 10, LORA_IQ_INVERSION_ON, true );

	//TimerInit(&timer_DMA_RX, UART_DMA_Timeout);
	//TimerSetValue( &timer_DMA_RX, RX_UART_TIMEOUT_VALUE);
	HAL_UART_Receive_DMA(&huart2, UartRxBuffer, 128);
}

/* Funkcja wysylaj¹ca dane oczekuj¹ce w buforze */
uint8_t Modem_Send( void )
{
	if((IsFifoEmpty() == 0) && (radio_busy == false))
	{
		PushFifo(RadioTxBuffer);	//Przeniesione do Callbacka TxDone
		Radio.Send( RadioTxBuffer, 64 );
		radio_busy = true;
		return true;
	}
	else
		return false;
}


void OnTxDone( void )
{
	Radio.Standby( );
	radio_busy = false;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    Radio.Standby();
    //BufferSize = size;
    //memcpy( Buffer, payload, BufferSize );
}

void OnTxTimeout( void )
{
	Radio.Standby( );
	radio_busy = false;
}

void OnRxTimeout( void )
{
	Radio.Standby( );
	radio_busy = false;
}

void OnRxError( void )
{
	Radio.Standby( );
}


uint8_t IsFifoFull( void )
{
	if(FIFO_OUT == ((FIFO_IN + 1) % BUFFER_SOCKETS))
		return true;
	else
		return false;
}

uint8_t IsFifoEmpty( void )
{
	if(FIFO_IN == FIFO_OUT)
		return true;
	else
		return false;
}
void PushFifo( uint8_t* data )
{
	if(FIFO_IN != FIFO_OUT) {
		memcpy(data, FIFO_Buffer[FIFO_OUT], 64);
		FIFO_OUT = (FIFO_OUT + 1) % BUFFER_SOCKETS;
		FIFO_COUNT--;
	}
}

void PutFifo( uint8_t* data )
{
	if(FIFO_OUT != ((FIFO_IN + 1) % BUFFER_SOCKETS)) {
		memcpy(FIFO_Buffer[FIFO_IN], data, 64);
		FIFO_IN = (FIFO_IN + 1) % BUFFER_SOCKETS;
		FIFO_COUNT++;
	}
}



