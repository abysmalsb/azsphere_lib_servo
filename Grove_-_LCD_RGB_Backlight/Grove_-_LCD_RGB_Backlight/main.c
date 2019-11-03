#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <applibs/log.h>
#include <applibs/i2c.h>
#include <soc/mt3620_i2cs.h>

#include "rgb-lcd.h"

#define MT3620_RDB_HEADER4_ISU2_I2C MT3620_I2C_ISU2

int main(void)
{
	int i2cFd = I2CMaster_Open(MT3620_RDB_HEADER4_ISU2_I2C);

	if (i2cFd < 0) {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetTimeout(i2cFd, 100);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	// set up the LCD's number of columns, rows and i2c file descriptor:
	begin(16, 2, i2cFd);

	unsigned char colorR = 255;
	unsigned char colorG = 0;
	unsigned char colorB = 0;

	setRGB(colorR, colorG, colorB);

	// Print a message to the LCD.
	print("hello, world!");

	const struct timespec sleepTime = { 1, 0 };
	nanosleep(&sleepTime, NULL);

	int counter = 0;
	char str[14];
	while (true)
	{
		// set the cursor to column 0, line 1
		// (note: line 1 is the second row, since counting begins with 0):
		setCursor(2, 1);
		// print the number of seconds since reset:
		sprintf(str, "%d", counter++);
		print(str);

		nanosleep(&sleepTime, NULL);
	}
}