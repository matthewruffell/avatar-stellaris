/**
 * @file large.c
 * @author Matthew Ruffell
 * @date 8 July 2015
 * @short A short vulnerable program to test Avatar
 */

// Include header files

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/lm3s1968.h"
#include "driverlib/debug.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "drivers/rit128x96x4.h"
#include <string.h>
#include <stdlib.h>

typedef long int32_t;

#ifdef DEBUGa
void
__error__(char *pcFilename, unsigned long ulLine)
{
}
#endif


// Global Variables

/**
 * Gets the current state of PORTB so we can check button presses atomically
 * @param None
 * @returns uint32_t state: The current state of PORTB
 */
static int32_t buttons_check_port (void);

static int32_t debounce_pin_states = 0;

struct _packet {
	int type;
	int length;
	char* data;
};

typedef struct _packet packet;

packet* current_packet;


// Functions

/**
 * A simple copy function that is poorly implemented.
 * This is rife with vulnerabilities and facilitates the
 * stack buffer overflow
 */
char* vulncpy(char* input)
{
	char buffer[20];
    strcpy(buffer, input);
    return buffer;
}

/**
 * Initialises the physical buttons
 * @param None
 * @return Nothing
 */
void buttons_initialise (void)
{
    /* Enable the PORT which the buttons are located */
    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOG);

    /* Enable the pins for which we want to use for control */
	GPIODirModeSet (GPIO_PORTG_BASE, GPIO_PIN_5, GPIO_DIR_MODE_IN); // Left
	GPIODirModeSet (GPIO_PORTG_BASE, GPIO_PIN_6, GPIO_DIR_MODE_IN); // Right
	GPIODirModeSet (GPIO_PORTG_BASE, GPIO_PIN_3, GPIO_DIR_MODE_IN); // UP
	GPIODirModeSet (GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_DIR_MODE_IN); // Down
	GPIODirModeSet (GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_DIR_MODE_IN); // Select

    /* Since the stellaris allows us to configure the maximum current that
     * can flow through particular pins on particular ports, we do this here
     */
	GPIOPadConfigSet (GPIO_PORTG_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA,
	                  GPIO_PIN_TYPE_STD_WPU);  // Left
	GPIOPadConfigSet (GPIO_PORTG_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA,
	  	              GPIO_PIN_TYPE_STD_WPU);  // Right
	GPIOPadConfigSet (GPIO_PORTG_BASE, GPIO_PIN_3, GPIO_STRENGTH_2MA,
	                  GPIO_PIN_TYPE_STD_WPU);  // UP
	GPIOPadConfigSet (GPIO_PORTG_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
	  	              GPIO_PIN_TYPE_STD_WPU);  // Down
	GPIOPadConfigSet (GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
	  	              GPIO_PIN_TYPE_STD_WPU);  // Select
}

/**
 * Gets the current state of PORTB so we can check button presses atomically
 * @param None
 * @returns int32_t state: The current state of PORTB
 */
static int32_t buttons_check_port (void)
{
    int32_t state = 0;
    state = GPIOPinRead (GPIO_PORTG_BASE, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_3 |
                         GPIO_PIN_4 | GPIO_PIN_7);
    return state;
}

void zero_string (char* data, int len)
{
	int i = 0;
	while (i < len)
	{
		data[i] = 0;
		i++;
	}
}

/**
 * Neatly prints a string to the screen
 * @param data - The string to be printed
 * @returns None
 */
