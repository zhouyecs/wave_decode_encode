// 要求添加temp后的第一版，这个版本会将temp数输出到txt文件中，以便encode使用

/*
 Freescale/NXP EPDC waveform firmware dumper
*/
#include <stdint.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Type 	Initial State 	Final State 	Waveform Period
INIT 	0-F 			F 	~4000 ms?
DU 		0-F 			0 or F 	~260 ms
GC16 	0-F 			0-F 	~760 ms
GC4 	0-F 			0, 5, A, or F 	~500 ms
A2 		0 or F 			0 or F 	~120 ms
*/

/*
少了GL16, 6 mode 有该模式
 */

// header structures from linux driver
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

#define WAVEFORM_MODE_INIT 0
#define WAVEFORM_MODE_DU 1
#define WAVEFORM_MODE_GC16 2
#define WAVEFORM_MODE_GC4 3
#define WAVEFORM_MODE_A2 4

uint8_t *waveform_buffer;
uint32_t wf_table[4096]; // according mode temp waveform table

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)       \
	(byte & 0x80 ? '1' : '0'),     \
		(byte & 0x40 ? '1' : '0'), \
		(byte & 0x20 ? '1' : '0'), \
		(byte & 0x10 ? '1' : '0'), \
		(byte & 0x08 ? '1' : '0'), \
		(byte & 0x04 ? '1' : '0'), \
		(byte & 0x02 ? '1' : '0'), \
		(byte & 0x01 ? '1' : '0')
