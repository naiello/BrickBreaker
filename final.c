/* BrickBreaker Game Final Project
	Kelsey Meranda & Nick Aiello */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include "gfx4.h"

#define SCREEN_SIZE 768
#define BRICK_W	48
#define BRICK_H 24
#define PADDLE_W 76
#define PADDLE_H 24
#define BALL_RADIUS 8
#define BALL_SPEED 600
#define PADDLE_SPEED 512
#define PADDLE_FRICTION 0.2
#define PADDLE_SIGHT_LEN 100
#define PADDLE_DTHETA (M_PI / 64)

#define TEXT_HEIGHT 12

#define WHITE 255, 255, 255
#define BLACK 0, 0, 0
#define COLOR_BRICK1 255, 0, 0
#define COLOR_BRICK2 0, 255, 0
#define COLOR_BRICK3 0, 0, 255
#define COLOR_SIGHT 128, 255, 255

#define KEY_LEFT 81
#define KEY_RIGHT 83
#define KEY_ESCAPE 27
#define KEY_ENTER 13

#define NLIVES 5
#define BRICK_SCORE 10

#define USTEP (10000)
#define DT (USTEP / 1000000.)

typedef enum { NORMAL, DEAD, GAMEOVER, QUIT, NEXTLVL } gamestate_t;

typedef struct Color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
} color_t;

typedef struct Vec2
{
	float x;
	float y;
} vec2_t;

typedef struct Rect
{
	int x;
	int y;
	int w;
	int h;
} rect_t;

typedef struct Ball
{
	vec2_t position;
	vec2_t velocity;
	int radius;
	int lives;
	int alive;
	color_t color;
} ball_t;

typedef struct Paddle
{
	vec2_t position;
	vec2_t velocity;
	color_t color;
	int locked;
	float launchAngle;
	int width;
} paddle_t;

typedef char board_t[SCREEN_SIZE / BRICK_H][SCREEN_SIZE / BRICK_W];

// Moves ball to center of paddle and paddle to center of screen. Loads board from file.
void startGame(ball_t*, paddle_t*, board_t);
// Displays final score screen and asks if user wants to play again
int playAgain(char*);
// Displays you win screen. Returns 1 if user wants to play again, returns 0 if user wants to quit
int win();
// Subtract 1 from lives and display game over screen if lives < 0
void kill(ball_t*);

// Loads a board from a file
int loadLevelNum(int, board_t);
int loadLevel(char*, board_t);

// Updates and redraws the game
gamestate_t gameLoop(ball_t*, paddle_t*, board_t);

// Checks if the ball collides with any bricks and deal with it as necessary
void checkBoardCollisions(ball_t*, board_t);
// Checks if ball collides with paddle and handles it appropriately
void checkPaddleCollisions(ball_t*, paddle_t*);

// Updates the position of the ball and keeps it from going off screen
void updateBall(ball_t*, float);
// Updates the motion of the paddle and keeps it from going off screen
void updatePaddle(paddle_t*, float);
// Checks if board is empty
int boardIsEmpty(board_t);
// Draws the ball
void drawBall(ball_t*);
// Draws the paddle
void drawPaddle(paddle_t*);
// Draws the board
void drawBoard(board_t);

// Handles keyboard input (left and right arrow keys to set the paddle velocity to +/- PADDLE_SPEED or zero if nothing pressed
int processInput(ball_t*, paddle_t*);

// Returns 1 if collision, 0 if no.  if 3rd argument isn't null, it is filled in with the intersection rectangle
int intersects(rect_t, rect_t, rect_t*);

// like gfx_color() but for a color_t
void setDrawColor(color_t color);
void buildColor(color_t*, unsigned char, unsigned char, unsigned char);

// For some reason C doesn't have these functions sooooooo
int signum(float);

// Get/increment score
int score(int);
void resetScore();

