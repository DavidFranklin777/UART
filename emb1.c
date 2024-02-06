#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Story EMB-1
 * -----------
 *
 * Write an initialization routine for a UART device. The UART is a memory
 * mapped device at the address 0xFC000000 on an embedded platform.
 * This peripheral is controlled by the following 32 bit registers (offsets given)
 *
 *  CNR: control register Offset: 0x0
 *  - Bit 0: enable UART TX/RX
 *  - Bit 1: enable UART interrupt; interrupt will be caused by changes in STA register
 *
 * BRR: Baud rate register Offset: 0x4 
 * BRR[0:3] 0-3 bits Selects the baud rate as follows
 *  - 0: 4800
 *  - 1: 9600
 *  - 2: 14400
 *  - 3: 19200
 *  - 4: 38400
 *  - 5: 57600
 *  - 6: 115200
 *  - 7: 128000
 *  - 8: 256000
 *
 *  BRR[4:5] 4th and 5th bit Selects parity as follows
 *  - 0 Even Parity
 *  - 1 Odd Parity
 *  - 2 No Parity
 *
 *  - BRR[8] Turning this bit on enables hardware flow control
 *
 *  - BRR[12:15] Contains the number of stop bits
 *
 *  STA: status register Offset: 0x8
 *  - Bit 0: RX ready: if set a byte can be read from RDR, reading the byte will clear RX ready (READ-ONLY)
 *  - Bit 1: TX ready: if set a byte can be writter to TDR, writing will clear TX ready until next byte can be written (READ-ONLY)
 *
 * TDR: Transmit data register     Offset: 0xC
 *  - Contains data to be transmitted via UART
 *
 * RDR: Receive data register     Offset: 0x10  (READ-ONLY)
 *  - Contains data received via UART
 *
 * You need to write an initialization routine for this UART in C with the following configuration.
 *
 * Baud Rate: 115200, stop bits 1, parity none, flow control none
 * TX and RX are interrupt based operations, with data registers cleared at the start of operation.
 */



// UART register offsets as given in the description
#define UART_BASE 0xFC000000
#define CNR_OFFSET 0x0
#define BRR_OFFSET 0x4
#define STA_OFFSET 0x8
#define TDR_OFFSET 0xC
#define RDR_OFFSET 0x10

#define TIMEOUT_LIMIT 1000000 // Timeout limit for UART to prevent infinite waiting time


// Data structure containing UART parameters including base address and other settings.
// Using volatile because we do not want the compiler to treat them as regular constants and optimize them
// Using volatile ensures that the compiler will not optimize those areas of code.
// baseAddress is not volatile because generally it is fixed
// The values stored at each of these registers can change regardless of the program flow
// So declaring the data pointed by pointers as volatile indicates that the values these pointers point to are subject to change
typedef struct
{
    uintptr_t  baseAddress;   // Using uintptr_t  because my system is 64. uintptr_t is based on size of the underlying architecture
    volatile uint32_t *cnr;   // Using uint32_t because its specified that these are 32 bit registers
    volatile uint32_t *brr;   
    volatile uint32_t *sta;
    volatile uint32_t *tdr;
    volatile uint32_t *rdr; 

    // Flag to check if UART is initialized
    bool isInitialized;
} UART_HANDLE;

// write operation should write one byte to UART
void write_uart(UART_HANDLE *h, char c)
{
    if(!h)
    {
        printf("Null pointer encountered \n");
        return;
    }

    // Check if device is initialized
    if (h->isInitialized == false) 
    {
        printf("UART is not initialized.\n");
        return;
    }

    /*  STA: status register Offset: 0x8
    *  - Bit 0: RX ready: if set a byte can be read from RDR, reading the byte will clear RX ready (READ-ONLY)
    *  - Bit 1: TX ready: if set a byte can be writter to TDR, writing will clear TX ready until next byte can be written (READ-ONLY)
    */

    uint32_t staVal = *(h->sta);
    // Wait until TX is ready, only if second bit is one in status register, we send data
    int timer = 0;
    while(!(staVal & 0x02))
    {
        timer++;
        if(timer > TIMEOUT_LIMIT)
        {
            printf("Maximum timeout reached. Device is not yet available to write new data");
            return;
        }
    }

    // Exit while loop if device is ready and send data into tdr space
    printf("Writing data .. \n");
    *(h->tdr) = c;
}

// read one byte from UART
int read_uart(UART_HANDLE *h)
{
    if(!h)
    {
        printf("Null pointer encountered \n");
        return -1;
    }
    else if (h->isInitialized == false)
    {
        printf("UART not initialized\n");
        return -1;
    }
    int timer = 0;

    // Wait until RX ready
    uint32_t staVal = *(h->sta);

    while (!(staVal & 0x01))
    {
        timer++;
        if(timer > TIMEOUT_LIMIT)
        {
            printf("Maximum timeout reached. Device is not yet available to read new data");
            return -1; // error value
        }
    }

    uint32_t rdrVal = *(h->rdr);
    return rdrVal;
}

// init UART
void init_uart(UART_HANDLE *h, bool useMock) {
    if (!h) {
        printf("Null pointer encountered \n");
        return;
    }


    // Point to real hardware registers
    h->baseAddress = UART_BASE;
    h->cnr = (volatile uint32_t *)(UART_BASE + CNR_OFFSET);
    h->brr = (volatile uint32_t *)(UART_BASE + BRR_OFFSET);
    h->sta = (volatile uint32_t *)(UART_BASE + STA_OFFSET);
    h->tdr = (volatile uint32_t *)(UART_BASE + TDR_OFFSET);
    h->rdr = (volatile uint32_t *)(UART_BASE + RDR_OFFSET);

    // Configure CNR and BRR as required
    *(h->cnr) = 0x3;    // Enable UART TX/RX and interrupt
    *(h->brr) = 0x0866; // Baud rate 115200, no parity, 1 stop bit
    h->isInitialized = true;
}


// Provide a sample utilizing the functions above 
// the produced code need not work (since it lacks the actual UART), but it needs
// to be "functional" in the sense that it does the right thing in code review
int main(int argc, char *argv[])
{
    UART_HANDLE uartHandle;

    // Initialize the isInitialized flag to false
    uartHandle.isInitialized = false;

    // Initialize UART
    init_uart(&uartHandle, true); 

    // Example usage of write and read
    write_uart(&uartHandle, 'A');

    int receivedData = read_uart(&uartHandle);

    if(receivedData != -1)
    {
        printf("Received data: %c\n", receivedData);
    }
    else
    {
        printf("Error in reading from UART. \n");
    }

    return 0;

}
