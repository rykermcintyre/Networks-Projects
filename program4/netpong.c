#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define WIDTH 43
#define HEIGHT 21
#define PADLX 1
#define PADRX WIDTH - 2

// Global variables recording the state of the game
// Position of ball
int ballX, ballY;
// Movement of ball
int dx, dy;
// Position of paddles
int padLY, padRY;
// Player scores
int scoreL, scoreR;

// ncurses window
WINDOW *win;

// Function primitives
void *host(void *);
void *client(void *);

// Struct typedefs
typedef struct {
	int port;
	char *host;
} info;

/* Draw the current game state to the screen
 * ballX: X position of the ball
 * ballY: Y position of the ball
 * padLY: Y position of the left paddle
 * padRY: Y position of the right paddle
 * scoreL: Score of the left player
 * scoreR: Score of the right player
 */
void draw(int ballX, int ballY, int padLY, int padRY, int scoreL, int scoreR) {
	// Center line
	int y;
	for(y = 1; y < HEIGHT-1; y++) {
		mvwaddch(win, y, WIDTH / 2, ACS_VLINE);
	}
	// Score
	mvwprintw(win, 1, WIDTH / 2 - 3, "%2d", scoreL);
	mvwprintw(win, 1, WIDTH / 2 + 2, "%d", scoreR);
	// Ball
	mvwaddch(win, ballY, ballX, ACS_BLOCK);
	// Left paddle
	for(y = 1; y < HEIGHT - 1; y++) {
	int ch = (y >= padLY - 2 && y <= padLY + 2)? ACS_BLOCK : ' ';
		mvwaddch(win, y, PADLX, ch);
	}
	// Right paddle
	for(y = 1; y < HEIGHT - 1; y++) {
	int ch = (y >= padRY - 2 && y <= padRY + 2)? ACS_BLOCK : ' ';
		mvwaddch(win, y, PADRX, ch);
	}
	// Print the virtual window (win) to the screen
	wrefresh(win);
	// Finally erase ball for next time (allows ball to move before next refresh)
	mvwaddch(win, ballY, ballX, ' ');
}

/* Return ball and paddles to starting positions
 * Horizontal direction of the ball is randomized
 */
void reset() {
	ballX = WIDTH / 2;
	padLY = padRY = ballY = HEIGHT / 2;
	// dx is randomly either -1 or 1
	dx = (rand() % 2) * 2 - 1;
	dy = 0;
	// Draw to reset everything visually
	draw(ballX, ballY, padLY, padRY, scoreL, scoreR);
}

/* Display a message with a 3 second countdown
 * This method blocks for the duration of the countdown
 * message: The text to display during the countdown
 */
void countdown(const char *message) {
	int h = 4;
	int w = strlen(message) + 4;
	WINDOW *popup = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
	box(popup, 0, 0);
	mvwprintw(popup, 1, 2, message);
	int countdown;
	for(countdown = 3; countdown > 0; countdown--) {
		mvwprintw(popup, 2, w / 2, "%d", countdown);
		wrefresh(popup);
		sleep(1);
	}
	wclear(popup);
	wrefresh(popup);
	delwin(popup);
	padLY = padRY = HEIGHT / 2; // Wipe out any input that accumulated during the delay
}

/* Perform periodic game functions:
 * 1. Move the ball
 * 2. Detect collisions
 * 3. Detect scored points and react accordingly
 * 4. Draw updated game state to the screen
 */
void tock() {
	// Move the ball
	ballX += dx;
	ballY += dy;

	// Check for paddle collisions
	// padY is y value of closest paddle to ball
	int padY = (ballX < WIDTH / 2) ? padLY : padRY;
	// colX is x value of ball for a paddle collision
	int colX = (ballX < WIDTH / 2) ? PADLX + 1 : PADRX - 1;
	if(ballX == colX && abs(ballY - padY) <= 2) {
		// Collision detected!
		dx *= -1;
		// Determine bounce angle
		if(ballY < padY) dy = -1;
		else if(ballY > padY) dy = 1;
		else dy = 0;
	}

	// Check for top/bottom boundary collisions
	if(ballY == 1) dy = 1;
	else if(ballY == HEIGHT - 2) dy = -1;

	// Score points
	if(ballX == 0) {
		scoreR = (scoreR + 1) % 100;
	reset();
	countdown("SCORE -->");
	} else if(ballX == WIDTH - 1) {
		scoreL = (scoreL + 1) % 100;
	reset();
	countdown("<-- SCORE");
	}
	// Finally, redraw the current state
	draw(ballX, ballY, padLY, padRY, scoreL, scoreR);
}

