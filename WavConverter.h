#ifndef __WAVCONVERTER_H
#define __WAVCONVERTER_H


#define ARG_COUNT                3
#define ARG_EXE                  0
#define ARG_PIO_PWM              1
#define ARG_PATHFILENAME         2

#define FALSE                    0
#define TRUE                     -1

#define BUFF_SIZE                1024

#define WAV_SAMPLE_RATE          22050
#define WAV_PWM_COUNT            (125000000 / WAV_SAMPLE_RATE)
#define WAV_FADE_SIZE            (WAV_PWM_COUNT / 10)


  /**************************/
 /* WAV file data offsets. */
/**************************/
#define WAV_ARG_CHUNK_ID         0
#define WAV_ARG_CHUNK_SIZE       4
#define WAV_ARG_FORMAT           8
#define WAV_ARG_SUB_CHUNK_ID     12
#define WAV_ARG_SUB_CHUNK_SIZE   16
#define WAV_ARG_AUDIO_FORMAT     20
#define WAV_ARG_NUM_CHANNELS     22
#define WAV_ARG_SAMPLE_RATE      24
#define WAV_ARG_BYTE_RATE        28
#define WAV_ARG_BLOCK_ALIGN      32
#define WAV_ARG_BITS_PER_SAMPLE  34
#define WAV_ARG_SUB_CHUNK2_ID    36
#define WAV_ARG_SUB_CHUNK2_SIZE  40
#define WAV_ARG_DATA             44


#endif
