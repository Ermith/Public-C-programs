#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

// v 37

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
#define	NO_CURRENT_FILE_NAME_ERROR "No current filename"
#define	LINE_TOO_LONG_ERROR "Line too long"

// string representation
typedef struct line {
	char *str;
	struct line *prev;
	struct line *next;
} line_t;

typedef struct command {
	char *name;
	int vars;
	int first;
	int second;
} command_t;

static int n = 0;
static int lines = 0;
static size_t size = 0;
static line_t *currentLine;
static char *lastError;
static bool printErrors = false;
static bool quit = false;
static bool modified = false;
static bool lastQ = false;
static char *buffer;
static int err = 0;
static char *fileName = NULL;
// static int all = 0;

char * parseFile(FILE *fp);
void classifyOption(char c);
void classifyStringOption(char *string);
void printHelp(void);
void printVersion(void);
void printLines(int from, int to, bool numbers);
void printLine(int num, bool numbers);
bool addressValidity(int address);
bool addressesValidity(int from, int to);
bool zero_address_check(command_t cmd);
bool getCommand(command_t *result);
char * executeCommand(command_t cmd);
size_t readline(char *line_t, size_t len);
int charToInt(char c);
int getNumber(char *string, int *number, int position);
void freeLines(void);
void unknownOptionC(char c);
void unknownOptionS(char *string);
void moveTo(int i);
bool readLines(line_t **firstLine);
void deleteLine(int address);
void deleteLines(int from, int to);

