#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define	MAX_LENGTH (1024 / sizeof (char))
#define	UNKNOWN_COMMAND_ERROR "Unknown command"
#define	INVALID_COMMAND_SUFFIX_ERROR "Invalid command suffix"
#define	WRONG_NUMBER_OF_PARAMS_ERROR "wrong number of params"
#define	INVALID_ADDRESS_ERROR "Invalid address"
#define	UNEXPECTED_ADDRESS_ERROR "Unexpected address"
#define	CANNOT_OPEN_INPUT_FILE_ERROR "Cannot open input file"
#define	OUT_OF_MEMMORY_ERROR "Out of memory"

// string representation
typedef struct lines {
	char *str;
	struct lines *prev;
	struct lines *next;
} line;

typedef struct commands {
	char *name;
	int vars;
	int first;
	int second;
} command;

typedef enum { yes = 1, no = 0 } boolean;



static int n = 1;
static int lines = 0;
static size_t size = 0;
static line *currentLine;
static char *lastError;
static boolean printErrors = no;
static boolean quit = no;
static char *buffer;
static int err = 0;




char * parseFile(FILE *fp);
void classifyOption(char c);
void classifyStringOption(char *string);
void printHelp(void);
void printVersion(void);
void printLines(int from, int to, boolean numbers);
void printLine(int num, boolean numbers);
boolean addressValidity(int address);
boolean addressesValidity(int from, int to);
void getCommand(command *result);
char * executeCommand(command cmd);
size_t readline(char **lineptr, size_t *len, FILE *stream);
int charToInt(char c);
boolean getNumber(char *string, int *number);
void freeLines(void);



int
main(int argc, char **argv) {
	char *error = NULL;


	// parsing options
	int i;
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (argv[i][1] == '-')
		{
			classifyStringOption(argv[i]);
		} else {
			for (int j = 1; argv[i][j] != '\0'; j++) {
				classifyOption(argv[i][j]);
			}
		}
	}


	// setting up the first line
	currentLine = malloc(sizeof (line));
	(*currentLine) = (line) {
		.str = malloc(MAX_LENGTH),
		.prev = NULL,
		.next = NULL
	};
	(*currentLine).str[0] = '\0';

	// parsing the file
	FILE *fp;

	if ((fp = fopen(argv[i], "r")) == NULL) {
		lastError = CANNOT_OPEN_INPUT_FILE_ERROR;
		fprintf(stderr, "%s: %s\n", argv[i], strerror(errno));
		fflush(stderr);
		n = -1;
	} else {
		error = parseFile(fp);

		//	unexpected error
		if (error != NULL) {
			fprintf(stderr, "%s\n", error);
			fflush(stderr);
			freeLines();
			fclose(fp);
			exit(1);
		} else {
			fprintf(stderr, "%zu\n", size);
			fflush(stderr);
		}

		fclose(fp);
	}



	// setting up for reading the commands
	// size of a command doesnt have to be very big
	size_t sizeMax = MAX_LENGTH;
	buffer = (char *) malloc(sizeMax);
	command cmd = {
		.name = (char *) malloc(sizeMax),
		.vars = 0,
		.first = 0,
		.second = 0
	};

	readline(&buffer, &sizeMax, stdin);

	while (!feof(stdin)) {
		getCommand(&cmd);

		error = executeCommand(cmd);
		cmd.vars = 0;
		cmd.first = 0;
		cmd.second = 0;

		if (error != NULL) {
			fprintf(stderr, "?\n");
			err = 1;
			if (printErrors)
				fprintf(stderr, "%s\n", error);
			lastError = error;
			fflush(stderr);
		}

		if (quit)
			break;

		readline(&buffer, &sizeMax, stdin);
	}

	free(buffer);
	free(cmd.name);
	freeLines();
	exit(err);
}



void
freeLines(void) {
	// getting to the end
	while ((*currentLine).next != NULL) {
		currentLine = (*currentLine).next;
	}


	// freeing up the space from the end
	line *prev;
	while (currentLine != NULL) {
		prev = (*currentLine).prev;
		free((*currentLine).str);
		free(currentLine);
		currentLine = prev;
	}
}

void
classifyOption(char c) {
	switch (c) {
				case 'h':
					printHelp();
					exit(0);
					break;
				case 'V':
					printVersion();
					exit(0);
					break;
				case 'G':
					break;
				case 'l':
					break;
				case 'p':
					break;
				case 'r':
					break;
				case 's':
					break;
				case 'v':
					break;
				default:
					fprintf(stderr, "ed: illegal option -- %c\n", c);
					fprintf(stderr, "usage: ed file\n");
					fflush(stderr);
					exit(1);
					break;
			}
}


