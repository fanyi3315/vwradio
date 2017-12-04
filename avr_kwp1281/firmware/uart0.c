#include "main.h"
#include <stdint.h>
#include <avr/interrupt.h>
#include "uart0.h"

/*************************************************************************
 * UART0
 *************************************************************************/

void uart0_init()
{
    // Baud Rate
#define BAUD 115200
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
#error USE_2X is not supported
#endif

    UCSR0A &= ~(_BV(U2X0));             // Do not use 2X
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); // N-8-1
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);   // Enable RX and TX
    UCSR0B |= _BV(RXCIE0);              // Enable Recieve Complete interrupt

    buf_init(&uart0_rx_buffer);
    buf_init(&uart0_tx_buffer);
}

void uart0_flush_tx()
{
    while (buf_has_byte(&uart0_tx_buffer)) {}
}

void uart0_put(uint8_t c)
{
    buf_write_byte(&uart0_tx_buffer, c);
    // Enable UDRE interrupts
    UCSR0B |= _BV(UDRIE0);
}

void uart0_put16(uint16_t w)
{
    uart0_put(w & 0x00FF);
    uart0_put((w & 0xFF00) >> 8);
}

void uart0_puts(uint8_t *str)
{
    while (*str != '\0')
    {
        uart0_put(*str);
        str++;
    }
}

void uart0_puthex_nib(uint8_t c)
{
    if (c < 10) // 0-9
    {
        uart0_put(c + 0x30);
    }
    else // A-F
    {
        uart0_put(c + 0x37);
    }
}

void uart0_puthex_byte(uint8_t c)
{
    uart0_puthex_nib((c & 0xf0) >> 4);
    uart0_puthex_nib(c & 0x0f);
}

void uart0_puthex_16(uint16_t w)
{
    uart0_puthex_byte((w & 0xff00) >> 8);
    uart0_puthex_byte((w & 0x00ff));
}

// USART Receive Complete
ISR(USART0_RX_vect)
{
    uint8_t c;
    c = UDR0;
    buf_write_byte(&uart0_rx_buffer, c);
}

// USART Data Register Empty (USART is ready to transmit a byte)
ISR(USART0_UDRE_vect)
{
    if (uart0_tx_buffer.read_index != uart0_tx_buffer.write_index)
    {
        UDR0 = uart0_tx_buffer.data[uart0_tx_buffer.read_index++];
    }
    else
    {
        // Disable UDRE interrupts
        UCSR0B &= ~_BV(UDRE0);
    }
}
