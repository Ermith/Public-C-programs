#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define	MAX_LENGTH 1024
#define	UNKNOWN_COMMAND_ERROR "Unknown command"
#define	INVALID_COMMAND_SUFFIX_ERROR "Invalid command suffix"
#define	WRONG_NUMBER_OF_PARAMS_ERROR "wrong number of params"
#define	INVALID_ADDRESS_ERROR "Invalid address"
#define	UNEXPECTED_ADDRESS_ERROR "Unexpected address"
#define	CANNOT_OPEN_INPUT_FILE_ERROR "Cannot open input file"
#define	OUT_OF_MEMMORY_ERROR "Out of memory"
#define	MISSING_FILE_NAME_ERROR "File name is missing.\nusage: ed file"
#define	BUFFER_MODIFIED_WARNING "Warning: buffer modified"
#define	UNEXPECTED_COMMAND_SUFFIX_ERROR "Unexpected command suffix"
#define	CANNOT_OPEN_OUTPUT_FILE_ERROR "Cannot open output file"

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

static int n = 0;
static int lines = 0;
static size_t size = 0;
static line *currentLine;
static char *lastError;
static boolean printErrors = no;
static boolean quit = no;
static boolean modified = no;
static boolean lastQ = no;
static char *buffer;
static int err = 0;
static char *fileName = NULL;

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
size_t readline(char *line, size_t len);
int charToInt(char c);
int getNumber(char *string, int *number, int position);
void freeLines(void);
void unknownOptionC(char c);
void unknownOptionS(char *string);
void moveTo(int i);
line * readLines(void);
void deleteLine(int address);
void deleteLines(int from, int to);

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

	currentLine = NULL;

	if (i != argc) {

		// setting up the first line
		n = 1;
		currentLine = malloc(sizeof (line));
		(*currentLine) = (line) {
			.str = malloc(MAX_LENGTH),
			.prev = NULL,
			.next = NULL
		};
		(*currentLine).str[0] = '\0';

		// parsing the file
		fileName = argv[i];
		FILE *fp;

		if ((fp = fopen(fileName, "r")) == NULL) {
			lastError = CANNOT_OPEN_INPUT_FILE_ERROR;
			fprintf(stderr, "%s: %s\n", fileName, strerror(errno));
			fflush(stderr);
			n = -1;
		} else {
			error = parseFile(fp);

			// unexpected error
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

	readline(buffer, sizeMax);

	while (!feof(stdin)) {
// printf("line read: %s\n\n", buffer);
		getCommand(&cmd);

// printf("name:   %s\n", cmd.name);
// printf("vars:   %d\n", cmd.vars);
// printf("first:  %d\n", cmd.first);
// printf("second: %d\n\n", cmd.second);

		error = executeCommand(cmd);
// printf("command executed\n");
		cmd.vars = 0;
		cmd.first = 0;
		cmd.second = 0;
		lastQ = (strcmp(cmd.name, "q") == 0);

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

		readline(buffer, sizeMax);
	}
// printf("quiting\n");
// printf("address of a buffer: %p\n", &buffer);
	free(buffer);
// printf("%s\n", cmd.name);
// printf("free(cmd.name)\n");
// printf("address of cmd.name: %p\n", &(cmd.name));
	free(cmd.name);
// printf("freeing lines\n");
	freeLines();
	exit(err);
}

void
moveTo(int i) {
// printf("moving to line %d\n", i);
	while (i < n) {		
		currentLine = (*currentLine).prev;
		n--;
// printf(" moved to %d\n", n);
	}

	while (i > n) {
		currentLine = (*currentLine).next;
		n++;
// printf(" moved to %d\n", n);
	}
}

void
freeLines(void) {
	if (currentLine == NULL)
		return;

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
unknownOptionC(char c) {
	fprintf(stderr, "ed: illegal option -- %c\n", c);
	fprintf(stderr, "usage: ed file\n");
	fflush(stderr);
	exit(1);
}

void
unknownOptionS(char *string) {
		fprintf(stderr, "ed: illegal option -- %s\n", string);
		fprintf(stderr, "usage: ed file\n");
		fflush(stderr);
		exit(1);
}

void
classifyOption(char c) {
	// since we don't reckognize any options,
	// we exit right in the beginning
	unknownOptionC(c);

	// this will not occure
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
		unknownOptionC(c);
		break;
	}
}

void
classifyStringOption(char *string) {
	// same here as in "classifyOption"
	unknownOptionS(string);

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
		unknownOptionS(string);
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
// printf("printing line %d\n", address);
	moveTo(address);

	if (numbers)
		printf("%d\t%s\n", n, (*currentLine).str);
	else
		printf("%s\n", (*currentLine).str);
	fflush(stdout);
// printf("line printed\n");
}

void
printLines(int from, int to, boolean numbers) {
	moveTo(from);

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
readline(char *line, size_t len) {
	memset(line, '\0', len);
	char c;
	size_t i = 0;

	while ((c = getchar()) != EOF && c != '\n' && i < len - 1) {
// printf("char read: %c\n", c);
		line[i] = c;
		i++;
	}

	return (i);
}

int
getNumber(char *string, int *number, int i) {
// printf("\n\n getting number from: %s\n", string);
	*number = 0;
	boolean negative = no;
	boolean positive = no;
	boolean readSomething = no;

	// This first if statement will ensure that " 1"
	// will give invalid address instead of unknown command
	// since address is always checked first.
	// Checking the second char ensures, that " p" will give
	// unknown command and not unknown address.
	if (string[i] == ' ' && string[i + 1] >= '0' && string[i + 1] <= '9') {
		*number = -1;
		return (i);
	}

	if (string[i] == '.') {
		*number =  n;
		return (i + 1);
	}

	if (string[i] == '$') {
		*number = lines;
		return (i + 1);
	}

	if (string[i] == '-') {
		negative = yes;
		i++;
	}

	if (string[i] == '+') {
		positive = yes;
		i++;
	}

	while (string[i] >= '0' && string[i] <= '9') {
// printf(" looking at digit: %c\n", string[i]);
		(*number) = (*number) * 10 + (string[i] - '0');
		i++;
		readSomething = yes;
	}

	if (positive) {
		*number = n + (*number);
	}

	if (negative) {
		*number = n - (*number);
	}

// printf("\n");
	if (readSomething)
		return (i);
	else
		return (-1);
}

void
getCommand(command *result) {
// printf("parsing command, string: %s\n\n", buffer);
	int position = 0;
	int num = 0;
	char *string = malloc(MAX_LENGTH);
	strcpy(string, buffer);
// printf("copied string: %s\n", string);

	// checking for comma -> checking propper format / reading numbers
	if (strchr(string, ',') != NULL) {
// printf("we found the comma in %s\n", string);
		position = getNumber(string, &((*result).first), position);
		if (position == -1) {
			(*result).vars = -1;
			return;
		}

		if (string[position] != ',') {
			(*result).vars = -1;
			return;
		}

// printf("comma is next, moving one char right\n");
		position = getNumber(string, &((*result).second), position + 1);
		if (position == -1) {
			(*result).vars = -1;
			return;
		}
// printf("we found the second number: %d\n", (*result).second);
// printf("last position: %d\n", position);
		(*result).vars = 2;

	} else {
// printf("we did not found comma in %s\n", string);
		num = getNumber(string, &((*result).first), position);
		if (num == -1) {
			(*result).vars = 0;
		} else {
			position = num;
			(*result).vars = 1;
		}
	}

	strcpy((*result).name, string + position);
// printf("copy -> from: %s  to  %s\n\n", string, (*result).name);
	free(string);
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

line *
readLines(void) {
	line *firstLine;
	line *myLine;
	char *string;
	firstLine = NULL;
	string = (char *)malloc(MAX_LENGTH);

	readline(string, MAX_LENGTH);
// printf("first line read: %s\n", string);

	while (strcmp(string, ".") != 0) {
		if (firstLine == NULL) {
			firstLine = malloc(sizeof (line));

			(*firstLine) = (line) {
				.str = (char *)malloc(MAX_LENGTH),
				.prev = NULL,
				.next = NULL
			};

			strcpy((*firstLine).str, string);
			myLine = firstLine;
		} else {
			line *tmp;
			tmp = malloc(sizeof (line));
			(*tmp) = (line) {
				.str = (char *)malloc(MAX_LENGTH),
				.prev = myLine,
				.next = NULL
			};

			(*myLine).next = tmp;
			strcpy((*tmp).str, string);
			myLine = tmp;
		}

		readline(string, MAX_LENGTH);
// printf(" another one: %s\n", string);
	}

	return (firstLine);
}

void
insertCMD(int i) {
// printf("inserting\n");
// printf("n: %d\n", n);
// printf("lines: %d\n", lines);
	line *firstLine;
	line *prevLine;
	moveTo(i);
	firstLine = readLines();
	if (currentLine != NULL)
		prevLine = (*currentLine).prev;
	else {
		prevLine = NULL;
		n++;
	}

/*printf("lines read:");
line *tmpLine;
tmpLine = firstLine;
while (tmpLine != NULL) {
	printf("%s\n", (*tmpLine).str);
	tmpLine = (*tmpLine).next;
}*/

	while (firstLine != NULL) {
		if (prevLine != NULL)
			(*prevLine).next = firstLine;
		(*firstLine).prev = prevLine;
		prevLine = firstLine;
		firstLine = (*firstLine).next;
		(*prevLine).next = currentLine;
		if (currentLine != NULL)
			(*currentLine).prev = prevLine;
		lines++;
		n++;
	}

	currentLine = prevLine;
	n--;
}

boolean
fucking_piece_of_shit_bitch_ass_dumm_looking_insert_command(command *cmd) {
	if ((*cmd).first < 0 || (*cmd).first > lines)
		return (no);

	if ((*cmd).vars == 2) {
		if ((*cmd).second < 0 || (*cmd).second > lines)
			return (no);
	}

	return (yes);
}

void
deleteLine(int address) {
	moveTo(address);

	line *prevLine;
	line *nextLine;
	line *tmp;

	tmp = currentLine;
	prevLine = (*tmp).prev;
	nextLine = (*tmp).next;

// printf("deleting line\n");
// printf("previous: %s\n", (*prevLine).str);
// printf("line being deleted: %s\n", (*tmp).str);
// printf("nextLine: %s\n", (*nextLine).str);

	if (prevLine != NULL) {
		(*prevLine).next = nextLine;
		currentLine = prevLine;
	} else {
		currentLine = (*tmp).next;
	}

	if (nextLine != NULL)
		(*nextLine).prev = prevLine;
	else
		n--;

	free((*tmp).str);
	free(tmp);
	lines--;

// printf("\n");
// printf("after delete\n");
// printf("previous line: %s\n", (*(*currentLine).prev).str);
// printf("current line: %s\n", (*currentLine).str);
// printf("next line: %s\n", (*(*currentLine).next).str);
// printf("number of line: %d\n", n);
}

void
deleteLines(int from, int to) {
// printf("deleting lines\n");
	moveTo(from);
	int i = n;
	line *nextLine;
	line *prevLine;
	line *tmp;
// printf("here\n");
	prevLine = (*currentLine).prev;

// printf("previous line: %s\n", (*prevLine).str);

	while (i != to) {
// printf("freeing line: %s\n", (*currentLine).str);
		nextLine = (*currentLine).next;

		if (currentLine != NULL) {
			free((*currentLine).str);
			free(currentLine);
		}

		currentLine = nextLine;
		lines--;
		i++;
	}

	tmp = currentLine;

// printf("last line to free: %s\n", (*tmp).str);

	if ((*tmp).next != NULL) {
// printf("after that: %s\n", (*(*tmp).next).str);
// printf("after after that: %s\n", (*(*(*tmp).next).next).str);
		(*(*tmp).next).prev = prevLine;
		currentLine = (*tmp).next;
		n = from;
	} else {
		currentLine = prevLine;
		n = from - 1;
	}

	if (prevLine != NULL) {
		(*prevLine).next = (*tmp).next;
	}

	free((*tmp).str);
	free(tmp);
	lines--;
// printf("here\n");

// printf("\n");
// printf("after delete\n");
// printf("previous line: %s\n", (*(*currentLine).prev).str);
// printf("current line: %s\n", (*currentLine).str);
// printf("next line: %s\n", (*(*currentLine).next).str);
// printf("number of line: %d\n", n);
}

void
writeLines(FILE *fp) {
	line *tmp;
	tmp = currentLine;
	size_t s = 0;

	while ((*tmp).prev != NULL) {
		tmp = (*tmp).prev;
	}

	while (tmp != NULL) {
		fprintf(fp, "%s\n", (*tmp).str);
		s += strlen((*tmp).str) + 1;
		tmp = (*tmp).next;
	}

	fflush(fp);
	fprintf(stderr, "%zu\n", s);
	fflush(stderr);
}

char *
executeCommand(command cmd) {
// printf("executing command %s\n", cmd.name);
	if (cmd.vars == -1)
		return (INVALID_ADDRESS_ERROR);

	if (cmd.name[0] == 'i') {
		if(!fucking_piece_of_shit_bitch_ass_dumm_looking_insert_command(&cmd))
			return (INVALID_ADDRESS_ERROR);
	} else {
		if (cmd.vars == 2) {
			if (!addressesValidity(cmd.first, cmd.second))
				return (INVALID_ADDRESS_ERROR);
		}

		if (cmd.vars == 1) {
			if (!addressValidity(cmd.first))
				return (INVALID_ADDRESS_ERROR);
		}
	}

	if (cmd.name[0] == 'w') {
		FILE *fp;

		if (strcmp(cmd.name, "w") != 0) {
			if (cmd.name[1] != ' ')
				return (UNEXPECTED_COMMAND_SUFFIX_ERROR);

			if (fileName == NULL)
				fileName = cmd.name + 2;

			if ((fp = fopen((cmd.name + 2), "w")) == NULL) {
				fprintf(stderr, "%s: %s\n", cmd.name + 2, strerror(errno));
				fflush(stderr);
				return (CANNOT_OPEN_OUTPUT_FILE_ERROR);
			}

		} else {
			if ((fp = fopen(fileName, "w")) == NULL) {
				fprintf(stderr, "%s: %s\n", fileName, strerror(errno));
				fflush(stderr);
				return (CANNOT_OPEN_OUTPUT_FILE_ERROR);
			}
		}

		writeLines(fp);
		fclose(fp);
		modified = no;
		return (NULL);
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

	if (cmd.name[0] == 'd') {
		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "d") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			deleteLines(cmd.first, cmd.second);
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "d") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			deleteLine(cmd.first);
		}

// printf("%d\n", n);
		modified = yes;
		return (NULL);
	}

	if (cmd.name[0] == 'i') {
		int l = 1;
		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "i") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			l = cmd.second;
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "i") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			l = cmd.first;
		}

		insertCMD(l);
		modified = yes;
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

		if (modified && !lastQ)
			return (BUFFER_MODIFIED_WARNING);

		quit = yes;
		return (NULL);
	}

	return (UNKNOWN_COMMAND_ERROR);
}

void
printHelp(void) {
	printf("	Filip's attempt at implementation of GNU Ed "
		"- The GNU line editor.\n");
	printf("Please note, that this is only my "
		"attempt at implementation,\n");
	printf("not the GNU software itself.\n");
	printf("\n");
	printf("Usage: ed [options] [file]\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help                 display this help and exit\n");
	printf("  -V, --version              "
		"output version information and exit\n");
	printf("  -G, --traditional          run in compatibility mode\n");
	printf("  -l, --loose-exit-status    "
		"exit with 0 status even if a command fails\n");
	printf("  -p, --prompt=STRING        "
		"use STRING as an interactive prompt\n");
	printf("  -r, --restricted           run in restricted mode\n");
	printf("  -s, --quiet, --silent      suppress diagnostics\n");
	printf("  -v, --verbose              be verbose\n");
	printf("Start edit by reading in 'file' if given.\n");
	printf("If 'file' begins with a '!', read output of shell command.\n");
	printf("\n");
	printf("Exit status: 0 for a normal exit, "
		"1 for environmental problems (file\n");
	printf("not found, invalid flags, I/O errors, etc), "
		"2 to indicate a corrupt or\n");
	printf("invalid input file, 3 for an internal "
		"consistency error (eg, bug) which\n");
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