void
classifyStringOption(char *string) {
	if (strcmp(string, "--help") == 0) {
		printHelp();
		exit(0);
	} else if (strcmp(string, "--version") == 0) {
		printVersion();
		exit(0);
	} else if (strcmp(string, "--traditional") == 0) {

	} else if (strcmp(string, "--loose-exit-status") == 0) {

	} else if (strcmp(string, "--prompt=") == 0) {

	} else if (strcmp(string, "--restricted") == 0) {

	} else if (strcmp(string, "--quite") == 0 ||
				strcmp(string, "--silent") == 0) {

	} else if (strcmp(string, "--verbose") == 0) {

	} else {
		fprintf(stderr, "ed: illegal option -- %s\n", string);
		fprintf(stderr, "usage: ed file\n");
		fflush(stderr);
		exit(1);
	}
}


char *
parseFile(FILE *fp) {
	char c;
	size_t i = 0;


	while ((c = fgetc(fp)) != EOF) {
		size += sizeof (char);

		if (i >= (MAX_LENGTH - 1)) {
			return (no);
		}

		if (c == '\n') {
			// null terminating previous string
			(*currentLine).str[i] = '\0';

			// getting memmory for the new line and its string
			line *nextLine;

			if ((nextLine = (line *) malloc(sizeof (line))) == NULL)
				return (OUT_OF_MEMMORY_ERROR);

			char *string;
			if ((string = (char *) malloc(MAX_LENGTH)) == NULL) {
				free(nextLine);
				return (OUT_OF_MEMMORY_ERROR);
			}


			// starting string is ""
			string[0] = '\0';

			(*nextLine) = (line) {
				.str = string,
				.prev = currentLine,
				.next = NULL
			};

			// moving forward to next line
			(*currentLine).next = nextLine;
			currentLine = nextLine;

			n++;
			i = 0;
		} else {
			// just appending characters to a line
			(*currentLine).str[i] = c;
			i++;
		}
	}

	if (strcmp((*currentLine).str, "") == 0) {
		free((*currentLine).str);
		currentLine = (*currentLine).prev;
		free((*currentLine).next);
		(*currentLine).next = NULL;
		n -= 1;
	}

	lines = n;

	return (NULL);
}

void
printLine(int address, boolean numbers) {
	while (n < address) {
		currentLine = (*currentLine).next;
		n++;
	}

	while (n > address) {
		currentLine = (*currentLine).prev;
		n--;
	}


	if (numbers)
		printf("%d\t%s\n", n, (*currentLine).str);
	else
		printf("%s\n", (*currentLine).str);
	fflush(stdout);
}

void
printLines(int from, int to, boolean numbers) {
	// getting to first position
	while (n < from) {
		currentLine = (*currentLine).next;
		n++;
	}

	while (n > from) {
		currentLine = (*currentLine).prev;
		n--;
	}

	// ptinting lines
	while (n < to) {
		if (numbers)
			printf("%d\t%s\n", n, (*currentLine).str);
		else
			printf("%s\n", (*currentLine).str);
		fflush(stdout);

		n++;
		currentLine = (*currentLine).next;
	}

	if (numbers)
		printf("%d\t%s\n", n, (*currentLine).str);
	else
		printf("%s\n", (*currentLine).str);
	fflush(stdout);
}



size_t
readline(char **lineptr, size_t *len, FILE *stream) {
	memset((*lineptr), '\0', sizeof (char) * 100);

	ssize_t chars = getline(lineptr, len, stream);

	if ((*lineptr)[chars - 1] == '\n') {
		(*lineptr)[chars - 1] = '\0';
		--chars;
	}


	return (chars);
}

boolean
getNumber(char *string, int *number) {
	int i = 0;
	*number = 0;
	boolean negative = no;
	boolean positive = no;
	boolean readSomething = no;


	// This first if statement will ensure that " 1"
	// will give invalid address instead of unknown command
	// since address is always checked first.
	// Checking the second char ensures, that " p" will give
	// unknown command and not unknown address.
	if (string[0] == ' ' && string[1] >= '0' && string[1] <= '9') {
		*number = -1;
		return (yes);
	}

	if (string[0] == '.') {
		*number =  n;
		strcpy(string, string + 1);
		return (yes);
	}

	if (string[0] == '$') {
		*number = lines;
		strcpy(string, string + 1);
		return (yes);
	}


	if (string[0] == '-') {
		negative = yes;
		i++;
	}


	if (string[0] == '+') {
		positive = yes;
		i++;
	}


	while (string[i] >= '0' && string[i] <= '9') {
		(*number) = (*number) * 10 + (string[i] - '0');
		i++;
		readSomething = yes;
	}

	if (positive) {
		if (readSomething)
			*number = n + (*number);
		else
			i = 0;
	}

	if (negative) {
		if (readSomething)
			*number = n - (*number);
		else
			i = 0;
	}

	strcpy(string, string + i);



	return (readSomething);
}

void
getCommand(command *result) {
	char *string = malloc(MAX_LENGTH);
	strcpy(string, buffer);

	// checking for comma -> checking propper format / reading numbers
	if (strchr(string, ',') != NULL) {
		if (!getNumber(string, &((*result).first))) {
			(*result).vars = -1;
			return;
		}

		if (string[0] != ',') {
			(*result).vars = -1;
			return;
		}

		string += 1;

		if (!getNumber(string, &((*result).second))) {
			(*result).vars = -1;
			return;
		}

		(*result).vars = 2;

	} else {

		if (!getNumber(string, &((*result).first))) {
			(*result).vars = 0;
		} else {
			(*result).vars = 1;
		}
	}

	strcpy((*result).name, string);
}