void print_string (char* data)
{
	int line_number = 0;
	int line_length = 0;

	char* start;
	char space = ' ';
	static char output[7][21] = {0};
	static defined = false;

	while (line_number < 7)
	{
		zero_string(&output[line_number], 21);
		line_number += 1;
	}

	line_number = 0;

	start = strtok(data, &space);

	// We can only fit 7 lines on the display
	while (line_number < 7)
	{
        // If the string is null, print what we have and exit
		if (start == NULL)
		{
			// ensure null byte on string
			output[line_number][20] = '\0';
			// output current string to display
			RIT128x96x4StringDraw(output[line_number], 5, line_number * 10 + 10, 15);
			break;
		}

		// we can only fit 20 characters on the display
		if ((line_length + strlen(start) + 1) < 20)
		{
            strncpy(&output[line_number][line_length], start, strlen(start));
            line_length += strlen(start) + 1;
            // place a space between the words
            output[line_number][line_length -1] = ' ';
		}
		else // line is longer than 20 characters
		{
			// ensure null byte on string
			output[line_number][20] = '\0';
			// output current string to display
			RIT128x96x4StringDraw(output[line_number], 5, line_number * 10 + 10, 15);
			// reset current line length
			line_length = 0;
			line_number += 1;
			// start building new string
			strncpy(&output[line_number][line_length], start, strlen(start));
			line_length += strlen(start) + 1;
			// place a space between the words
			output[line_number][line_length -1] = ' ';
		}

		// Fetch the next token
		start = strtok(NULL, &space);
	}

}

char* create_string (void)
{
	char* string = malloc(150);
	zero_string(string, 150);
    int current_length = 0;
    int current_position = 0;
    int finished = false;
    int changed = false;

    RIT128x96x4Clear();
    RIT128x96x4StringDraw("Enter your message", 5, 0, 15);
    RIT128x96x4StringDraw("[SELECT] to enter", 5, 85, 15);

    while (!finished)
    {
		int32_t state = buttons_check_port ();

		/* Left button */
		if (!(state & GPIO_PIN_5) && !(debounce_pin_states & GPIO_PIN_5))
		{
			if (current_position != 0)
			{
				string[current_position] = 0;
				current_position -= 1;
				changed = true;
			}
			debounce_pin_states |= GPIO_PIN_5;
		}
		else if ((state & GPIO_PIN_5) && debounce_pin_states & GPIO_PIN_5)
		{
			debounce_pin_states ^= GPIO_PIN_5;
		}

		/* Right button */
		if (!(state & GPIO_PIN_6) && !(debounce_pin_states & GPIO_PIN_6))
		{
			if (current_position < 150)
			{
				if (string[current_position] == 0)
				{
					string[current_position] = ' ';
				}
				current_position += 1;
				changed = true;
			}
			debounce_pin_states |= GPIO_PIN_6;
		}
		else if ((state & GPIO_PIN_6) && debounce_pin_states & GPIO_PIN_6)
		{
			debounce_pin_states ^= GPIO_PIN_6;
		}

		/* Up button */
		if (!(state & GPIO_PIN_3) && !(debounce_pin_states & GPIO_PIN_3))
		{
			if (string[current_position] == 0)
			{
				string[current_position] = 'A';
			}
			else if (string[current_position] < 'z')
			{
				string[current_position] += 1;
			}
			changed = true;
			debounce_pin_states |= GPIO_PIN_3;
		}
		else if ((state & GPIO_PIN_3) && debounce_pin_states & GPIO_PIN_3)
		{
			debounce_pin_states ^= GPIO_PIN_3;
		}

		/* Down button */
		if (!(state & GPIO_PIN_4) && !(debounce_pin_states & GPIO_PIN_4))
		{
			if (string[current_position] == 0)
			{
				string[current_position] = 'z';
			}
			else if(string[current_position] > 'A')
			{
				string[current_position] -= 1;
			}
			changed = true;
			debounce_pin_states |= GPIO_PIN_4;
		}
		else if ((state & GPIO_PIN_4) && debounce_pin_states & GPIO_PIN_4)
		{
			debounce_pin_states ^= GPIO_PIN_4;
		}

		// Select Button
		if (!(state & GPIO_PIN_7) && !(debounce_pin_states & GPIO_PIN_7))
		{
			finished = true;
			debounce_pin_states |= GPIO_PIN_7;
		}
		else if ((state & GPIO_PIN_7) && debounce_pin_states & GPIO_PIN_7)
		{
			debounce_pin_states ^= GPIO_PIN_7;
		}

		if (changed)
		{
			changed = false;
			print_string (string);
		}
    }

    return string;
}