/* Listen to keyboard input
 * Updates global pad positions
 */
void *listenInput(void *args) {
	while(1) {
		switch(getch()) {
			case KEY_UP: padRY--;
			 break;
			case KEY_DOWN: padRY++;
			 break;
			case 'w': padLY--;
			 break;
			case 's': padLY++;
			 break;
			default: break;
	}

	}
	return NULL;
}

void initNcurses() {
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	refresh();
	win = newwin(HEIGHT, WIDTH, (LINES - HEIGHT) / 2, (COLS - WIDTH) / 2);
	box(win, 0, 0);
	mvwaddch(win, 0, WIDTH / 2, ACS_TTEE);
	mvwaddch(win, HEIGHT-1, WIDTH / 2, ACS_BTEE);
}

int main(int argc, char *argv[]) {
	// Process args
	// refresh is clock rate in microseconds
	// This corresponds to the movement speed of the ball
	int refresh;
	int port;
	bool isHost;
	char *host;
	char *difficulty;;
	if(argc >= 3 && argc <= 4) {
		if (strcmp(argv[1],"--host")==0) {
			isHost=true;
			port=atoi(argv[2]);
			host = "None";
			difficulty=argv[3];
			if(strcmp(difficulty, "easy") == 0) refresh = 80000;
			else if(strcmp(difficulty, "medium") == 0) refresh = 40000;
			else if(strcmp(difficulty, "hard") == 0) refresh = 20000;
			else {
				printf("ERROR: Difficulty should be one of easy, medium, hard.\n");
				exit(1);
			}
		}
		else {
			host=argv[1];
			port=atoi(argv[2]);
		}
	}
	else {
		printf("Usage: \n./netpong --host PORT DIFFICULTY\n**or**\n./netpong HOSTNAME PORT\n");
		exit(0);
	}
	
	info var = {port, host};
	
	if (isHost) {
		pthread_t host;
		pthread_create(&host, NULL, host, (void *)&var);
	}
	else {
		pthread_t client;
		pthread_create(&client, NULL, client, (void *)&var);
	}
	
	// Set up ncurses environment
	initNcurses();

	// Set starting game state and display a countdown
	reset();
	countdown("Starting Game");

	// Listen to keyboard input in a background thread
	pthread_t pth;
	pthread_create(&pth, NULL, listenInput, NULL);

	// Main game loop executes tock() method every REFRESH microseconds
	struct timeval tv;
	while(1) {
		gettimeofday(&tv,NULL);
		unsigned long before = 1000000 * tv.tv_sec + tv.tv_usec;
		tock(); // Update game state
		gettimeofday(&tv,NULL);
		unsigned long after = 1000000 * tv.tv_sec + tv.tv_usec;
		unsigned long toSleep = refresh - (after - before);
		// toSleep can sometimes be > refresh, e.g. countdown() is called during tock()
		// In that case it's MUCH bigger because of overflow!
		if(toSleep > refresh) toSleep = refresh;
		usleep(toSleep); // Sleep exactly as much as is necessary
	}

	// Clean up
	pthread_join(pth, NULL);
	endwin();
	return 0;
}



// Host code
void *host(void *args) {
	
	// Determine port from args
	int PORT = ((info *)args)->port;
	
	// Declare variables
	struct sockaddr_in sin, client_addr;
	char buf[4096];
	int s, addr_len;
	
	// Build addr data struct
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);
	
	// Open socket
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Socket failed\n");
		exit(1);
	}
	
	// Bind
	if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
		fprintf(stderr, "Bind failed\n");
		exit(1);
	}
	
	// Determine addr len
	addr_len = sizeof(client_addr);
	
	recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
	
	FILE *fp;
	fp = fopen("log.txt", "a");
	fwrite(buf, sizeof(buf), 1, fp);
	
	
	
	
	
	// Close socket
	close(s);
}




// Client code
void *client(void *args) {
	// Determine port and host name from args
	int PORT = ((info *)args)->port;
	char *HOST = ((info *)args)->host;
	
	// Declare variables
	struct hostent *hp;
	struct sockaddr_in sin;
	int s, len, addr_len;
	
	// Translate host name
	hp = gethostbyname(HOST);
	if (!hp) {
		fprintf(stderr, "Unknown host\n");
		exit(1);
	}
	
	// Build address data struct
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(SERVER_PORT);
	
	// Open the socket
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "Socket failed\n");
		exit(1);
	}
	
	// addr len
	addr_len = sizeof(sin);
	
	char buf[4096] = "Hello";
	sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&sin, sizeof(struct sockaddr));
	
	
	
	
	
	
	
	
	
	// Close sock
	close(s);
}



