boolean
addressValidity(int address) {
	if (address < 1 || address > lines)
		return (no);

	return (yes);
}

boolean
addressesValidity(int from, int to) {
	if (from < 1 || from > lines)
		return (no);
	if (to < 1 || to > lines)
		return (no);
	if (from > to)
		return (no);

	return (yes);
}


char *
executeCommand(command cmd) {
	if (cmd.vars == -1)
		return (INVALID_ADDRESS_ERROR);

	if (cmd.vars == 2) {
		if (!addressesValidity(cmd.first, cmd.second))
			return (INVALID_ADDRESS_ERROR);
	}

	if (cmd.vars == 1) {
		if (!addressValidity(cmd.first))
			return (INVALID_ADDRESS_ERROR);
	}


	if (strcmp(cmd.name, "") == 0) {
		if (cmd.vars == 2)
			printLine(cmd.second, no);

		if (cmd.vars == 1)
			printLine(cmd.first, no);


		if (cmd.vars == 0) {
			if (!addressValidity(n + 1))
				return (INVALID_ADDRESS_ERROR);
			printLine(n + 1, no);
		}

		return (NULL);
	}


	if (cmd.name[0] == 'p') {

		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLines(cmd.first, cmd.second, no);
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLine(cmd.first, no);
		}

		if (cmd.vars == 0) {
			if (!addressValidity(n))
				return (INVALID_ADDRESS_ERROR);

			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);
			printLine(n, no);

		}

		return (NULL);
	}

	if (cmd.name[0] == 'n') {

		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLines(cmd.first, cmd.second, yes);
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLine(cmd.first, yes);
		}

		if (cmd.vars == 0) {
			if (!addressValidity(n))
				return (INVALID_ADDRESS_ERROR);

			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);
			printLine(n, yes);
		}


		return (NULL);

	}

	if (cmd.name[0] == 'H') {
		if (cmd.vars != 0)
			return (UNEXPECTED_ADDRESS_ERROR);

		if (strcmp(cmd.name, "H") != 0)
			return (INVALID_COMMAND_SUFFIX_ERROR);

		if (lastError != NULL) {
			fprintf(stderr, "%s\n", lastError);
			fflush(stderr);
		}

		if (printErrors)
			printErrors = no;
		else
			printErrors = yes;

		return (NULL);

	}

	if (cmd.name[0] == 'h') {
		if (cmd.vars != 0)
			return (UNEXPECTED_ADDRESS_ERROR);

		if (strcmp(cmd.name, "h") != 0)
			return (INVALID_COMMAND_SUFFIX_ERROR);

		if (lastError != NULL) {
			fprintf(stderr, "%s\n", lastError);
			fflush(stderr);
		}

		return (NULL);
	}

	if (cmd.name[0] == 'q') {
		if (cmd.vars != 0)
			return (UNEXPECTED_ADDRESS_ERROR);

		if (strcmp(cmd.name, "q") != 0)
			return (INVALID_COMMAND_SUFFIX_ERROR);

		quit = yes;
		return (NULL);
	}

	return (UNKNOWN_COMMAND_ERROR);
}

void
printHelp(void) {
	printf("	Filip's attempt at implementation of GNU Ed - The GNU line editor.\n");
	printf("Please note, that this is only my attempt at implementation,\n");
	printf("not the GNU software itself.\n");
	printf("\n");
	printf("Usage: ed [options] [file]\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help                 display this help and exit\n");
	printf("  -V, --version              output version information and exit\n");
	printf("  -G, --traditional          run in compatibility mode\n");
	printf("  -l, --loose-exit-status    exit with 0 status even if a command fails\n");
	printf("  -p, --prompt=STRING        use STRING as an interactive prompt\n");
	printf("  -r, --restricted           run in restricted mode\n");
	printf("  -s, --quiet, --silent      suppress diagnostics\n");
	printf("  -v, --verbose              be verbose\n");
	printf("Start edit by reading in 'file' if given.\n");
	printf("If 'file' begins with a '!', read output of shell command.\n");
	printf("\n");
	printf("Exit status: 0 for a normal exit, 1 for environmental problems (file\n");
	printf("not found, invalid flags, I/O errors, etc), 2 to indicate a corrupt or\n");
	printf("invalid input file, 3 for an internal consistency error (eg, bug) which\n");
	printf("caused ed to panic.\n");
	printf("\n");
	printf("If you find any bugs, please fix them. :D\n");
	printf("Ed home page: http://www.gnu.org/software/ed/ed.html\n");
	printf("General help using GNU software: http://www.gnu.org/gethelp\n");
}

void
printVersion(void) {
	printf("Filip\n");
	printf("No licence.\n");
	printf("Do whatever you want with this code.\n");
}