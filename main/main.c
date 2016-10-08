#include <compiler.h>
#include <common.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>

/*
    MAIN FILE

    Parse argv and call compiler function

    Author: Michal Kukowski
    email: michalkukowski10@gmail.com
*/

void usage()
{
    printf( "Compiler\n\n"
            "ARGS:\n"
            "MANDATORY\n"
            "--input[-i]\t\tfile to compile\n"
            "--output[-o]\t\twrite output to file\n\n"
            "OPTIONAL:\n"
            "--Wall[-a]\t\tprint all warnings\n"
            "--Werror[-e]\t\tmake all warnings into errors\n"
            "--tokens[-t]\t\tget token list instead of asm code\n\n"
            "Examples:\n"
            "./compiler.out --input my_code --output my_code.asm\n"
            "./compiler.out --input my_code --output my_code.asm --Wall --Werror\n"
            "./compiler.out --input my_code --tokens --output mytokens\n\n");

    exit(0);
}

int main(int argc, char **argv)
{
    int ret;
    struct stat   st;

	static struct option long_option[] =
	{
		{"Wall",	no_argument,	    0,	'a'},
		{"Werror",	no_argument,	    0,	'e'},
		{"O",	    required_argument,	0,	'O'},
        {"tokens",  no_argument,        0,  't'},
        {"output",  required_argument,  0,  'o'},
        {"input",  required_argument,   0,  'i'},
		{NULL,		0,				    0,	'\0'}

    };

    int opt;

    /* need at least 2 param: -input and -output */
    if(argc < 3)
        usage();

    while ((opt = getopt_long_only(argc, argv, "aeto:i:O:",
                    long_option, NULL )) != -1)
    {
        switch(opt)
        {
            case 'a':
            {
                option.wall = 1;
                break;
            }
            case 'e':
            {
                option.werr = 1;
                break;
            }
            case 't':
            {
                option.tokens = 1;
                break;
            }
            case 'O':
            {
                option.optimal = atoi(argv[optind - 1]);
                break;
            }
            case 'o':
            {
                option.output_file = argv[optind - 1];
                break;
            }
            case 'i':
            {
                option.input_file = argv[optind - 1];
                break;
            }
            default:
            {
                usage();
            }
        }
    }

    if(option.input_file == NULL || option.output_file == NULL)
        usage();

    /* check existing of inpu file */
    if( stat (option.input_file, &st))
    {
        printf("%sERROR: %s No such file%s\n\n", RED, option.input_file, RESET);

        usage();
    }

    /* args parsed, run compiler */
    ret = main_compile();
    if(ret)
        ERROR("compiler error\n", 1, "");

    return 0;
}
