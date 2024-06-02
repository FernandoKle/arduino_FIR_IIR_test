ISR(USART_RX_vect)
{
	// Empty buffer
	if(waiting == 0)
	{
		for(uint16_t i = 0; i<RX_LINE_SIZE; i++)
			rx_line[i] = (uint8_t) 0 ;
	}

	// Block
	waiting = 1;

	// Receive data
	uint8_t input = UDR0;
	if(input != '\n')
		rx_line[rx_line_pos++] = input;

	// Handle interrupt
	if( (rx_line_pos >= RX_LINE_SIZE)
		|| (input == '\n' && rx_line_pos > 0) ) 
	{
		// Handle result
		//logic_handler();

		// Resize
		rx_line_pos = 0;

		// Unblock
		waiting = 0;
	}
}