/**
 * Reads in a packet from the RX UART0 and
 * makes a struct of that packet
 * @param None
 * @returns packet - the received packet
 * Packet format:
 * -------------------------------------------
 * | 1b |  2b  |  variable                   |
 * -------------------------------------------
 * |Type|Length|Data                         |
 * -------------------------------------------
 */
packet* process_packet (void)
{
    int i = 0;
    packet* received = NULL;

    if (UARTCharsAvail (UART0_BASE))
    {
    	received = malloc(12);
    	current_packet = received;

    	received->type = ((char) UARTCharGet (UART0_BASE)) - 48;
    	received->length = ((char) UARTCharGet (UART0_BASE)) - 48;
    	received->data = malloc (received->length + 1);
    	while (i < received->length)
    	{
    		received->data[i] = ((char) UARTCharGet (UART0_BASE));
    		i++;
    	}
    	if (received->data)
    	{
    	    received->data[i] = '\0';
    	}
    }

    return received;
}

/**
 * Builds and sends a packet
 * @param char* message - the message to transmit
 * @returns None
 */
void send_packet (char* message)
{
	int length = strlen (message);
	int i = 0;

	packet transmit;
	transmit.type = 1;
	transmit.length = length;
	transmit.data = message;

	if (UARTSpaceAvail (UART0_BASE))
	{
		UARTCharPut (UART0_BASE, transmit.type);
		UARTCharPut (UART0_BASE, transmit.length);
		while (i < transmit.length)
		{
			UARTCharPut(UART0_BASE, transmit.data[i]);
			i++;
		}
	}
}

/**
 * Displays the main menu
 * @param None
 * @returns None
 */
void display_menu (void)
{
    RIT128x96x4Clear();
    RIT128x96x4StringDraw("Welcome!", 5, 0, 15);
    RIT128x96x4StringDraw("[UP] Receive string", 5, 20, 15);
    RIT128x96x4StringDraw("[LEFT] for About", 5, 30, 15);
    RIT128x96x4StringDraw("[RIGHT] for Help", 5, 40, 15);
    RIT128x96x4StringDraw("[DOWN] Send string", 5, 50, 15);
    RIT128x96x4StringDraw("or send UART packet", 5, 85, 15);
}

/**
 * Receives a packet from UART and displays message on screen
 * @param None
 * @returns None
 */
void receive_message (packet* received)
{
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("Receiving message:", 5, 0, 15);
	RIT128x96x4StringDraw("[SELECT] to return", 5, 85, 15);
	if (!received)
	{
		while (received == NULL)
		{
			received = process_packet ();
		}
	}
	vulncpy (received->data);
	print_string (received->data);
}

/**
 * Displays the about screen
 * @param None
 * @returns None
 */
void display_about (void)
{
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("About", 5, 0, 15);
	RIT128x96x4StringDraw("Matthew Ruffell", 5, 20, 15);
	RIT128x96x4StringDraw("Computer Science Hons", 5, 30, 15);
	RIT128x96x4StringDraw("University of", 5, 40, 15);
	RIT128x96x4StringDraw("Canterbury 2015", 5, 50, 15);
	RIT128x96x4StringDraw("[SELECT] to return", 5, 85, 15);
}

/**
 * Displays the help screen
 * @param None
 * @returns None
 */
void display_help (void)
{
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("Help", 5, 0, 15);
	RIT128x96x4StringDraw("Intentionally", 5, 20, 15);
	RIT128x96x4StringDraw("vulnerable app", 5, 30, 15);
	RIT128x96x4StringDraw("to demonstrate", 5, 40, 15);
	RIT128x96x4StringDraw("automatic exploit", 5, 50, 15);
	RIT128x96x4StringDraw("generation", 5, 60, 15);
	RIT128x96x4StringDraw("[SELECT] to return", 5, 85, 15);
}

/**
 * Enables the user to create a message and send it over UART
 * @param None
 * @returns None
 */