int main()
{
	paddle_t paddle;
	ball_t ball;
	board_t board = { 0 };
	gamestate_t state = NORMAL;
	int level = 1;

	gfx_open(SCREEN_SIZE, SCREEN_SIZE, "Brick");
	startGame(&ball, &paddle, board);
	loadLevelNum(level, board);

	ball.lives = NLIVES;

	while (state != QUIT)
	{
		usleep(USTEP);
		switch (state)
		{
			case GAMEOVER:
				if (!playAgain("GAME OVER"))
					return 0;

				// Restart the game
				ball.lives = NLIVES;
				resetScore();
				level = 1;
				loadLevelNum(level, board);

				// Fall through to case DEAD
			case DEAD:
				startGame(&ball, &paddle, board);
				// Fall through to case NORMAL
			case NORMAL:
				state = gameLoop(&ball, &paddle, board);
				break;
			case NEXTLVL:
				level++;
				if (loadLevelNum(level, board))
				{
					startGame(&ball, &paddle, board);
					state = gameLoop(&ball, &paddle, board);
				}
				else if (playAgain("YOU WIN!"))  // Play again
				{
					ball.lives = NLIVES;
					resetScore();
					level = 1;
					loadLevelNum(level, board);
					startGame(&ball, &paddle, board);
					state = gameLoop(&ball, &paddle, board);
				}
				else
					return 0;
				break;
		}
	}
}

void startGame(ball_t* ball, paddle_t* paddle, board_t board)
{
	// Init the paddle
	paddle->position.x = (SCREEN_SIZE - PADDLE_W) / 2;
	paddle->position.y = SCREEN_SIZE - PADDLE_H * 2;
	paddle->velocity.x = paddle->velocity.y = 0;
	paddle->width = PADDLE_W;
	buildColor(&(paddle->color), WHITE);

	// Init the ball
	ball->position.x = paddle->position.x + (paddle->width / 2);
	ball->position.y = paddle->position.y - BALL_RADIUS - 1;
	ball->velocity.x = 0;
	ball->velocity.y = 0;
	ball->radius = BALL_RADIUS;
	ball->alive = 1;
	buildColor(&(ball->color), WHITE);

	// Allow user to launch the ball
	paddle->locked = 1;
	paddle->launchAngle = M_PI / 2;
}

int loadLevelNum(int lvl, board_t board)
{
	char lvlnm[8];
	sprintf(lvlnm, "%i.level", lvl);
	return loadLevel(lvlnm, board);
}

int loadLevel(char* file, board_t board)
{
	FILE* fp = fopen(file, "r");
	char b;
	int n = 0;
	int nBricksPerRow = SCREEN_SIZE / BRICK_W;

	if (!fp)
		return 0;

	while ((b = getc(fp)) != EOF)
	{
		if (isdigit(b))
		{
			board[n / nBricksPerRow][n % nBricksPerRow] = b - '0';
			n++;
		}
	}

	fclose(fp);
	return 1;
}

gamestate_t gameLoop(ball_t* ball, paddle_t* paddle, board_t board)
{
	// If ball fell off the screen, send the appropriate flag to main
	if (!ball->alive)
	{
		return (ball->lives > 0) ? DEAD : GAMEOVER;
	}

	// If board is empty, go to next level
	if (boardIsEmpty(board))
	{
		return NEXTLVL;
	}

	// Respond to user input
	if (!processInput(ball, paddle))
	{
		return QUIT;
	}

	// Update motion of ball & paddle
	if (!paddle->locked)
	{
		updateBall(ball, DT);
		updatePaddle(paddle, DT);
	}

	// Check for collisions
	checkBoardCollisions(ball, board);
	checkPaddleCollisions(ball, paddle);

	// Draw the game
	gfx_clear();
	drawBoard(board);
	drawBall(ball);
	drawPaddle(paddle);

	gfx_color(WHITE);
	char scoreText[24], lifeText[24];
	sprintf(lifeText, "Lives: %i", ball->lives);
	sprintf(scoreText, "Score: %i", score(0));
	gfx_text(10, SCREEN_SIZE - TEXT_HEIGHT * 2, scoreText);
	gfx_text(10, SCREEN_SIZE - TEXT_HEIGHT, lifeText);

	gfx_flush();

	return NORMAL;
}

int playAgain(char* msg)
{
	event_t evt;
	char scoreText[24];
	
	sprintf(scoreText, "Final Score: %i", score(0));

	gfx_color(WHITE);
	gfx_clear();
	gfx_text(SCREEN_SIZE / 3, SCREEN_SIZE / 2 - 2 * TEXT_HEIGHT, msg);
	gfx_text(SCREEN_SIZE / 3, SCREEN_SIZE / 2 - TEXT_HEIGHT, scoreText);
	gfx_text(SCREEN_SIZE / 3, SCREEN_SIZE / 2, "Press ENTER to play again");
	gfx_text(SCREEN_SIZE / 3, SCREEN_SIZE / 2 + TEXT_HEIGHT, "Press ESCAPE to quit");
	gfx_flush();

	while (1)
	{
		evt = gfx_wait();
		if (evt.type == KEYDOWN)
		{
			if (evt.key == KEY_ESCAPE)
				return 0;
			else if (evt.key == KEY_ENTER)
				return 1;
		}
	}
}