void wave_decode(const char *fp)
{
	long lSize;
	char *buffer;
	size_t result;

	FILE *pFile;

	pFile = fopen(fp, "rb");
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

	int i, j;
	int trt_entries;		  //  temperature range table
	int wv_data_offs;		  //  offset for waveform data
	int waveform_buffer_size; // size for waveform data

	uint8_t *temp_range_bounds;
	trt_entries = wv_file->wdh.trc + 1;

	temp_range_bounds = (uint8_t *)malloc(trt_entries);

	memcpy(temp_range_bounds, &wv_file->data, trt_entries);

	// FILE *pTempFile = fopen("temp_range_bounds.txt", "w");
	// if (pTempFile == NULL)
	// {
	// 	fprintf(stderr, "reate file failed: temp_range_bounds.txt\n");
	// 	exit(EXIT_FAILURE);
	// }
	// for (int i = 0; i < trt_entries; i++)
	// {
	// 	fprintf(pTempFile, "%d ", temp_range_bounds[i]);
	// }

	// for (int i = 0; i < trt_entries; i++)
	// {
	// 	printf("temp_range_bounds[%d] = %d\n", i, temp_range_bounds[i]);
	// }

	wv_data_offs = sizeof(wv_file->wdh) + trt_entries + 1;
	waveform_buffer_size = lSize - wv_data_offs;
	// printf("waveform_buffer_size: %d\n", waveform_buffer_size);
	// printf("lSize: %d\n", lSize);
	// printf("wv_data_offs: %d\n", wv_data_offs);

	waveform_buffer = (uint8_t *)malloc(waveform_buffer_size);
	memcpy(waveform_buffer, (uint8_t *)(buffer) + wv_data_offs, waveform_buffer_size);

	printf("size of mxcfb_waveform_data_file: %ld\n", sizeof(struct mxcfb_waveform_data_file));
	printf("size of wv_file: %ld\n", sizeof(*wv_file));
	printf("size of wv_file->data: %ld\n", sizeof(wv_file->data));
	printf("wv_file->data: %08X\n", wv_file->data);
	printf("size of wv_file->wdh: %ld\n", sizeof(wv_file->wdh));
	// print the content of wv_file->wdh
	printf("wi0: %08X\n", wv_file->wdh.wi0);
	printf("wi1: %08X\n", wv_file->wdh.wi1);
	printf("wi2: %08X\n", wv_file->wdh.wi2);
	printf("wi3: %08X\n", wv_file->wdh.wi3);
	printf("wi4: %08X\n", wv_file->wdh.wi4);
	printf("wi5: %08X\n", wv_file->wdh.wi5);
	printf("wi6: %08X\n", wv_file->wdh.wi6);
	printf("xwia: %08X\n", wv_file->wdh.xwia);
	printf("cs1: %02X\n", wv_file->wdh.cs1);
	printf("wmta: %08X\n", wv_file->wdh.wmta);
	printf("fvsn: %02X\n", wv_file->wdh.fvsn);
	printf("luts: %02X\n", wv_file->wdh.luts);
	printf("mc: %02X\n", wv_file->wdh.mc);
	printf("trc: %02X\n", wv_file->wdh.trc);
	printf("advanced_wfm_flags: %02X\n", wv_file->wdh.advanced_wfm_flags);
	printf("eb: %02X\n", wv_file->wdh.eb);
	printf("sb: %02X\n", wv_file->wdh.sb);
	printf("reserved0_1: %02X\n", wv_file->wdh.reserved0_1);
	printf("reserved0_2: %02X\n", wv_file->wdh.reserved0_2);
	printf("reserved0_3: %02X\n", wv_file->wdh.reserved0_3);
	printf("reserved0_4: %02X\n", wv_file->wdh.reserved0_4);
	printf("reserved0_5: %02X\n", wv_file->wdh.reserved0_5);
	printf("cs2: %02X\n", wv_file->wdh.cs2);

	// int l = 0;

	// auto calculate the number of mode, the number of temp
	int nmode = (((uint64_t)waveform_buffer[7] << 56) + ((uint64_t)waveform_buffer[6] << 48) + ((uint64_t)waveform_buffer[5] << 40) +
				 ((uint64_t)waveform_buffer[4] << 32) + ((uint64_t)waveform_buffer[3] << 24) + ((uint64_t)waveform_buffer[2] << 16) +
				 ((uint64_t)waveform_buffer[1] << 8) + (uint64_t)waveform_buffer[0]) /
				8;

	int ntemp = trt_entries;

	uint64_t wv_modes[nmode];

	int k = 0;
	uint64_t addr;
	// get modes addr
	for (i = 0; i < nmode * 8; i += 8)
	{
		addr = ((uint64_t)waveform_buffer[i + 7] << 56) + ((uint64_t)waveform_buffer[i + 6] << 48) + ((uint64_t)waveform_buffer[i + 5] << 40) +
			   ((uint64_t)waveform_buffer[i + 4] << 32) + ((uint64_t)waveform_buffer[i + 3] << 24) + ((uint64_t)waveform_buffer[i + 2] << 16) +
			   ((uint64_t)waveform_buffer[i + 1] << 8) + (uint64_t)waveform_buffer[i];
		// fprintf(stderr, "wave #%d addr:%08x\n", k, addr);
		wv_modes[k] = addr;
		k++;
	}

	k = 0;

	uint64_t wv_modes_temps[nmode][ntemp];

	// get modes temp addr
	for (j = 0; j < nmode; j++)
	{
		uint64_t m = wv_modes[j];
		for (i = 0; i < ntemp * 8; i += 8)
		{ // 14 * 8

			addr = ((uint64_t)waveform_buffer[m + i + 7] << 56) + ((uint64_t)waveform_buffer[m + i + 6] << 48) + ((uint64_t)waveform_buffer[m + i + 5] << 40) +
				   ((uint64_t)waveform_buffer[m + i + 4] << 32) + ((uint64_t)waveform_buffer[m + i + 3] << 24) + ((uint64_t)waveform_buffer[m + i + 2] << 16) +
				   ((uint64_t)waveform_buffer[m + i + 1] << 8) + (uint64_t)waveform_buffer[m + i];

			// fprintf(stderr, "wave #%d, temp #%d addr:%08x (%x)\n", j, k, addr, m + i);
			wv_modes_temps[j][k] = addr;
			k++;
		}
		k = 0;
	}

	printf("Total mode number: %d; Total temp number: %d.\n", nmode, ntemp);

	int x, y;
	for (int mode = 0; mode < nmode; mode++)
	{
		// 构建文件名
		char modefile[20];
		sprintf(modefile, "mode%d.txt", mode);

		// 打开文件
		FILE *pModeFile = fopen(modefile, "w");

		// 检查文件是否成功打开
		if (pModeFile == NULL)
		{
			fprintf(stderr, "Create file failed: %s\n", modefile);
			exit(EXIT_FAILURE);
		}

		// wf_table
		sprintf(modefile, "mem_mode%d.txt", mode);

		FILE *pMemModeFile = fopen(modefile, "w");

		if (pMemModeFile == NULL)
		{
			fprintf(stderr, "Create file failed: %s\n", modefile);
			exit(EXIT_FAILURE);
		}

		for (int temp = 0; temp < ntemp; temp++)
		{
			uint64_t m = wv_modes_temps[mode][temp];
			uint64_t phases = ((uint64_t)waveform_buffer[m + 7] << 56) + ((uint64_t)waveform_buffer[m + 6] << 48) + ((uint64_t)waveform_buffer[m + 5] << 40) +
							  ((uint64_t)waveform_buffer[m + 4] << 32) + ((uint64_t)waveform_buffer[m + 3] << 24) + ((uint64_t)waveform_buffer[m + 2] << 16) +
							  ((uint64_t)waveform_buffer[m + 1] << 8) + (uint64_t)waveform_buffer[m];

			// 写入mode, phases等元数据到文件
			fprintf(pModeFile, "temp %d, phases %d\n", temp_range_bounds[temp], (int)phases);
			fprintf(pMemModeFile, "lut_type %d, frame num %d, temp %d, lut_size %d\n", mode, (int)phases, temp, (int)phases * 16);
			uint8_t luts[phases][16][16];

			k = 0;
			for (i = 0; i < phases * 256; i += 256)
			{
				// fprintf(stderr, "mode %d phase %d/%ld : \n", mode, k + 1, phases);
				j = 0;
				for (x = 0; x < 16; x++)
				{
					// uint32_t curvalue = 0;
					for (y = 0; y < 16; y++)
					{
						// curvalue |= ((waveform_buffer[m + 8 + i + j] & 0x3) << (y << 1));
						luts[k][y][x] = waveform_buffer[m + 8 + i + j];
						// luts[k][y][x] = wf_table[y->x][k]
						//  fprintf(stderr, "[%x-%x %x] ", (15 - y), (15 - x), waveform_buffer[m + 8 + i + j]);
						//				printf("%x ",waveform_buffer[m+8+i+j]);
						j++;
					}
					// wf_table[(k << 4) + x] = curvalue;
					//  fprintf(stderr, "\n");
				}
				// fprintf(stderr, "\n");
				k++;
			}

			for (i = 0; i < 16; i++)
			{
				for (j = 0; j < 16; j++)
				{
					fprintf(pModeFile, "%x -> %x : ", i, j);
					for (k = 0; k < phases; k++)
					{
						fprintf(pModeFile, "%x ", luts[k][i][j]);
					}
					fprintf(pModeFile, "\n");
				}
			}

			for (k = 0; k < phases; k++)
			{
				for (i = 0; i < 16; i++)
				{
					uint32_t curvalue = 0;
					for (j = 0; j < 16; j++)
					{
						// fprintf(stderr, "%x -> %x : ", i, j);
						curvalue |= ((luts[k][i][j] & 0x3) << (j * 2));
						// fprintf(stderr, "%x=%x ", luts[k][i][j], wf_table[idx][k]);
					}
					wf_table[k * 16 + i] = curvalue;
					fprintf(pMemModeFile, "0x%08x\n", curvalue);
				}
			}
		}
		fclose(pModeFile);
	}
	free(temp_range_bounds);
	free(waveform_buffer);
	// fclose(pTempFile);
	fclose(pFile);
	free(buffer);
}

char *fwfile = "320_R110_AD6702_ES133TE3C1_TC.fw";

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: ./wave_decode *.fw\n");
	}
	else
	{
		fwfile = argv[1];
	}
	wave_decode(fwfile);

	return 0;
}
