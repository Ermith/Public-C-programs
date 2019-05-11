#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <conio.h>

#define HEIGHT 30
#define WIDTH 60
#define MAXLENGTH 1000
#define HEAD 'O'
#define BODY '#'
#define BOARDERS '.'
#define DEATH 'x'
#define FOOD '@'


char Map[WIDTH][HEIGHT];
COORD Snake[MAXLENGTH];
int SnakeLength;
int Head;
COORD Direction;
int var;
int Game;
COORD krtkuvDort;
COORD murderTiles[15];


void newGame(void);
void init(void);
void move(void);
void setDirection(void);
int getInput(void);
void moveCursor(int x, int Y);
void showCursor(int i);
void eat(char c);
COORD getRandomCoord(void);
int modulo(int a, int b);
void drawMap(void);
void printMap(void);





int
main(void) {
	char c = 'y';
	while(c == 'y') {
		newGame();

		moveCursor(0, HEIGHT + 1);
		printf("Game Over\n");
		showCursor(1);
		printf("Restart? y/n\n");

		c = tolower(_getch());

		while (c != 'y' && c != 'n') {
			printf("Invalid response\n");
			printf("Restart? y/n \n");
			c = tolower(_getch());
		}
	}
	system("cls");
}

void
showCursor(int i) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO     cursorInfo;
	GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = i; // set the cursor visibility
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

int
modulo(int a, int b) {
	int result = a % b;
	if (result < 0)
		result += b;
	return result;
}

COORD
getRandomCoord(void) {
	COORD pos;
	pos.X = modulo(rand(), WIDTH - 1);
	pos.Y = modulo(rand(), HEIGHT - 1);
	return pos;
	}

void
drawMap(void) {
	//fills the map
	for (int i = 0; i < HEIGHT; ++i) {
		for (int j = 0; j < WIDTH; ++j) {
			if (i == HEIGHT - 1) {
				Map[j][i] = BOARDERS;
			} else 
			if (j == WIDTH - 1) {
				Map[j][i] = BOARDERS;
			} else {
				Map[j][i] = ' ';
			}			
		}
	}

	//adds the snake
	int j;
	int x;
	int y;
	for (int i = 0; i < SnakeLength; ++i) {
		j = modulo(i + Head, MAXLENGTH);
		x = Snake[j].X;
		y = Snake[j].Y;

		Map[x][y] = BODY;
	}

	//adds murderTiles
	for (int i = 0; i < 15; ++i) {
		x = murderTiles[i].X;
		y = murderTiles[i].Y;
		Map[x][y] = DEATH;
	}

	//snake head
	x = Snake[Head].X;
	y = Snake[Head].Y;
	Map[x][y] = HEAD;

	//adds the krtkuvDort
	Map[krtkuvDort.X][krtkuvDort.Y] = FOOD;
}

void
printMap(void) {
	for (int i = 0; i < HEIGHT; ++i) {
		for (int j = 0; j < WIDTH; ++j) {
			printf("%c", Map[j][i]);
		}
		printf("\n");
	}
}

void
init(void) {
	Game = 1;
	SnakeLength = 5;
	Head = 15;
	srand(time(NULL));

	//sets up random obstacles
	for (int i = 0; i < 15; ++i) {
		murderTiles[i] = getRandomCoord();
	}

	krtkuvDort = getRandomCoord();

	//sets initial direction : right
	Direction.X = 1;
	Direction.Y = 0;

	//initializes the snake
	int x;
	for (int i = 0; i < SnakeLength; ++i) {
		x = i + modulo(Head, MAXLENGTH);
		Snake[x].X = WIDTH / 2;
		Snake[x].Y = HEIGHT / 2;
	}

	drawMap();

	//clears the console for windows
	system("cls");
	showCursor(0);

	//sets console dimensions
	system("MODE 60,32");
	printMap();
}


void
moveCursor(int x, int Y) {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD Position;
	Position.X = x;
	Position.Y = Y;
	SetConsoleCursorPosition(hOut, Position);
}



int
getInput(void) {
	if(kbhit())
		return _getch();
	else
		return -1;
}

void
setDirection(void) {
	var = getInput();
	var = tolower(var);

	switch (var) {
		case 'w': //UP
			Direction.X = 0;
			Direction.Y = -1;
			break;
		case 'a': //LEFT
			Direction.X = -1;
			Direction.Y = 0;
			break;
		case 's': //DOWN
			Direction.X = 0;
			Direction.Y = 1;
			break;
		case 'd': //RIGHT
			Direction.X = 1;
			Direction.Y = 0;
			break;
		default:
			break;
	}
}



void
eat(char c) {
	switch (c) {
		case BODY:
			Game = 0;
			break;
		case DEATH:
			Game = 0;
			break;
		case FOOD:
			SnakeLength++;
			//replaces food when eaten
			krtkuvDort = getRandomCoord();
			while (Map[krtkuvDort.X][krtkuvDort.Y] != ' ')			
				krtkuvDort = getRandomCoord();
			Map[krtkuvDort.X][krtkuvDort.Y] = FOOD;
			moveCursor(krtkuvDort.X, krtkuvDort.Y);
			printf("%c", FOOD);
			break;
		default:
			break;

	}
}

void
move(void) {
	setDirection();

	//removes last tile
	int x = Snake[modulo(Head + SnakeLength, MAXLENGTH)].X;
	int y = Snake[modulo(Head + SnakeLength, MAXLENGTH)].Y;
	Map[x][y] = ' ';
	moveCursor(x,y);
	printf("%c", Map[x][y]);

	//replace head, because it moved
	Map[Snake[Head].X][Snake[Head].Y] = BODY;
	moveCursor(Snake[Head].X, Snake[Head].Y);
	printf("%c", BODY);

	//place head where it moved
	x = modulo((Snake[Head].X + Direction.X), WIDTH - 1);
	y = modulo((Snake[Head].Y + Direction.Y), HEIGHT - 1);
	Head = modulo((Head - 1), MAXLENGTH);
	Snake[Head].X = x;
	Snake[Head].Y = y;
	moveCursor(x,y);
	printf("%c", HEAD);

	eat(Map[x][y]);

}


void
newGame(void) {
	init();

	while (Game) {
		move();
		Sleep(60);
	}
}