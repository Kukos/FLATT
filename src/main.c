#include <stdio.h>
#include <stdlib.h>
#include <pattern.h>
#include <file_buffer.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>

void help()
{
	printf(	"Match pattern in text or file\n"
			"\nUsage:\n"
			"--text | -t\t\ttext on stdin\n"
			"--pattern | -p\t\tpattern on stdin\n"
			"--file | -f\t\tpath to file with text\n"
			"--kmp | -k\t\tuse Knuth-Morris-Pratt algorithm(default)\n"
			"--fsm | -m\t\tuse Finite state machine\n"
			"--help | -h\t\tprint this info\n"
			"\nExample:\n"
			"./pattern -t \"ala_ma_kota\" -p \"ala\"\n"
			"./pattern -f ./file_with_text -p \"pattern\" --fsm\n" );
}

int main(int argc, char **argv)
{
	int c;
	int fd;
	file_buffer *fb = NULL;
	char *text = NULL;
	char *pattern = NULL;

	void (*match_pattern)(const char*, const char*) = kmp;

	struct option long_option[] =
	{
		{"text",	required_argument,	0,	't'},
		{"pattern",	required_argument,	0,	'p'},
		{"file",	required_argument,	0,	'f'},
		{"kmp",		no_argument,	 	0,	'k'},
		{"fsm",		no_argument,		0,	'm'},
		{"help",	no_argument,		0,	'h'},
		{NULL,		0,					0,	'\0'}

	};

	/* we need text and pattern */
	if(argc <= 3)
	{
		help();
		return 0;
	}

	while( (c = getopt_long(argc,argv,"t:p:f:kmh",long_option,NULL)) != -1)
	{
		switch(c)
		{
			case 't': 	text = argv[optind - 1]; 	break;
			case 'p':	pattern = argv[optind - 1];	break;
			case 'k':	match_pattern = kmp;	    break;
			case 'm':	match_pattern = fsm;	    break;
            case 'f':
            {
                fd = open(argv[optind - 1],O_RDONLY);
                if(fd == -1)
                {
                    fprintf(stderr,"FILE DOESN'T EXIST!!!\n");
                    return 1;
                }

                fb = file_buffer_create(fd,PROT_READ);
				close(fd);

                text = fb->buffer;
                break;
            }
		}
	}

	if(text == NULL || pattern == NULL)
	{
		help();
		return 0;
	}

	match_pattern(text,pattern);

	file_buffer_destroy(fb);

	return 0;
}