int
main(int argc, char **argv) {
	char *error = NULL;

// printf("beginning\n");

	// parsing options
	int i;
	for (i = 1; i < argc && argv[i][0] == '-'; i++) {
		if (argv[i][1] == '-') {
			classifyStringOption(argv[i]);
		} else {
			for (int j = 1; argv[i][j] != '\0'; j++) {
				classifyOption(argv[i][j]);
			}
		}
	}

	currentLine = NULL;

// printf("checking fileName\n");

	// check whether the file name is given
	if (i != argc) {

		// setting up the first line

		if ((currentLine = (line_t *) malloc(sizeof (line_t))) == NULL) {
			fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
			exit(1);
		}

		if (((*currentLine).str = (char *) malloc(MAX_LENGTH)) == NULL) {
			free(currentLine);
			fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
			exit(1);
		}

		n = 1;
		(*currentLine).prev = NULL;
		(*currentLine).next = NULL;
		(*currentLine).str[0] = '\0';
// all += 2;
// printf("%d\n", all);
// printf("opening file\n");
		// parsing the file
		fileName = argv[i];
		FILE *fp;

		if ((fp = fopen(fileName, "r")) == NULL) {
			lastError = CANNOT_OPEN_INPUT_FILE_ERROR;
			fprintf(stderr, "%s: %s\n", fileName, strerror(errno));
			fflush(stderr);
			n = -1;
		} else {
// printf("parsing file\n");
			error = parseFile(fp);
// printf("file parsed\n");
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

// printf("file closed\n");

	// setting up for reading the command
	if ((buffer = (char *) malloc(MAX_LENGTH)) == NULL) {

// printf("here\n");
		freeLines();
		fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
		exit(1);
	}

// printf("first\n");

	command_t cmd = {
		.name = (char *) malloc(MAX_LENGTH),
		.vars = 0,
		.first = 0,
		.second = 0
	};

// printf("second\n");

	if (cmd.name == NULL) {
		freeLines();
		free(buffer);
		fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
		exit(1);
	}

// printf("here\n");
// all += 2;
// printf("%d\n", all);

	readline(buffer, MAX_LENGTH);

	while (!feof(stdin)) {
// printf("line read: %s\n\n", buffer);
		if (!getCommand(&cmd)) {
			fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
			err = 1;
			goto freeMem;
		}

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

		readline(buffer, MAX_LENGTH);
	}
// printf("quiting\n");
// printf("address of a buffer: %p\n", &buffer);
freeMem:
	free(buffer);
// all--;
// printf("%d\n", all);
// printf("%s\n", cmd.name);
// printf("free(cmd.name)\n");
// printf("address of cmd.name: %p\n", &(cmd.name));
	free(cmd.name);
// all--;
// printf("%d\n", all);
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
	line_t *prev;
	while (currentLine != NULL) {
		prev = (*currentLine).prev;
		free((*currentLine).str);
		free(currentLine);
		currentLine = prev;
// all -= 2;
// printf("%d\n", all);
	}
}

void
unknownOptionC(char c) {
	fprintf(stderr, "ed: illegal option -- %c\n", c);
	fprintf(stderr, "usage: ed [file]\n");
	fflush(stderr);
	exit(1);
}

void
unknownOptionS(char *string) {
		fprintf(stderr, "ed: illegal option -- %s\n", string);
		fprintf(stderr, "usage: ed [file]\n");
		fflush(stderr);
		exit(1);
}

void
classifyOption(char c) {
	// since we don't reckognize any options,
	// we exit right in the beginning
	unknownOptionC(c);

/*
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
	*/
}

void
classifyStringOption(char *string) {
	// same here as in "classifyOption"
	unknownOptionS(string + 2);

/*
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
	*/
}

char *
parseFile(FILE *fp) {
	char c;
	size_t i = 0;

// printf("up to read the first char\n");
	while ((c = fgetc(fp)) != EOF) {
		size += sizeof (char);

// printf("read\n");
// printf("%c\n", c);

		if (i >= (MAX_LENGTH - 1)) {
			return (LINE_TOO_LONG_ERROR);
		}


		if (c == '\n') {
// printf("reading line\n");
			// null terminating previous string
			(*currentLine).str[i] = '\0';

			// getting memmory for the new line and its string
			line_t *nextLine;

			if ((nextLine = (line_t *) malloc(sizeof (line_t))) == NULL)
				return (OUT_OF_MEMMORY_ERROR);

// all++;
// printf("%d\n", all);

			char *string;
			if ((string = (char *) malloc(MAX_LENGTH)) == NULL) {
				free(nextLine);
// all--;
// printf("%d\n", all);
				return (OUT_OF_MEMMORY_ERROR);
			}
// all++;
// printf("%d\n", all);

			// starting string is ""
			string[0] = '\0';

			(*nextLine) = (line_t) {
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

// printf("check\n");

	if (strcmp((*currentLine).str, "") == 0) {
		free((*currentLine).str);
		currentLine = (*currentLine).prev;
		free((*currentLine).next);
		(*currentLine).next = NULL;
		n -= 1;
// all -= 2;
// printf("%d\n", all);
	}
// printf("ending\n");
	lines = n;

	return (NULL);
}

void
printLine(int address, bool numbers) {
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
printLines(int from, int to, bool numbers) {
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
	bool negative = false;
	bool positive = false;
	bool readSomething = false;

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
// printf("%d\n", n);
// printf("%d\n", lines);
		return (i + 1);
	}

	if (string[i] == '$') {
		*number = lines;
		return (i + 1);
	}

	if (string[i] == '-') {
		negative = true;
		i++;
	}

	if (string[i] == '+') {
		positive = true;
		i++;
	}

	while (string[i] >= '0' && string[i] <= '9') {
// printf(" looking at digit: %c\n", string[i]);
		(*number) = (*number) * 10 + (string[i] - '0');
		i++;
		readSomething = true;
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

bool
getCommand(command_t *result) {
// printf("parsing command, string: %s\n\n", buffer);
	int position = 0;
	int num = 0;
	char *string = (char *) malloc(MAX_LENGTH);

	if (string == NULL) {
		fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
		err = 1;
		return (false);
	}

// all++;
// printf("%d\n", all);
	strcpy(string, buffer);
// printf("copied string: %s\n", string);

	// checking for comma -> checking propper format / reading numbers
	if (strchr(string, ',') != NULL) {
// printf("we found the comma in %s\n", string);
		position = getNumber(string, &((*result).first), position);
		if (position == -1) {
			(*result).vars = -1;
			return (true);
		}

		if (string[position] != ',') {
			(*result).vars = -1;
			return (true);
		}

// printf("comma is next, moving one char right\n");
		position = getNumber(string, &((*result).second), position + 1);
		if (position == -1) {
			(*result).vars = -1;
			return (true);
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
// all--;
// printf("%d\n", all);
	return (true);
}

bool
addressValidity(int address) {
	if (address < 1 || address > lines)
		return (false);

	return (true);
}

bool
addressesValidity(int from, int to) {
	if (from < 1 || from > lines)
		return (false);
	if (to < 1 || to > lines)
		return (false);
	if (from > to)
		return (false);

	return (true);
}

bool
readLines(line_t **firstLine) {
	line_t *fLine = NULL;
	line_t *myLine;
	char *string;
	string = (char *) malloc(MAX_LENGTH);

	if (string == NULL) {
		fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
		err = 1;
		return (false);
	}

// all++;
// printf("%d\n", all);

	readline(string, MAX_LENGTH);
// printf("first line read: %s\n", string);

	while (strcmp(string, ".") != 0) {
		if (fLine == NULL) {
			fLine = (line_t *) malloc(sizeof (line_t));

			if (fLine == NULL) {
				free(string);
				fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
				err = 1;
				return (false);
			}

			(*fLine) = (line_t) {
				.str = (char *) malloc(MAX_LENGTH),
				.prev = NULL,
				.next = NULL
			};

			if ((*fLine).str == NULL) {
				free(fLine);
				free(string);
				fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
				err = 1;
				return (false);
			}
// all += 2;
// printf("%d\n", all);

			strcpy((*fLine).str, string);
			myLine = fLine;
		} else {
			line_t *tmp;
			tmp = (line_t *) malloc(sizeof (line_t));

			if (tmp == NULL) {
				while (fLine != NULL) {
					tmp = fLine;
					fLine = (*fLine).next;
					free((*tmp).str);
					free(tmp);
				}

				fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
				err = 1;
				return (false);
			}

			(*tmp) = (line_t) {
				.str = (char *) malloc(MAX_LENGTH),
				.prev = myLine,
				.next = NULL
			};

			if ((*tmp).str == NULL) {
				free(tmp);

				while (fLine != NULL) {
					tmp = fLine;
					fLine = (*fLine).next;
					free((*tmp).str);
					free(tmp);
				}

				fprintf(stderr, "%s\n", OUT_OF_MEMMORY_ERROR);
				err = 1;
				return (false);
			}

// all += 2;
// printf("%d\n", all);

			(*myLine).next = tmp;
			strcpy((*tmp).str, string);
			myLine = tmp;
		}

		readline(string, MAX_LENGTH);
// printf(" another one: %s\n", string);
	}

	free(string);
	(*firstLine) = fLine;
// all--;
// printf("%d\n", all);
	return (true);
}

bool
insertCMD(int i) {
// printf("inserting\n");
// printf("n: %d\n", n);
// printf("lines: %d\n", lines);
	line_t *firstLine = NULL;
	line_t *prevLine;
	moveTo(i);
	if (!readLines(&firstLine))
		return (false);

	if (currentLine != NULL)
		prevLine = (*currentLine).prev;
	else {
		prevLine = NULL;
		n++;
	}

/*	printf("lines read:");
	line *tmpLine;
	tmpLine = firstLine;
	while (tmpLine != NULL) {
		printf("%s\n", (*tmpLine).str);
		tmpLine = (*tmpLine).next;
	} */

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

	return (true);
}

bool
zero_address_check(command_t cmd) {
	if (cmd.first < 0 || cmd.first > lines)
		return (false);

	if (cmd.vars == 2) {
		if (cmd.second < 0 || cmd.second > lines)
			return (false);
	}

	return (true);
}

void
deleteLine(int address) {
	moveTo(address);

	line_t *prevLine;
	line_t *nextLine;
	line_t *tmp;

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
// all -= 2;
// printf("%d\n", all);
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
	line_t *nextLine;
	line_t *prevLine;
	line_t *tmp;
// printf("here\n");
	prevLine = (*currentLine).prev;

// printf("previous line: %s\n", (*prevLine).str);

	while (i != to) {
// printf("freeing line: %s\n", (*currentLine).str);
		nextLine = (*currentLine).next;

		if (currentLine != NULL) {
			free((*currentLine).str);
			free(currentLine);
// all -= 2;
// printf("%d\n", all);
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
// all -= 2;
// printf("%d\n", all);
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
	line_t *tmp;
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
executeCommand(command_t cmd) {
// printf("executing command %s\n", cmd.name);
	if (cmd.vars == -1)
		return (INVALID_ADDRESS_ERROR);

	if (cmd.name[0] == 'i') {
		if (!zero_address_check(cmd))
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
				fprintf(
					stderr, "%s: %s\n",
					cmd.name + 2,
					strerror(errno));

				fflush(stderr);
				return (CANNOT_OPEN_OUTPUT_FILE_ERROR);
			}

		} else {
			if (fileName == NULL)
				return (NO_CURRENT_FILE_NAME_ERROR);

			if ((fp = fopen(fileName, "w")) == NULL) {
				fprintf(
					stderr,
					"%s: %s\n",
					fileName,
					strerror(errno));

				fflush(stderr);
				return (CANNOT_OPEN_OUTPUT_FILE_ERROR);
			}
		}

		writeLines(fp);
		fclose(fp);
		modified = false;
		return (NULL);
	}

	if (strcmp(cmd.name, "") == 0) {
		if (cmd.vars == 2)
			printLine(cmd.second, false);

		if (cmd.vars == 1)
			printLine(cmd.first, false);

		if (cmd.vars == 0) {
			if (!addressValidity(n + 1))
				return (INVALID_ADDRESS_ERROR);
			printLine(n + 1, false);
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
		modified = true;
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

		if (!insertCMD(l))
			return (OUT_OF_MEMMORY_ERROR);

		modified = true;
		return (NULL);
	}

	if (cmd.name[0] == 'p') {

		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLines(cmd.first, cmd.second, false);
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLine(cmd.first, false);
		}

		if (cmd.vars == 0) {
			if (!addressValidity(n))
				return (INVALID_ADDRESS_ERROR);

			if (strcmp(cmd.name, "p") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);
			printLine(n, false);
		}

		return (NULL);
	}

	if (cmd.name[0] == 'n') {

		if (cmd.vars == 2) {
			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLines(cmd.first, cmd.second, true);
		}

		if (cmd.vars == 1) {
			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);

			printLine(cmd.first, true);
		}

		if (cmd.vars == 0) {
			if (!addressValidity(n))
				return (INVALID_ADDRESS_ERROR);

			if (strcmp(cmd.name, "n") != 0)
				return (INVALID_COMMAND_SUFFIX_ERROR);
			printLine(n, true);
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
			printErrors = false;
		else
			printErrors = true;

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

		quit = true;
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
