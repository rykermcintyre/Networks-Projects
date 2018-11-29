#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define WIDTH 43
#define HEIGHT 21
#define PADLX 1
#define PADRX WIDTH - 2

// Log file
FILE *log = fopen("log.txt", "a");

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

typedef struct {
	int s;
	struct sockaddr_in sin;
} sock_sin;

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
void *listenInputHost(void *args) {
	while(1) {
		switch(getch()) {
			case KEY_UP: padLY--;
			 break;
			case KEY_DOWN: padLY++;
			 break;
			default: break;
	}

	}
	return NULL;
}
void *listenInputClient(void *args) {
	while(1) {
		switch(getch()) {
			case KEY_UP: padRY--;
			 break;
			case KEY_DOWN: padRY++;
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
	int PORT;
	bool isHost;
	char *HOST;
	char *difficulty;;
	if(argc >= 3 && argc <= 4) {
		if (strcmp(argv[1],"--host")==0) {
			isHost=true;
			PORT=atoi(argv[2]);
			HOST = "None";
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
			HOST=argv[1];
			PORT=atoi(argv[2]);
		}
	}
	else {
		printf("Usage: \n./netpong --host PORT DIFFICULTY\n**or**\n./netpong HOSTNAME PORT\n");
		exit(0);
	}
	
	// Separate threads for host or client
	info var = {PORT, HOST};
	
	if (isHost) {
		printf("Waiting for challenger\n");
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
			fprintf(log, "Socket failed\n");
			exit(1);
		}
		
		// Bind
		if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
			fprintf(log, "Bind failed\n");
			exit(1);
		}
		sock_sin var2 = {s, client_addr};
		while (1) {
			char ready[4096];
			int sz;
			if ((sz = recvfrom(s, ready, sizeof(ready), 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) != -1) {
				break;
			}
		}
		pthread_t host_thread;
		pthread_create(&host_thread, NULL, host, (void *)&var2);
	}
	else {
		pthread_t client_thread;
		pthread_create(&client_thread, NULL, client, (void *)&var);
	}
	
	// Set up ncurses environment
	initNcurses();

	// Set starting game state and display a countdown
	reset();

	// Listen to keyboard input in a background thread
	pthread_t pth;
	if (isHost) {
		pthread_create(&pth, NULL, listenInputHost, NULL);
	}
	else {
		pthread_create(&pth, NULL, listenInputClient, NULL);
	}
	
	countdown("Starting Game");

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








void *send_state_host(void *args) {
	struct sockaddr_in client_addr = ((sock_sin *)args)->sin;
	int s = ((sock_sin *)args)->s;
	int addr_len = sizeof(client_addr);
	while (1) {
		char pos[4096];
		memset(pos, 0, sizeof(pos));
		sprintf(pos, "%d", padLY);
		if (sendto(s, pos, strlen(pos), 0, (struct sockaddr *)&client_addr, sizeof(struct sockaddr)) < 0) {
			fprintf(log, "Could not send host paddle pos: %s\n", strerror(errno));
			exit(1);
		}
		usleep(1000);
	}
}

void *recv_state_host(void *args) {
	struct sockaddr_in client_addr = ((sock_sin *)args)->sin;
	int s = ((sock_sin *)args)->s;
	int addr_len = sizeof(client_addr);
	while (1) {
		char pos[4096];
		memset(pos, 0, sizeof(pos));
		int sz;
		while ((sz = recvfrom(s, pos, sizeof(pos), 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) < 0) {
		}
		padRY = atoi(pos);
	}
}

// Host code
void *host(void *args) {
	// Resolve things from args
	struct sockaddr_in client_addr = ((sock_sin *)args)->sin;
	int s = ((sock_sin *)args)->s;
	
	// Determine addr len
	int addr_len = sizeof(client_addr);
	
	// Wait for client to connect
	/*while (1) {
		char cd[4096];
		memset(cd, 0, sizeof(cd));
		sprintf(cd, "Waiting for a challenger on port %d", PORT);
		countdown(cd);
		char ready[4096];
		int sz;
		if ((sz = recvfrom(s, ready, sizeof(ready), 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) != -1) {
			break;
		}
		usleep(3000000);
	}*/
	
	// Set up threads to send and receive stats constantly
	sock_sin q = {s, client_addr};
	
	pthread_t send_state;
	pthread_create(&send_state, NULL, send_state_host, (void *)&q);
	
	pthread_t recv_state;
	pthread_create(&recv_state, NULL, recv_state_host, (void *)&q);
	
	/*
	recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
	
	FILE *fp;
	fp = fopen("log.txt", "a");
	fwrite(buf, sizeof(buf), 1, fp);
	*/
	
	
	
	
	// Close socket
	close(s);
}

void *send_state_client(void *args) {
	struct sockaddr_in sin = ((sock_sin *)args)->sin;
	int s = ((sock_sin *)args)->s;
	int addr_len = sizeof(sin);
	while (1) {
		char pos[4096];
		memset(pos, 0, sizeof(pos));
		sprintf(pos, "%d", padRY);
		if (sendto(s, pos, strlen(pos), 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0) {
			fprintf(log, "Could not send client paddle pos: %s\n", strerror(errno));
			exit(1);
		}
		usleep(1000);
	}
}

void *recv_state_client(void *args) {
	struct sockaddr_in sin = ((sock_sin *)args)->sin;
	int s = ((sock_sin *)args)->s;
	int addr_len = sizeof(sin);
	while (1) {
		char pos[4096];
		memset(pos, 0, sizeof(pos));
		int sz;
		while ((sz = recvfrom(s, pos, sizeof(pos), 0, (struct sockaddr *)&sin, (socklen_t *)&addr_len)) < 0) {
		}
		padLY = atoi(pos);
	}
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
		fprintf(log, "Unknown host\n");
		exit(1);
	}
	
	// Build address data struct
	bzero((char*)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = htons(PORT);
	
	// Open the socket
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(log, "Socket failed\n");
		exit(1);
	}
	
	// addr len
	addr_len = sizeof(sin);
	
	// Tell host client is ready
	char ready[4096] = "ready\0";
	if (sendto(s, ready, strlen(ready), 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0) {
		fprintf(log, "Could not send ready msg: %s\n", strerror(errno));
		exit(1);
	}
	
	// Set up threads to send and receive stats constantly
	sock_sin q = {s, sin};
	
	pthread_t send_state;
	pthread_create(&send_state, NULL, send_state_client, (void *)&q);
	
	pthread_t recv_state;
	pthread_create(&recv_state, NULL, recv_state_client, (void *)&q);
	/*
	char buf[4096] = "Hello\0";
	sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&sin, sizeof(struct sockaddr));
	*/
	
	
	
	
	
	
	
	
	// Close sock
	close(s);
}



















