#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct waveform_data_header
{
	unsigned int wi0;
	unsigned int wi1;
	unsigned int wi2;
	unsigned int wi3;
	unsigned int wi4;
	unsigned int wi5;
	unsigned int wi6;

	unsigned int xwia : 24;
	unsigned int cs1 : 8;

	unsigned int wmta : 24;
	unsigned int fvsn : 8;
	unsigned int luts : 8;
	unsigned int mc : 8;
	unsigned int trc : 8;
	unsigned int advanced_wfm_flags : 8;
	unsigned int eb : 8;
	unsigned int sb : 8;
	unsigned int reserved0_1 : 8;
	unsigned int reserved0_2 : 8;
	unsigned int reserved0_3 : 8;
	unsigned int reserved0_4 : 8;
	unsigned int reserved0_5 : 8;
	unsigned int cs2 : 8;
};

struct mxcfb_waveform_data_file
{
	struct waveform_data_header wdh;
	uint32_t *data; /* Temperature Range Table + Waveform Data */
};

void extract_filename_without_suffix(const char *file_name, char *result) {
    char *dot = strrchr(file_name, '.');  // Find the last occurrence of '.'
    
    if (dot != NULL) {
        // Copy characters from file_name until just before the dot
        strncpy(result, file_name, dot - file_name);
        result[dot - file_name] = '\0';  // Null-terminate the string
    } else {
        // No dot found, copy the whole file_name
        strcpy(result, file_name);
    }
}

// Function to transform uint8_t numbers to concatenated hex string
void uint8ArrayToConcatenatedHex(uint8_t numbers[], int count, char *result) {
    // Initialize result string
    result[0] = '\0';

    // Temporary buffer for each hex string
    char tempHex[3]; // 2 characters for hex number + 1 for null terminator

    // Loop through each number
    for (int i = 0; i < count; ++i) {
        // Convert current number to hex string
        sprintf(tempHex, "%02X", numbers[i]);

        // Concatenate tempHex to result
        strcat(result, tempHex);
    }
}

bool check(char *file_name)
{
	long lSize;
	char *buffer;
	size_t result;
	FILE *pFile;

	pFile = fopen(file_name, "rb");
	if (pFile == NULL)
	{
		fputs("File error", stderr);
		exit(1);
	}
	// obtain file size:
	fseek(pFile, 0, SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	// allocate memory to contain the whole file:
	buffer = (char *)malloc(sizeof(char) * lSize);
	if (buffer == NULL)
	{
		fputs("Memory error", stderr);
		exit(2);
	}

	// copy the file into the buffer:
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize)
	{
		fputs("Reading error", stderr);
		exit(3);
	}

	/* the whole file is now loaded in the memory buffer. */
	struct mxcfb_waveform_data_file *wv_file;
	wv_file = (struct mxcfb_waveform_data_file *)buffer;

    // get the reserved0_1, reserved0_2, reserved0_3, reserved0_4, reserved0_5
    uint8_t numbers[5];
    numbers[0] = wv_file->wdh.reserved0_1;
    numbers[1] = wv_file->wdh.reserved0_2;
    numbers[2] = wv_file->wdh.reserved0_3;
    numbers[3] = wv_file->wdh.reserved0_4;
    numbers[4] = wv_file->wdh.reserved0_5;

    char filename_without_suffix[11];
    if(strlen(file_name) > 13) return false;
    extract_filename_without_suffix(file_name, filename_without_suffix);
    // printf("filename_without_suffix: %s\n", filename_without_suffix);

    char concatenatedHex[11];
    uint8ArrayToConcatenatedHex(numbers, 5, concatenatedHex);
    // printf("Concatenated hex string: %s\n", concatenatedHex);

    for(int i = 0; i < 10; i ++)
        if(filename_without_suffix[i] != concatenatedHex[i] && filename_without_suffix[i] != concatenatedHex[i] + 32)
            return false;

    return true;
    
    // cannot cmp if there is a lower letter in fw name
    // if (strcmp(filename_without_suffix, concatenatedHex) == 0) {
    //     return true;
    // } else {
    //     return false;
    // }
}

int main(int argc, char **argv)
{
    char *file_name;
	if (argc != 2)
	{
		printf("Usage: ./check_version *.fw\n");
	}
	else
	{
		file_name = argv[1];
	}

	if(check(file_name))
    {
        printf("Version is correct\n");
    }
    else
    {
        printf("Version is incorrect\n");
    }

	return 0;
}