void kill(ball_t* ball)
{
	ball->lives -= 1;
	ball->alive = 0;
}

int processInput(ball_t* ball, paddle_t* paddle)
{
	//paddle->velocity.x = 0;

	event_t event;
	while (gfx_event_waiting())
	{
		event = gfx_wait();
		switch (event.type)
		{
			case KEYDOWN:
				switch (event.key)
				{
					case KEY_LEFT:
						if (paddle->locked)
							paddle->launchAngle = fmin(paddle->launchAngle + PADDLE_DTHETA, 7 * M_PI / 8);
						else
							paddle->velocity.x = -PADDLE_SPEED;
						break;
					case KEY_RIGHT:
						if (paddle->locked)
							paddle->launchAngle = fmax(paddle->launchAngle - PADDLE_DTHETA, M_PI / 8);
						else
							paddle->velocity.x = PADDLE_SPEED;
						break;
					case ' ':
						if (paddle->locked)
						{
							paddle->locked = 0;
							ball->velocity.x = BALL_SPEED * cosf(paddle->launchAngle);
							ball->velocity.y = BALL_SPEED * sinf(paddle->launchAngle);
						}
						break;
				}
				break;
			case KEYUP:
				if ((event.key == KEY_LEFT) || (event.key == KEY_RIGHT))
					paddle->velocity.x = 0;
				break;
			case 'q':
				return 0;
		}
	}

	return 1;
}

void updateBall(ball_t* ball, float timestep)
{
	ball->position.x += ball->velocity.x * timestep;
	ball->position.y += ball->velocity.y * timestep;

	if ((ball->position.x < 0))
	{
		ball->position.x = 0;
		ball->velocity.x *= -1.;
	}
	else if ((ball->position.x + ball->radius) > SCREEN_SIZE)
	{
		ball->position.x = SCREEN_SIZE - ball->radius;
		ball->velocity.x *= -1.;
	}
	
	if (ball->position.y < 0)
	{
		ball->position.y = 0;
		ball->velocity.y *= -1.;
	}
	else if (ball->position.y > SCREEN_SIZE)
		kill(ball);
}

void updatePaddle(paddle_t* paddle, float timestep)
{
	paddle->position.x += paddle->velocity.x * timestep;

	if (paddle->position.x < 0)
		paddle->position.x = 0;
	else if ((paddle->position.x + paddle->width) > SCREEN_SIZE)
		paddle->position.x = SCREEN_SIZE - paddle->width;
}

int boardIsEmpty(board_t board)
{
	int r, c;
	for (r = 0; r < (SCREEN_SIZE / BRICK_H); r++)
	{
		for (c = 0; c < (SCREEN_SIZE / BRICK_W); c++)
		{
			if (board[r][c])
				return 0;
		}
	}

	return 1;
}

void drawPaddle(paddle_t* paddle)
{
	setDrawColor(paddle->color);
	gfx_fill_rectangle(paddle->position.x, paddle->position.y, paddle->width, PADDLE_H);
	gfx_color(WHITE);
	gfx_rectangle(paddle->position.x, paddle->position.y, paddle->width, PADDLE_H);

	if (paddle->locked)
	{
		gfx_text(10, 20, "Use Left/Right Arrow Keys to aim.");
		gfx_text(10, 20 + TEXT_HEIGHT, "Press SPACEBAR to launch ball.");

		gfx_color(COLOR_SIGHT);
		gfx_line(paddle->position.x + paddle->width / 2, paddle->position.y - BALL_RADIUS, 
			(paddle->position.x + paddle->width / 2) + PADDLE_SIGHT_LEN * cosf(paddle->launchAngle), paddle->position.y - BALL_RADIUS - PADDLE_SIGHT_LEN * sinf(paddle->launchAngle));
	}
}

void drawBall(ball_t* ball)
{
	setDrawColor(ball->color);
	gfx_circle((int)ball->position.x, (int)ball->position.y, ball->radius);
}