void send_message (void)
{
	RIT128x96x4Clear();
	RIT128x96x4StringDraw("Enter your message", 5, 0, 15);
	RIT128x96x4StringDraw("[UP] to increment", 5, 20, 15);
	RIT128x96x4StringDraw("[LEFT] to move left", 5, 30, 15);
	RIT128x96x4StringDraw("[RIGHT] to move right", 5, 40, 15);
	RIT128x96x4StringDraw("[DOWN] decrement", 5, 50, 15);
	RIT128x96x4StringDraw("[SELECT] to enter", 5, 85, 15);
    char* message = create_string ();
    send_packet (message);

}


/**
 * Processes the button presses in main menu
 * @param None
 * @returns None
 */
void process_buttons (void)
{
	int32_t state = buttons_check_port ();

	/* Left button */
	if (!(state & GPIO_PIN_5) && !(debounce_pin_states & GPIO_PIN_5))
	{
		display_about ();
		debounce_pin_states |= GPIO_PIN_5;
	}
	else if ((state & GPIO_PIN_5) && debounce_pin_states & GPIO_PIN_5)
	{
		debounce_pin_states ^= GPIO_PIN_5;
	}

	/* Right button */
	if (!(state & GPIO_PIN_6) && !(debounce_pin_states & GPIO_PIN_6))
	{
        display_help ();
		debounce_pin_states |= GPIO_PIN_6;
	}
	else if ((state & GPIO_PIN_6) && debounce_pin_states & GPIO_PIN_6)
	{
		debounce_pin_states ^= GPIO_PIN_6;
	}

	/* Up button */
	if (!(state & GPIO_PIN_3) && !(debounce_pin_states & GPIO_PIN_3))
	{
        receive_message (NULL);
		debounce_pin_states |= GPIO_PIN_3;
	}
	else if ((state & GPIO_PIN_3) && debounce_pin_states & GPIO_PIN_3)
	{
		debounce_pin_states ^= GPIO_PIN_3;
	}

	/* Down button */
	if (!(state & GPIO_PIN_4) && !(debounce_pin_states & GPIO_PIN_4))
	{
		send_message ();
		debounce_pin_states |= GPIO_PIN_4;
	}
	else if ((state & GPIO_PIN_4) && debounce_pin_states & GPIO_PIN_4)
	{
		debounce_pin_states ^= GPIO_PIN_4;
	}

	// Select Button
	if (!(state & GPIO_PIN_7) && !(debounce_pin_states & GPIO_PIN_7))
	{
		display_menu();
		debounce_pin_states |= GPIO_PIN_7;
	}
	else if ((state & GPIO_PIN_7) && debounce_pin_states & GPIO_PIN_7)
	{
		debounce_pin_states ^= GPIO_PIN_7;
	}
}

/**
 * Initialises the UART for use
 * @param None
 * @returns None
 */
void uart_initialise (void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	UARTConfigSetExpClk (UART0_BASE, SysCtlClockGet (), 38400,
			UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			UART_CONFIG_PAR_NONE);

	UARTEnable(UART0_BASE);
}



/**
 * Checks for a packet on the UART and processes it
 * @param None
 * @returns None
 */
void process_uart (void)
{
    // Check to see if there is data waiting
	if (UARTCharsAvail (UART0_BASE))
	{
        packet* received = process_packet ();

		switch (received->type)
		{
		case 1:
			receive_message (received);
			break;
		case 2:
			display_about ();
			break;
		case 3:
			display_help ();
			break;
		case 4:
			send_message ();
		}
	}
}

int main(void)
{
    // Set the clock to run from the crystal at 8Mhz
    SysCtlClockSet (SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_8MHZ);

    // Initialise the OLED display.
    RIT128x96x4Init (1000000);

    // Initialise the buttons
    buttons_initialise ();

    // Initialise UART
    uart_initialise ();

    // Initialise variables

    // Show the menu
    display_menu ();

    while(1)
    {
        // Check for button presses
        process_buttons ();

        // Check for UART packets
        process_uart ();

    }
}
