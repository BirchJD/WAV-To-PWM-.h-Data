/*****************************************************************************/
/* WavConverter.c          (C) Jason Birch                 V1.0.0 2022-05-16 */
/*                                                                           */
/* Strip all information from a 16bit sample WAV file. And convert the       */
/* Sample values to mark/space counts summing to WAV_PWM_COUNT based on best */
/* resolution for 125MHz system clock frequency. The format can also be      */
/* easly sent to the PIO SM or PWM via DMA with little CPU power and time    */
/* required.                                                                 */
/*                                                                           */
/* Use Audacity to export files as mono 16 bits resolution, 22050Hz sample   */
/* rate.                                                                     */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WavConverter.h"


int main(int argc, char* argv[])
{
   FILE* File = NULL;
   char* Data = NULL;
   char* Ptr;
   short Value;
   float FadeCount;
   float FadeStep;
   char WavName[BUFF_SIZE+1];
   char Buffer[BUFF_SIZE+1];
   unsigned char FormatPIO = TRUE;
   unsigned long FileSize = 0;
   unsigned long Count = 0;
   unsigned long TotalSize;
   unsigned long WavDataSize;
   unsigned long WavByteRate;
   unsigned short WavChannels;
   unsigned short WavSampleRate;
   unsigned short WavBitsPerSample;

   if (argc != ARG_COUNT)
   {
      printf("\n");
      printf("%s [PIO|PWM] [PATH_FILENAME]\n", argv[ARG_EXE]);
      printf("WHERE:\n");
      printf("[PIO|PWM]       - Produce output for PIO or PWM hardware audio.\n");
      printf("[PATH_FILENAME] - Path & filename of input WAV file.\n");
      printf("\n");
      printf("Converts a 16bit Mono WAV file into a 16bit Mono custom PWM file which\n");
      printf("can be sent directly to the Pico PCM PIO SM for fastest performance.\n");
      printf("\n");
      printf("Output format for PIO:\n");
      printf("2 bytes - WAV Sample Mark Count.\n");
      printf("2 bytes - WAV Sample Space Count.\n");
      printf("...\n");
      printf("\n");
      printf("Output format for PWM:\n");
      printf("2 bytes - WAV Sample Mark Count Left Channel.\n");
      printf("2 bytes - WAV Sample Mark Count Right Channel.\n");
      printf("...\n");
      printf("\n");
   }
   else
   {
  /**********************************/
 /* Read the WAV file into memory. */
/**********************************/
      if ((File = fopen(argv[ARG_PATHFILENAME], "rb")) == NULL)
         printf("Failed to open file for reading: %s\n", argv[ARG_PATHFILENAME]);
      else
      {
         fseek(File, 0, SEEK_END);
         FileSize = ftell(File);
         rewind(File);
         if ((Data = malloc(FileSize)) != NULL)
            fread(Data, FileSize, 1, File);
         fclose(File);
      }
  /**********************************************/
 /* Check the file format is the correct type. */
/**********************************************/
      if (strncmp(&(Data[WAV_ARG_FORMAT]), "WAVE", 4))
         printf("Not a WAV file: %s\n", argv[ARG_PATHFILENAME]);
      else
      {
         if (!strcmp(argv[ARG_PIO_PWM], "PWM"))
            FormatPIO = FALSE;

         WavChannels = *(unsigned short*)&Data[WAV_ARG_NUM_CHANNELS];
         WavSampleRate = *(unsigned int*)&Data[WAV_ARG_SAMPLE_RATE];
         WavByteRate = *(unsigned int*)&Data[WAV_ARG_BYTE_RATE];
         WavBitsPerSample = *(unsigned short*)&Data[WAV_ARG_BITS_PER_SAMPLE];
         WavDataSize = *(unsigned int*)&Data[WAV_ARG_SUB_CHUNK2_SIZE];

         printf("Output Format: %s\n", argv[ARG_PIO_PWM]);
         printf("File: %s\n", argv[ARG_PATHFILENAME]);
         printf("Channels: %u\n", WavChannels);
         printf("Sample Rate: %u\n", WavSampleRate);
         printf("Byte Rate: %lu\n", WavByteRate);
         printf("Bits Per Sample: %u\n", WavBitsPerSample);
         printf("Data Size: %lu\n", WavDataSize);

  /***************************************************************/
 /* Check the WAV file data is mono, 16bit & 22050 sample rate. */
/***************************************************************/
         if (WavChannels != 1 || WavSampleRate != 22050 || WavBitsPerSample != 16)
            printf("WAV File Needs To Be Of Format - Channels: 1 - SampleRate: 22050 - Bits Per Sample: 16\n");
         else
         {
  /************************************/
 /* Save the refined data to a file. */
/************************************/
            strcpy(Buffer, argv[ARG_PATHFILENAME]);
            if ((Ptr = strrchr(Buffer, '.')))
               Ptr[0] = '\0';
            strcpy(WavName, &(Buffer[2]));
            strcat(Buffer, ".h");
            printf("Output file: %s\n", Buffer);
            if ((File = fopen(Buffer, "wb")) == NULL)
               printf("Failed to open file for writing: %s\n", Buffer);
            else
            {
  /***********************************************************/
 /* Write a file headder for a C include file for the data. */
/***********************************************************/
               TotalSize = 1 + WavDataSize + (4 * WAV_FADE_SIZE);
               fprintf(File, "#ifndef __%s_WAV_H\n", WavName);
               fprintf(File, "#define __%s_WAV_H\n", WavName);

               if (FormatPIO)
                  fprintf(File, "// Data intended for PIO SM hardware option.\n", WavName);
               else
                  fprintf(File, "// Data intended for PWM hardware option.\n", WavName);

               fprintf(File, "const unsigned short %s_WAV[]=\n{ 0x%04X,0x%04X,\n", WavName, (TotalSize & 0xFFFF), (TotalSize >> 16));

  /**********************************************************************************/
 /* Write fade in data as idle period is 0V and active silent PWM value is 50% on. */
/**********************************************************************************/
               Value = *(short*)(&Data[WAV_ARG_DATA]);
               Value = WAV_PWM_COUNT * (Value + 32768) / 65536;
               FadeCount = WAV_PWM_COUNT;
               FadeStep = (float)Value / WAV_FADE_SIZE;
               for (Count = 0; Count < WAV_FADE_SIZE; ++Count)
               {
                  if (FormatPIO)
                     fprintf(File, "0x%X,0x%X,", WAV_PWM_COUNT - (short)FadeCount, (short)FadeCount);
                  else
                     fprintf(File, "0x%X,0x%X,", WAV_PWM_COUNT - (short)FadeCount, WAV_PWM_COUNT - (short)FadeCount); // Left channel << 16, right channel & 0xFFFF

                  FadeCount -= FadeStep;
                  if (!((Count + 2) % 6))
                     fputs("\n", File);
               }
               fputs("\n\n", File);

  /************************************************/
 /* Prepare to plot wav data for debug purposes. */
/************************************************/
//               memset(Buffer, ' ', 80);
//               Buffer[79] = '\r';
//               Buffer[80] = '\0';

  /*******************************************/
 /* Write PWM audio data to C headder file. */
/*******************************************/
               for (Count = 0; Count < WavDataSize; Count += sizeof(short))
               {
                  Value = *(short*)(&Data[WAV_ARG_DATA + Count]);
                  Value = WAV_PWM_COUNT * (Value + 32768) / 65536;
                  if (!Value)
                     ++Value;

                  if (FormatPIO)
                     fprintf(File, "0x%X,0x%X,", Value, WAV_PWM_COUNT - Value);
                  else
                     fprintf(File, "0x%X,0x%X,", Value, Value); // Left channel << 16, right channel & 0xFFFF

                  if (!((Count + 2) % 12))
                     fputs("\n", File);

  /*************************************/
 /* Plot wav data for debug purposes. */
/*************************************/
//                  Value = 78 * Value / WAV_PWM_COUNT;
//                  Buffer[Value] = '*';
//                  puts(Buffer);
//                  Buffer[Value] = ' ';
               }

  /***********************************************************************************/
 /* Write fade out data as idle period is 0V and active silent PWM value is 50% on. */
/***********************************************************************************/
               fputs("\n\n", File);
               FadeCount = Value;
               FadeStep = (float)Value / WAV_FADE_SIZE;
               for (Count = 0; Count < WAV_FADE_SIZE; ++Count)
               {
                  FadeCount -= FadeStep;
                  if (FormatPIO)
                     fprintf(File, "0x%X,0x%X,", (short)FadeCount, WAV_PWM_COUNT - (short)FadeCount);
                  else
                     fprintf(File, "0x%X,0x%X,", (short)FadeCount, (short)FadeCount); // Left channel << 16, right channel & 0xFFFF

                  if (!((Count + 2) % 6))
                     fputs("\n", File);
               }

  /*****************************************************************************/
 /* Terminate data with zero values, to tell the PIO SM to go into idle mode. */
/*****************************************************************************/
               fputs("\n\n0x00,0x00\n};\n#endif\n", File);
               fclose(File);
            }
         }
      }

  /*************************************/
 /* Ensure allocated memory is freed. */
/*************************************/
      if (Data != NULL)
         free(Data);
   }
   
   return FALSE;
}
