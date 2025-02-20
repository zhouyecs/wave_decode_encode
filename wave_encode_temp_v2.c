// 要求添加temp后的第二版

// v1
// 修改decode，可以将temp输出到mode txt
// 修改encode，可以将temp输入到fw文件

// v2
// 指定生成的fw文件名，将文件名输入值fw文件头中
// fw文件头还有48 bytes的剩余空间，
// 分别位于第0~37, 39~47, 63个byte
// 但根据waveform_data_header中的变量名，
// 只有带reserved的为保留剩余空间，共5 bytes
// 这里选择将reserved部分填满
// 这里限制目标文件名的字符范围为0~9, a~f, A~F，文件名长度为10
// 例如：12345ABCDE.fw
// 命令为./wave_encode 12345ABCDE.fw mode*.txt

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 1024
#define MAX_MODE 10
#define MAX_TEMP 20
#define MAX_PHASES 256

#define max(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

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

uint64_t wv_modes_temps[6][14];
uint8_t *waveform_buffer;

// return true if the file specified by the filename exists
bool file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp); // close the file
    }
    return is_exist;
}

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

void wave_encode(int argc, char **argv)
{
    // 1. calculate the size and offset
    int ntemp = 0;                     // the temp of the wave
    int mode = argc - 2;               // the number of modes
    int wave_buffer_size = 0;          // the size of the wave_buffer
    int temp_size[MAX_MODE][MAX_TEMP]; // the size of the temp in the mode
    memset(temp_size, 0, sizeof temp_size);
    int phases_sum = 0;                          // the sum of the phases in mode*.txt
    int phases_mode[MAX_MODE] = {};              // number of phases in the mode
    uint64_t wv_modes[MAX_MODE];                 // the offset of mode
    uint64_t wv_modes_temps[MAX_MODE][MAX_TEMP]; // the offset of temp of each mode
    memset(wv_modes_temps, 0, sizeof wv_modes_temps);
    uint8_t *temp_range_bounds = malloc(14 * sizeof(uint8_t));

    // read the wave
    for (int i = 2; i < argc; i++)
    {
        int t, p, trb_index = 0;
        FILE *file;
        char line[MAX_LINE_LENGTH];
        file = fopen(argv[i], "r");
        if (file == NULL)
        {
            printf("cannot open file %s\n", argv[i]);
            exit(1);
        }

        while (fgets(line, sizeof(line), file))
        {
            if (strncmp(line, "temp", 4) == 0)
            {
                sscanf(line, "temp %d, phases %d", &t, &p);
                temp_range_bounds[trb_index] = (uint8_t)t;
                phases_sum += p;
                phases_mode[i - 2] += p;
                temp_size[i - 2][trb_index] = p;
                trb_index++;
                ntemp = max(ntemp, trb_index - 1);
            }
        }
        fclose(file);
    }
    ntemp++;
    printf("ntemp = %d\n", ntemp);

    wv_modes[0] = 8 * mode;
    // printf("wv_modes[0] = %08lx\n", wv_modes[0]);
    for (int i = 1; i < mode; i++)
    {
        wv_modes[i] = wv_modes[i - 1] + 16 * 16 * phases_mode[i - 1] + 2 * 8 * ntemp;
        // printf("wv_modes[%d] = %08lx\n", i, wv_modes[i]);
    }

    for (int i = 0; i < mode; i++)
    {
        wv_modes_temps[i][0] = wv_modes[i] + 8 * ntemp;
        // printf("wv_modes_temps[%d][0] = %08lx\n", i, wv_modes_temps[i][0]);
        for (int j = 1; j < ntemp; j++)
        {
            wv_modes_temps[i][j] = wv_modes_temps[i][j - 1] + 16 * 16 * temp_size[i][j - 1] + 8;
            // printf("wv_modes_temps[%d][%d] = %08lx\n", i, j, wv_modes_temps[i][j]);
        }
    }

    wave_buffer_size = 16 * 16 * phases_sum + 8 * mode + 2 * 8 * mode * ntemp;
    // printf("wave_buffer_size = %d\n", wave_buffer_size);
    // printf("phases_sum = %d\n", phases_sum);

    // 2. fill the waveform_buffer with wv_modes, wv_modes_temps and luts
    int trt_entries = ntemp;

    int wv_data_offs = 48 + trt_entries + 1;
    // printf("wv_data_offs = %08x\n", wv_data_offs);

    uint8_t *waveform_buffer;
    waveform_buffer = (uint8_t *)malloc(wave_buffer_size);

    int startIndex = 0;
    for (int i = 0; i < mode; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            waveform_buffer[startIndex++] = (uint8_t)(wv_modes[i] >> (8 * j));
            // printf("waveform_buffer[%08x] = %02x\n", startIndex - 1, waveform_buffer[startIndex - 1]);
        }
        // printf("\n");
    }

    for (int i = 0; i < mode; i++)
    {
        startIndex = wv_modes[i];
        for (int j = 0; j < ntemp; j++)
        {
            for (int k = 0; k < 8; k++)
            {
                waveform_buffer[startIndex++] = (uint8_t)(wv_modes_temps[i][j] >> (8 * k));
                // printf("waveform_buffer[%08x] = %02x\n", startIndex - 1, waveform_buffer[startIndex - 1]);
            }
            // printf("\n");
        }
    }

    for (int i = 0; i < mode; i++)
    {
        // int m = wv_modes[i];
        int t = 0, p, value, trb;
        FILE *file;
        char line[MAX_LINE_LENGTH];
        uint8_t luts[MAX_PHASES][16][16];
        file = fopen(argv[i + 2], "r");
        if (file == NULL)
        {
            printf("cannot open file %s\n", argv[i]);
            exit(2);
        }

        while (fgets(line, sizeof(line), file))
        {
            if (strncmp(line, "temp", 4) == 0)
            {
                sscanf(line, "temp %d, phases %d", &trb, &p);

                for (int j = 0; j < 16 * 16; j++)
                {
                    char c;
                    do
                    {
                        fscanf(file, "%c", &c);
                    } while (c != ':');

                    for (int k = 0; k < p; k++)
                    {
                        fscanf(file, "%d", &value);
                        luts[k][j / 16][j % 16] = (uint8_t)value;
                    }
                }

                startIndex = wv_modes_temps[i][t++];
                // printf("wv_modes_temps[%d][%d] = %08x\n", i, t, startIndex);
                for (int j = 0; j < 8; j++)
                {
                    waveform_buffer[startIndex++] = (uint8_t)((uint64_t)p >> (8 * j));
                    // printf("waveform_buffer[%08x] = %02x\n", startIndex - 1, waveform_buffer[startIndex - 1]);
                }

                for (int j = 0; j < p; j++)
                    for (int k = 0; k < 16; k++)
                        for (int l = 0; l < 16; l++)
                        {
                            waveform_buffer[startIndex++] = luts[j][l][k];
                            // if ( )
                            //     printf("luts[%d][%d][%d] = waveform_buffer[%08x] = %d\n", j, l, k, startIndex - 1, waveform_buffer[startIndex - 1]);
                        }

                memset(luts, 0, sizeof luts);
            }
        }
        fclose(file);
    }

    // 3. write the wv_file and waveform_buffer into the buffer, and then write the buffer into the binary file
    char *target_filename = argv[1];
    char filename_without_suffix[10];
    int num[5];
    
    // printf("target_filename: %s\n", target_filename);
    extract_filename_without_suffix(target_filename, filename_without_suffix);
    printf("filename_without_suffix: %s\n", filename_without_suffix);

    struct mxcfb_waveform_data_file *wv_file = malloc(sizeof(struct mxcfb_waveform_data_file));
    wv_file->wdh.trc = trt_entries - 1;

    // transform the filename_without_suffix to 5 numbers
    for(int i = 0; i < 10; i += 2)
    {
        char a = filename_without_suffix[i];
        char b = filename_without_suffix[i + 1];
        if (a >= '0' && a <= '9')
            num[i / 2] = a - '0';
        else if (a >= 'a' && a <= 'f')
            num[i / 2] = a - 'a' + 10;
        else if (a >= 'A' && a <= 'F')
            num[i / 2] = a - 'A' + 10;
        else
            num[i / 2] = 0;
        num[i / 2] = num[i / 2] * 16;
        if (b >= '0' && b <= '9')
            num[i / 2] += b - '0';
        else if (b >= 'a' && b <= 'f')
            num[i / 2] += b - 'a' + 10;
        else if (b >= 'A' && b <= 'F')
            num[i / 2] += b - 'A' + 10;
        else
            num[i / 2] += 0;
    }
    wv_file->wdh.reserved0_1 = num[0];
    wv_file->wdh.reserved0_2 = num[1];
    wv_file->wdh.reserved0_3 = num[2];
    wv_file->wdh.reserved0_4 = num[3];
    wv_file->wdh.reserved0_5 = num[4];

    // printf("size of(struct mxcfb_waveform_data_file): %ld\n", sizeof(struct mxcfb_waveform_data_file));
    // printf("size of wv_file->wdh: %ld\n", sizeof wv_file->wdh);
    // printf("size of wv_file->data: %ld\n", sizeof wv_file->data);
    
    // FILE *pTempFile;
    // uint8_t *temp_range_bounds = malloc(14 * sizeof(uint8_t));
    // int trb = 0;
    // pTempFile = fopen(argv[argc - 1], "r");
    // for (int i = 0; i < 14; i++)
    // {
    //     fscanf(pTempFile, "%d", &trb);
    //     temp_range_bounds[i] = (uint8_t)trb;
    //     printf("temp_range_bounds[%d] = %d\n", i, temp_range_bounds[i]);
    // }
    // fclose(pTempFile);

    char *buffer = malloc(wave_buffer_size + wv_data_offs);
    memcpy(buffer, (char *)wv_file, sizeof(struct mxcfb_waveform_data_file));
    memcpy(buffer + 48, (char *)temp_range_bounds, 14 * sizeof(uint8_t));
    memcpy(buffer + wv_data_offs, (char *)waveform_buffer, wave_buffer_size);

    // printf("trc: %02x\n", buffer[38]);
    // for (int i = 17360, j = 0; i <= 17423; i++, j++)
    // {
    //     if (j % 16 == 0)
    //         printf("\n");
    //     printf("%02x ", (uint8_t)buffer[i]);
    // }

    // create the fw file, and write the buffer into the file
    char *output_filename = argv[1];
    FILE *pFile;
    pFile = fopen(output_filename, "wb");
    if (pFile == NULL)
    {
        fputs("File error", stderr);
        exit(3);
    }
    else
    {
        fwrite(buffer, wave_buffer_size + wv_data_offs, 1, pFile);
        fclose(pFile);
    }
    printf("Total mode number: %d; Total temp number: %d.\n", mode, ntemp);

    free(waveform_buffer);
    free(wv_file);
    free(buffer);
    free(temp_range_bounds);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage: ./wave_encode *.fw temp_range_bounds.txt *.txt\n");
        return EXIT_FAILURE;
    }
    else
    {
        // 6 txt files default
        wave_encode(argc, argv); // argc[1] = *.fw, argc[2, ...] = mode*.txt
    }
    return 0;
}