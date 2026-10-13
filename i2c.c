// Distributed with a free-will license.
// Use it any way you want, profit or free, provided it fits in the licenses of its associated works.
// BMP280
// This code is designed to work with the BMP280_I2CS I2C Mini Module available from ControlEverything.com.
// https://www.controleverything.com/content/Barometer?sku=BMP280_I2CSs#tabs-0-product_tabset-2

#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SSD1306_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

static int oled_command(int file, uint8_t command)
{
	uint8_t data[2] = {0x00, command};
	return write(file, data, sizeof(data)) == sizeof(data) ? 0 : -1;
}

static void oled_glyph(char character, uint8_t columns[5])
{
	static const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static const uint8_t digits[10][5] = {
		{0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
		{0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
		{0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
		{0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
		{0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}
	};
	static const uint8_t alphabet[][5] = {
		{0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
		{0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
		{0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
		{0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
		{0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
		{0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
		{0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x02, 0x04, 0x08, 0x7F},
		{0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
		{0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
		{0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
		{0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x03, 0x0C, 0x70, 0x0C, 0x03},
		{0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
		{0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}
	};
	static const uint8_t punctuation[][5] = {
		{0x00, 0x36, 0x36, 0x00, 0x00},
		{0x00, 0x60, 0x60, 0x00, 0x00},
		{0x08, 0x08, 0x08, 0x08, 0x08}
	};
	const uint8_t blank[5] = {0, 0, 0, 0, 0};
	const uint8_t *glyph = blank;

	if(character >= '0' && character <= '9')
	{
		glyph = digits[character - '0'];
	}
	else if(strchr(letters, character))
	{
		glyph = alphabet[strchr(letters, character) - letters];
	}
	else if(character == ':')
	{
		glyph = punctuation[0];
	}
	else if(character == '.')
	{
		glyph = punctuation[1];
	}
	else if(character == '-')
	{
		glyph = punctuation[2];
	}
	memcpy(columns, glyph, 5);
}

static void oled_text(int x, int y, const char *text)
{
	for(; *text; text++, x += 6)
	{
		uint8_t glyph[5];
		oled_glyph(*text, glyph);
		for(int column = 0; column < 5; column++)
		{
			for(int row = 0; row < 7; row++)
			{
				int pixel_x = x + column;
				int pixel_y = y + row;
				if(pixel_x >= 0 && pixel_x < OLED_WIDTH && pixel_y >= 0 && pixel_y < OLED_HEIGHT && (glyph[column] & (1 << row)))
				{
					oled_buffer[pixel_x + (pixel_y / 8) * OLED_WIDTH] |= (uint8_t)(1 << (pixel_y % 8));
				}
			}
		}
	}
}

static int oled_show(int file, double temperature, double pressure,
		     double average_temperature, double average_pressure, int average_available)
{
	static const uint8_t init_commands[] = {
		0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
		0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
		0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
	};
	char temperature_text[32];
	char pressure_text[32];
	uint8_t page_data[OLED_WIDTH + 1];

	if(ioctl(file, I2C_SLAVE, SSD1306_ADDRESS) < 0)
	{
		return -1;
	}
	memset(oled_buffer, 0, sizeof(oled_buffer));
	for(size_t i = 0; i < sizeof(init_commands); i++)
	{
		if(oled_command(file, init_commands[i]) < 0)
		{
			return -1;
		}
	}

	if(average_available)
	{
		snprintf(temperature_text, sizeof(temperature_text), "T:%.2f A:%.2f C", temperature, average_temperature);
		snprintf(pressure_text, sizeof(pressure_text), "P:%.1f A:%.1f HPA", pressure, average_pressure);
	}
	else
	{
		snprintf(temperature_text, sizeof(temperature_text), "T:%.2f A:-- C", temperature);
		snprintf(pressure_text, sizeof(pressure_text), "P:%.1f A:-- HPA", pressure);
	}
	// GRUPO: Cairo, Hugo, Bruno, Marina
	oled_text(4, 2, "BRUNO CAIRO HUGO MARINA");
	oled_text(4, 26, temperature_text);
	oled_text(1, 46, pressure_text);

	if(oled_command(file, 0x21) < 0 || oled_command(file, 0x00) < 0 || oled_command(file, OLED_WIDTH - 1) < 0 ||
	   oled_command(file, 0x22) < 0 || oled_command(file, 0x00) < 0 || oled_command(file, OLED_HEIGHT / 8 - 1) < 0)
	{
		return -1;
	}
	page_data[0] = 0x40;
	for(int page = 0; page < OLED_HEIGHT / 8; page++)
	{
		memcpy(page_data + 1, oled_buffer + page * OLED_WIDTH, OLED_WIDTH);
		if(write(file, page_data, sizeof(page_data)) != sizeof(page_data))
		{
			return -1;
		}
	}
	return 0;
}

static int write_average_csv(double temperature, double pressure)
{
	FILE *csv = fopen("bmp280_averages.csv", "a+");
	if(csv == NULL)
	{
		perror("Failed to open bmp280_averages.csv");
		return -1;
	}

	if(fseek(csv, 0, SEEK_END) != 0)
	{
		perror("Failed to seek in bmp280_averages.csv");
		fclose(csv);
		return -1;
	}
	if(ftell(csv) == 0 && fprintf(csv, "timestamp,avg_temperature_c,avg_pressure_hpa\n") < 0)
	{
		perror("Failed to write CSV header");
		fclose(csv);
		return -1;
	}

	time_t now = time(NULL);
	struct tm timestamp;
	char timestamp_text[32];
	if(now == (time_t)-1 || localtime_r(&now, &timestamp) == NULL ||
	   strftime(timestamp_text, sizeof(timestamp_text), "%Y-%m-%d %H:%M:%S", &timestamp) == 0)
	{
		fprintf(stderr, "Failed to get timestamp for CSV row\n");
		fclose(csv);
		return -1;
	}

	int result = fprintf(csv, "%s,%.2f,%.2f\n", timestamp_text, temperature, pressure);
	if(result < 0 || fclose(csv) != 0)
	{
		perror("Failed to write CSV average");
		return -1;
	}
	return 0;
}

void main() 
{
	int counter = 0;
	double media_temp = 0;
	double media_pressure = 0;
		double latest_average_temp = 0;
		double latest_average_pressure = 0;
		int average_available = 0;
	// Create I2C bus
	while(1) {
		int file;
		char *bus = "/dev/i2c-1";
		if((file = open(bus, O_RDWR)) < 0) 
		{
			printf("Failed to open the bus. \n");
			exit(1);
		}
		// Get I2C device, BMP280 I2C address is 0x76(108)
		ioctl(file, I2C_SLAVE, 0x76);

		// Read 24 bytes of data from address(0x88)
		char reg[1] = {0x88};
		write(file, reg, 1);
		char data[24] = {0};
		if(read(file, data, 24) != 24)
		{
			printf("Error : Input/output Error \n");
			exit(1);
		}
		// Convert the data
		// temp coefficents
		int dig_T1 = data[1] * 256 + data[0];
		int dig_T2 = data[3] * 256 + data[2];
		if(dig_T2 > 32767)
		{
			dig_T2 -= 65536;
		}
		int dig_T3 = data[5] * 256 + data[4];
		if(dig_T3 > 32767)
		{
			dig_T3 -= 65536;
		}
		// pressure coefficents
		int dig_P1 = data[7] * 256 + data[6];
		int dig_P2  = data[9] * 256 + data[8];
		if(dig_P2 > 32767)
		{
			dig_P2 -= 65536;
		}
		int dig_P3 = data[11]* 256 + data[10];
		if(dig_P3 > 32767)
		{
			dig_P3 -= 65536;
		}
		int dig_P4 = data[13]* 256 + data[12];
		if(dig_P4 > 32767)
		{
			dig_P4 -= 65536;
		}
		int dig_P5 = data[15]* 256 + data[14];
		if(dig_P5 > 32767)
		{
			dig_P5 -= 65536;
		}
		int dig_P6 = data[17]* 256 + data[16];
		if(dig_P6 > 32767)
		{
			dig_P6 -= 65536;
		}
		int dig_P7 = data[19]* 256 + data[18];
		if(dig_P7 > 32767)
		{
			dig_P7 -= 65536;
		}
		int dig_P8 = data[21]* 256 + data[20];
		if(dig_P8 > 32767)
		{
			dig_P8 -= 65536;
		}
		int dig_P9 = data[23]* 256 + data[22];
		if(dig_P9 > 32767)
		{
			dig_P9 -= 65536;
		}
			
		// Select control measurement register(0xF4)
		// Normal mode, temp and pressure over sampling rate = 1(0x27)
		char config[2] = {0};
		config[0] = 0xF4;
		config[1] = 0x27;
		write(file, config, 2);
		
		// Select config register(0xF5)
		// Stand_by time = 1000 ms(0xA0)
		config[0] = 0xF5;
		config[1] = 0xA0;
		write(file, config, 2);
		sleep(1);
		
		// Read 8 bytes of data from register(0xF7)
		// pressure msb1, pressure msb, pressure lsb, temp msb1, temp msb, temp lsb, humidity lsb, humidity msb
		reg[0] = 0xF7;
		write(file, reg, 1);
		if(read(file, data, 8) != 8)
		{
			printf("Error : Input/output Error \n");
			exit(1);
		}
		
		// Convert pressure and temperature data to 19-bits
		long adc_p = (((long)data[0] * 65536) + ((long)data[1] * 256) + (long)(data[2] & 0xF0)) / 16;
		long adc_t = (((long)data[3] * 65536) + ((long)data[4] * 256) + (long)(data[5] & 0xF0)) / 16;
			
		// Temperature offset calculations
		double var1 = (((double)adc_t) / 16384.0 - ((double)dig_T1) / 1024.0) * ((double)dig_T2);
		double var2 = ((((double)adc_t) / 131072.0 - ((double)dig_T1) / 8192.0) *(((double)adc_t)/131072.0 - ((double)dig_T1)/8192.0)) * ((double)dig_T3);
		double t_fine = (long)(var1 + var2);
		double cTemp = (var1 + var2) / 5120.0;
			
		// Pressure offset calculations
		var1 = ((double)t_fine / 2.0) - 64000.0;
		var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
		var2 = var2 + var1 * ((double)dig_P5) * 2.0;
		var2 = (var2 / 4.0) + (((double)dig_P4) * 65536.0);
		var1 = (((double) dig_P3) * var1 * var1 / 524288.0 + ((double) dig_P2) * var1) / 524288.0;
		var1 = (1.0 + var1 / 32768.0) * ((double)dig_P1);
		double p = 1048576.0 - (double)adc_p;
		p = (p - (var2 / 4096.0)) * 6250.0 / var1;
		var1 = ((double) dig_P9) * p * p / 2147483648.0;
		var2 = p * ((double) dig_P8) / 32768.0;
		double pressure = (p + (var1 + var2 + ((double)dig_P7)) / 16.0) / 100;
		
		// Output data to screen
		printf("Pressure : %.2f hPa \n", pressure);
		printf("Temperature in Celsius : %.2f C \n", cTemp);

		if(counter < 10)
		{
			media_temp += cTemp;
			media_pressure += pressure;
			counter++;
		}
		else
		{
			double average_temp = media_temp / counter;
			double average_pressure = media_pressure / counter;
			latest_average_temp = average_temp;
			latest_average_pressure = average_pressure;
			average_available = 1;
			printf("#################################################\n");
			printf("AVG Pressure : %.2f hPa \n", average_pressure);
			printf("AVG Temperature in Celsius : %.2f C \n", average_temp);
			printf("#################################################\n");

			if(write_average_csv(average_temp, average_pressure) < 0)
			{
				close(file);
				exit(1);
			}
			counter = 0;
			media_temp = 0;
			media_pressure = 0;
		}

		if(oled_show(file, cTemp, pressure, latest_average_temp, latest_average_pressure, average_available) < 0)
		{
			perror("Failed to display readings on SSD1306");
			close(file);
			exit(1);
		}
	}
}