void drawBoard(board_t board)
{
	int r, c;
	unsigned char brick;
	color_t brickColors[] = { { COLOR_BRICK1 }, { COLOR_BRICK2 }, { COLOR_BRICK3 } };
	for (r = 0; r < (SCREEN_SIZE / BRICK_H); r++)
	{
		for (c = 0; c < (SCREEN_SIZE / BRICK_W); c++)
		{
			brick = board[r][c];
			if (brick)
			{
				setDrawColor(brickColors[brick - 1]);
				gfx_fill_rectangle(c * BRICK_W, r * BRICK_H, BRICK_W, BRICK_H);
				gfx_color(BLACK);
				gfx_rectangle(c * BRICK_W, r * BRICK_H, BRICK_W, BRICK_H);
			}
		}
	}
}

void checkPaddleCollisions(ball_t* ball, paddle_t* paddle)
{
	rect_t brect, prect, overlap;
	brect.x = ball->position.x - ball->radius;
	brect.y = ball->position.y - ball->radius;
	brect.w = brect.h = ball->radius * 2;

	prect.x = paddle->position.x;
	prect.y = paddle->position.y;
	prect.w = paddle->width;
	prect.h = PADDLE_H;

	if (intersects(brect, prect, &overlap))
	{

		if (overlap.y > ball->position.y) // Ball hit from above and/or one of the sides
		{
			// Reverse the y-velocity
			ball->velocity.y *= -1.;

			// Hit from the side
			if (overlap.w < overlap.h)
			{
				ball->position.x -= signum(ball->velocity.x) * overlap.w;
				ball->velocity.x *= (signum(ball->velocity.x) == signum(paddle->velocity.x)) ? 1. : -1.;
			}

			// Move the ball out of the paddle
			ball->position.y -= overlap.h;
		}

		// Give the ball a little bit of x velocity based on the paddle's velocity
		ball->velocity.x += paddle->velocity.x * PADDLE_FRICTION;
	}
}

void checkBoardCollisions(ball_t* ball, board_t board)
{
	int r, c;
	rect_t brick, bl, overlap;
	bl.x = ball->position.x;
	bl.y = ball->position.y;
	bl.w = bl.h = 2 * BALL_RADIUS;
	brick.w = BRICK_W;
	brick.h = BRICK_H;
	//usleep(1000000);
	for (r = 0; r < (SCREEN_SIZE / BRICK_H); r++)
	{
		for (c = 0; c < (SCREEN_SIZE / BRICK_W); c++)
		{
			if (board[r][c])
			{
				brick.x = c * BRICK_W;
				brick.y = r * BRICK_H;

				if (intersects(brick, bl, &overlap))
				{
					board[r][c]--;
					score(BRICK_SCORE);

					
					if (ball->velocity.y < 0)
					{
						ball->position.y = brick.y + brick.h + 1;
					}
					else
					{
						ball->position.y = brick.y - ball->radius - 1;
					}
					ball->velocity.y *= -1.;

					if (overlap.w < overlap.h)	// Hit from mostly top or mostly bottom
					{
						ball->position.x -= signum(ball->velocity.x) * overlap.w;
						ball->velocity.x *= -1;
					}
					
					return;
				}
			}
		}
	}
}

int intersects(rect_t a, rect_t b, rect_t* result)
{
	if ((a.x > (b.x + b.w)) || (b.x > (a.x + a.w)) || (a.y > (b.y + b.h)) || (b.y > (a.y + a.h)))
		return 0;
	
	int x2, y2;
	if (result)
	{
		result->x = fmax(a.x, b.x);
		result->y = fmax(a.y, b.y);
		result->w = fmin(a.x + a.w, b.x + b.w) - result->x;
		result->h = fmin(a.y + a.h, b.y + b.h) - result->y;
	}
	
	return 1;
}

void setDrawColor(color_t c)
{
	gfx_color(c.r, c.g, c.b);
}

void buildColor(color_t* col, unsigned char r, unsigned char g, unsigned char b)
{
	col->r = r;
	col->g = g;
	col->b = b;
}

int signum(float a)
{
	return ((a > 0) - (a < 0));
}

int score(int inc)
{
	static int scr;
	scr += inc;
	return scr;
}

void resetScore()
{
	score(-score(0));
}
