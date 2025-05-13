#include <string.h>

struct WavHeader{
	unsigned char riff[4];
	unsigned int overall_size	;
	unsigned char wave[4];
	unsigned char fmt_chunk_marker[4];
	unsigned int length_of_fmt;
	unsigned int format_type;
	unsigned int channels;
	unsigned int sample_rate;
	unsigned int byterate;
	unsigned int block_align;
	unsigned int bits_per_sample;
	unsigned char data_chunk_header [4];
	unsigned int data_size;
};

struct WavHeader ConfigWavHeader(){
  struct WavHeader header;

  // Setting Variables
  memcpy(header.riff, (unsigned char[]){"0000"}, sizeof header.riff);
  header.overall_size = 0;
  memcpy(header.wave, (unsigned char[]){"0000"}, sizeof header.wave);
  memcpy(header.fmt_chunk_marker, (unsigned char[]){"0000"}, sizeof header.fmt_chunk_marker);
  header.length_of_fmt= 0;
  header.format_type = 0;
  header.channels = 0;
  header.sample_rate = 0;
  header.byterate = 0;
  header.block_align = 0;
  header.bits_per_sample= 0;
  memcpy(header.data_chunk_header, (unsigned char[]){"0000"}, sizeof header.data_chunk_header);
  header.data_size = 0;

  return header;
}

char *readFile(char *fileName)
{
    FILE *file = fopen(fileName, "r");
    char *code;
    size_t n = 0;
    int c;

    if (file == NULL)
        return NULL; //could not open file

    code = malloc(1000);

    while ((c = fgetc(file)) != EOF)
    {
        code[n++] = (char) c;
    }

    // don't forget to terminate with the null character
    code[n] = '\0';        

    return code;
}

