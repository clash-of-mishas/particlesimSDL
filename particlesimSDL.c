// Particle Simulator, written by misha soup
// Project started in October 2023
// Requires SDL2

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// all these variables are explained in the config file
typedef struct configOptions{
	
	int MAX_PARTICLE_COUNT;
	double MAX_PARTICLE_SPEED;
	double MIN_PARTICLE_SPEED;
	double MAX_DIRECTION;
	char ENABLE_STARTING_PARTICLES;
	char ENABLE_BENCHMARK;
	double MAX_BENCHMARK_SPF;
	char ENABLE_AUTO_ADD_PARTICLES;
	char ENABLE_CIRCLE_PARTICLES;
	char ENABLE_CIRCLE_FILLED;
	char ENABLE_BORDER_COLLISION;
	char ENABLE_BORDER_CLAMP;
	char ENABLE_PARTICLE_COLLISION;
	int MAX_MEMORY_ALLOCATION;
	double BACKGROUND_COL_R;
	double BACKGROUND_COL_G;
	double BACKGROUND_COL_B;
	char ENABLE_GENERATE_ONCE;
	int BUTTON_PADDING;
	double BUTTON_TRANSPARENCY;
	double BUTTON_COL_R;
	double BUTTON_COL_G;
	double BUTTON_COL_B;
	int WINDOW_WIDTH;
	int WINDOW_HEIGHT;
	double FRICTION;
	
} configOptions;

const char optStr1[] = "MAX_PARTICLE_COUNT";
const char optStr2[] = "MAX_PARTICLE_SPEED";
const char optStr3[] = "MIN_PARTICLE_SPEED";
const char optStr7[] = "MAX_DIRECTION";
const char optStr8[] = "ENABLE_STARTING_PARTICLES";
const char optStr9[] = "ENABLE_BENCHMARK";
const char optStr10[] = "MAX_BENCHMARK_SPF";
const char optStr11[] = "ENABLE_AUTO_ADD_PARTICLES";
const char optStr12[] = "ENABLE_CIRCLE_PARTICLES";
const char optStr13[] = "ENABLE_CIRCLE_FILLED";
const char optStr16[] = "ENABLE_BORDER_COLLISION";
const char optStr17[] = "ENABLE_BORDER_CLAMP";
const char optStr18[] = "ENABLE_PARTICLE_COLLISION";
const char optStr19[] = "MAX_MEMORY_ALLOCATION";
const char optStr24[] = "BACKGROUND_COL_R";
const char optStr25[] = "BACKGROUND_COL_G";
const char optStr26[] = "BACKGROUND_COL_B";
const char optStr27[] = "ENABLE_GENERATE_ONCE";
const char optStr28[] = "BUTTON_PADDING";
const char optStr29[] = "BUTTON_TRANSPARENCY";
const char optStr30[] = "BUTTON_COL_R";
const char optStr31[] = "BUTTON_COL_G";
const char optStr32[] = "BUTTON_COL_B";
const char optStr33[] = "WINDOW_WIDTH";
const char optStr34[] = "WINDOW_HEIGHT";
const char optStr35[] = "FRICTION";

// list of particles types. Each particle share some or all of the 
// forces, and interact with each other in unique ways.
// particles can change into a different type, emit,
// absorb, bond or be annihilated depending on the type
typedef enum {
	
	red_particle,
	blue_particle,
	green_particle,
	yellow_particle,
	pink_particle,
	
	numOfParticleTypes
	
} particleType;

// the particle structure with all the properties each particle will have
typedef struct particle {
	
	double x;
	double y;
	
	double velocityX;
	double velocityY;
	
	double r;
	double g;
	double b;
	
	double size;
	
	double mass;
	
	particleType type;
	
	int collidingWith;
	int bondingWith;
	
} particle;

// what to do on mouse button down / finger tap
// more will be added later
typedef enum{
	
	addParticle,
	changeVelocity
	
} tapMode;

// how many particles are currently on the screen
static int length;

// the restrict keyword tells the compiler that
// this pointer will never change - ie, this pointer wil be pointing
// at the same address for the entirety of the program's life,
// allowing the compiler to do optimisations on it
static particle* restrict particles;

// how many bonding there are between particles
static int bondLength;

// a 2D array containing the particle numbers of bonding particles
static int* restrict bonding;

// needed to convert degrees to radians
static const double halfPi = M_PI / 180.0;

// our particle window
static SDL_Window* win;

// the renderer that will draw particles on the window
static SDL_Renderer* winRend;

// time last frame took to render
static double delta;

// file to read the options from
static FILE* config;

// our options struct
static configOptions* restrict options;

// keeps track if the particle window is open
static char isRunning;

// keeps track if the particles are moving or paused
static char isSimulating;

// the buttons that will be drawn on the screen
static SDL_Rect* restrict buttons;

// keep track of which button is being pressed
static int buttonPressed;

// which particle to add. User can change this by 
// pressing the "particle colour" button
static int addParticleType;

// keeps track of what to do when user taps/presses
static tapMode mode;

// the user has selected a particle to change velocity
static int selectedParticle;

// the velocity to change the selected particle to
static double velocityXToChange, velocityYToChange;

// we need to know where on the window the user has pressed
static SDL_Event mouseDown;

// used for debugging
static FILE* restrict debug;

// draw a circle, outlined or filled, for each particle.
// this function uses the midpoint circle algorithm, in particular Jesko's method
// the static inline keywords tell the compiler that we don't want to
// treat this as a separate function, but rather bake the code inside the 
// function inside the code that called it - to remove the overhead
// of calling a function, which includes setting up the stack, copying
// arguments etc
static inline void drawCircle(int particleNum, int xPos, int yPos, int diameter, char filled){
	
	int centreX, centreY, x;
	
	// drawing circles for the buttons
	if(xPos < 0){
		
		centreX = (int)particles[particleNum].x; 
		centreY = (int)particles[particleNum].y;
		x = (int)(0.5 * particles[particleNum].size); // grab the radius
		
	}
	
	else{
		
		centreX = xPos; 
		centreY = yPos;
		x = diameter >> 1;
		
	}
	
	int y = 0;
	int tx;
	int ty = x / 16;
	
	while(!(x < y)){
		
		// instead of drawing pixels around the circle, we draw lines
		// through the circle
		if(filled){
			
			SDL_RenderDrawLine(winRend, centreX - x, centreY - y, centreX + x, centreY - y);
			SDL_RenderDrawLine(winRend, centreX - x, centreY + y, centreX + x, centreY + y);
			SDL_RenderDrawLine(winRend, centreX - y, centreY - x, centreX + y, centreY - x);
			SDL_RenderDrawLine(winRend, centreX - y, centreY + x, centreX + y, centreY + x);
			
		}
		 
		else{
			
			//  Each of the following renders an octant of the circle
			SDL_RenderDrawPoint(winRend, centreX + x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX + x, centreY + y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY + y);
			SDL_RenderDrawPoint(winRend, centreX + y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX + y, centreY + x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY + x);
			
		}
		
		y++;
		ty += y;
		tx = ty - x;
		
		if(tx >= 0){
			
			ty = tx;
			x--;
			
		}
		
   }
   
   return;
   
}

// C doesn't have standard integer min() and max()???? WTF?????
// this is the closest thing we have to lambda functions in C
static inline int max(int a, int b){ return ((a > b) ? a : b); }
static inline int min(int a, int b){ return ((a < b) ? a : b); }

// draw a triangle
static inline void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, char fill){
	
	if(fill){
		
		// sort the vertices by y coordinate
		if(y1 > y2){
			
			int swapX = x1;
			int swapY = y1;
			x1 = x2;
			y1 = y2;
			x2 = swapX;
			y2 = swapY;
			
		}
		
		if(y2 > y3){
			
			int swapX = x2;
			int swapY = y2;
			x2 = x3;
			y2 = y3;
			x3 = swapX;
			y3 = swapY;
			
		}
		
		if(y1 > y2){
			
			int swapX = x1;
			int swapY = y1;
			x1 = x2;
			y1 = y2;
			x2 = swapX;
			y2 = swapY;
			
		}
		
		// the bottom two Y positions are the same, therefore the flat side is on the bottom
		if(y2 == y3){
			
			for(int i = 0; i <  (max(x2, x3) - min(x2, x3)); i++){
				
				SDL_RenderDrawLine(winRend, (min(x2, x3) + i), y2, x1, y1);
				
			}
			
		}
		
		// top 2 y positions are the same
		else if(y1 == y2){
			
			for(int i = 0; i < (max(x1, x2) - min(x1, x2)); i++){
				
				SDL_RenderDrawLine(winRend, (min(x1, x2) + i), y2, x3, y3);
				
			}
			
		}
		
		// the triangle has one of its side straight on the Y axis
		else if(x1 == x3){
			
			for(int i = 0; i < (max(y1, y3) - min(y1, y3)); i++){
				
				SDL_RenderDrawLine(winRend, x1, (y1 + i), x2, y2);
				
			}
			
		}
		
		else{
			
			// cut the triangle in half and draw the two smaller triangles
			int middleX = x1 + (int)((((double)(y2 - y1)) / ((double)(y3 - y1))) * ((double)(x3 - x1)));
			
			for(int i = 0; i < (max(x2, middleX) - min(x2, middleX)); i++){
				
				SDL_RenderDrawLine(winRend, (min(x2, middleX) + i), y2, x1, y1);
				
			}
			
			for(int i = 0; i < (max(x2, middleX) - min(x2, middleX)); i++){
				
				SDL_RenderDrawLine(winRend, (min(x2, middleX) + i), y2, x3, y3);
				
			}
			
		}
		
	}
	
	else{
		
		// we just want a outline, easy
		SDL_RenderDrawLine(winRend, x1, y1, x2, y2);
		SDL_RenderDrawLine(winRend, x2, y2, x3, y3);
		SDL_RenderDrawLine(winRend, x1, y1, x3, y3);
		
	}
	
	return;
	
}

// draws the reset icon on the button.
// Uses a modified version of the algorihm used in the function above
static inline void drawResetIcon(){
	
	int centreX = buttons[3].x + (buttons[3].w >> 1);
	int centreY = buttons[3].y + (buttons[3].h >> 1);
	
	// draw the (thick) circle
	for(int i = 0; i < (buttons[3].h >> 4); i++){
		
		int x = (buttons[3].h / 3) - i;
		
		int y = 0;
		int tx;
		int ty = x / 16;
		
		while(!(x < y)){
				
			//  Each of the following renders an octant of the circle
			SDL_RenderDrawPoint(winRend, centreX + x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY + y);
			SDL_RenderDrawPoint(winRend, centreX + y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY + x);
			
			y++;
			ty += y;
			tx = ty - x;
			
			if(tx >= 0){
				
				ty = tx;
				x--;
				
			}
			
	   }
	
	}
	
	int startingX = (buttons[3].x + (buttons[3].w >> 1)) + (buttons[3].h / 3) - (buttons[3].h / 7);
	int endingX = (buttons[3].x + (buttons[3].w >> 1)) + (buttons[3].h / 3) + (buttons[3].h / 7);
	int pointX = (buttons[3].x + (buttons[3].w >> 1)) + (buttons[3].h / 3);
	int startingY = buttons[3].y + (buttons[3].h >> 1);
	int endingY = (buttons[3].y + (buttons[3].h >> 1)) + (buttons[3].h / 6);
	
	drawTriangle(startingX, startingY, pointX, endingY, endingX, startingY, 1);
   
   return;
   
}

// draw the "add particle" icon
static inline void drawAddParticleIcon(){
	
	int centreX = buttons[4].x + (buttons[4].w >> 1);
	int centreY = buttons[4].y + (buttons[4].h >> 1);
	
	// draw the (thick) circle
	for(int i = 0; i < (buttons[4].h >> 4); i++){
		
		int x = (buttons[4].h / 3) - i;
		
		int y = 0;
		int tx;
		int ty = x / 16;
		
		while(!(x < y)){
				
			//  Each of the following renders an octant of the circle
			SDL_RenderDrawPoint(winRend, centreX + x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX + x, centreY + y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY - y);
			SDL_RenderDrawPoint(winRend, centreX - x, centreY + y);
			SDL_RenderDrawPoint(winRend, centreX + y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX + y, centreY + x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY - x);
			SDL_RenderDrawPoint(winRend, centreX - y, centreY + x);
			
			y++;
			ty += y;
			tx = ty - x;
			
			if(tx >= 0){
				
				ty = tx;
				x--;
				
			}
			
	   }
	
	}
	
	// draw the "plus" shape, the two loops draws a line each,
	// perpendicular to each other in the middle
	int startingX = (centreX - (buttons[4].h >> 5));
	int startingY = (centreY - (buttons[4].h / 6));
	int endingY = (centreY + (buttons[4].h / 6));
	
	for(int i = 0; i < (buttons[4].h >> 4); i++){
		
		SDL_RenderDrawLine(winRend, (startingX + i), startingY, (startingX + i), endingY);
		
	}
	
	startingX = (centreX - (buttons[4].h / 6));
	startingY = (centreY - (buttons[4].h >> 5));
	int endingX = (centreX + (buttons[4].h / 6));
	
	for(int i = 0; i < (buttons[4].h >> 4); i++){
		
		SDL_RenderDrawLine(winRend, startingX, (startingY + i), endingX, (startingY + i));
		
	}
	
	return;
	
}


static inline void drawChangeVelocityIcon(){
	
	int centreX = (buttons[4].x + (buttons[4].w >> 1)) - (buttons[4].h >> 2) - 1;
	int centreY = (buttons[4].y + (buttons[4].h >> 1));
	
	// the radius
	int x = (buttons[4].h >> 2);
	
	int y = 0;
	int tx;
	int ty = x / 16;
	
	while(!(x < y)){
		
		SDL_RenderDrawLine(winRend, centreX - x, centreY - y, centreX + x, centreY - y);
		SDL_RenderDrawLine(winRend, centreX - x, centreY + y, centreX + x, centreY + y);
		SDL_RenderDrawLine(winRend, centreX - y, centreY - x, centreX + y, centreY - x);
		SDL_RenderDrawLine(winRend, centreX - y, centreY + x, centreX + y, centreY + x);
		
		y++;
		ty += y;
		tx = ty - x;
		
		if(tx >= 0){
			
			ty = tx;
			x--;
			
		}
	}	
		
	
	
	// draw an arrow pointing towards the particle
	drawTriangle(
		((buttons[4].x + (buttons[4].w >> 1)) + (buttons[4].h >> 1)),
		(buttons[4].y + (buttons[4].h >> 2)),
		((buttons[4].x + (buttons[4].w >> 1))),
		(buttons[4].y + (buttons[4].h >> 1)),
		((buttons[4].x + (buttons[4].w >> 1)) + (buttons[4].h >> 1)),
		(buttons[4].y + ((buttons[4].h >> 2) * 3)),
		1
	);
	
	return;
	
}

// stores the state for the PRNG
unsigned int randState;

// generate a pseudo random number between 0 and 2^32
// Uses the xorshift32 algorithm, and is thread safe
static inline unsigned int randu(unsigned int* seed){
	
	*seed ^= *seed << 13;
	*seed ^= *seed >> 17;
	*seed ^= *seed << 5;
	
	return *seed;
	
}

// get the random number, cast to double & divide by the maximum
// possible number for an unsigned int so it will return 
// a nunber between 0 and 1
// Thread safe as well
static inline double randf(unsigned int* seed){
	
	return (double)(randu(seed)) / (double)(0xFFFFFFFFu);
	
}

// generate random particles at x, y
// position. if either is negative, then the particles
// will be spread over the window
static inline void generateRandomParticles(int x, int y){
	
	// generate a random number of particles to add
	int particleCount = (int)(randf(&randState) * (double)options->MAX_PARTICLE_COUNT);
	
	if(options->ENABLE_GENERATE_ONCE){
		
		particleCount = 1;
		
	}
	
	if(particleCount == 0){
		
		return;
		
	}
	
	// we need to check if the allocated memory is enough to hold
	// the amount of particles, if it's too much particles will stop generating 
	if(((length + particleCount) * (int)sizeof(particle)) > options->MAX_MEMORY_ALLOCATION){
		
		return;
		
	}

	// loop over every particle, giving it random values
	for(int i = length; i < (length + particleCount); i++){
		
		if(x < 0 || y < 0){
			
			particles[i].x = randf(&randState) * (double)options->WINDOW_WIDTH;
			particles[i].y = randf(&randState) * (double)options->WINDOW_HEIGHT;
			
		}
		
		else{
			
			particles[i].x = (double)x;
			particles[i].y = (double)y;
			
		}
		
		// select a random particle type
		if(addParticleType == -1){
			
			particles[i].type = (int)(randf(&randState) * (double)numOfParticleTypes);
			
		}
		
		// add specific particle type 
		else{
			
			particles[i].type = addParticleType;
			
		}
		
		// select mass, size and colour based on type
		switch(particles[i].type){
			
			case red_particle:
			
				particles[i].r = 255.0;
				particles[i].g = 0.0;
				particles[i].b = 0.0;
				particles[i].size = 20.0;
				particles[i].mass = 1.0;
				
				// red is large, and light
				
				break;
				
			case blue_particle:
			
				particles[i].r = 0.0;
				particles[i].g = 0.0;
				particles[i].b = 255.0;
				particles[i].size = 25.0;
				particles[i].mass = 1.2f;
				
				// blue is larger, and a bit heavier
				
				break;
				
			case green_particle:
			
				particles[i].r = 0.0;
				particles[i].g = 255.0;
				particles[i].b = 0.0;
				particles[i].size = 10.0;
				particles[i].mass = 0.01f;
				
				// green is small and very light
				
				break;
				
			case yellow_particle:
			
				particles[i].r = 255.0;
				particles[i].g = 255.0;
				particles[i].b = 0.0;
				particles[i].size = 5.0;
				particles[i].mass = 10.0;
				
				// yellow is very small and very heavy 
				
				break;
				
			case pink_particle:
			
				particles[i].r = 255.0;
				particles[i].g = 0.0;
				particles[i].b = 255.0;
				particles[i].size = 2.0;
				particles[i].mass = 0.0001f;
				
				// pink is extremely small and extremely light
				
				break;
			
			default:
				 
				 break;
				
		}
		
		// if the user has selected pixels for the particles, set the size (diameter) to 1 pixel
		if(!(options->ENABLE_CIRCLE_PARTICLES)){
			
			particles[i].size = 1.0;
			
		}
		
		// select a random direction and speed
		double direction = ((randf(&randState) * (options->MAX_DIRECTION)) - (0.5 * options->MAX_DIRECTION)) - 90.0;
		double speed = (randf(&randState) * (options->MAX_PARTICLE_SPEED - options->MIN_PARTICLE_SPEED)) + options->MIN_PARTICLE_SPEED;
		
		//calculate the velocity from direction and speed
		particles[i].velocityX = cos(direction * halfPi) * speed;
		particles[i].velocityY = sin(direction * halfPi) * speed;
		
		particles[i].collidingWith = -1;
		particles[i].bondingWith = -1;

	}
	
	length += particleCount; // add to total amount
	
	return;
	
}

//border collision
static inline void handleBorderCollision(){
	
	if(options->ENABLE_BORDER_COLLISION){
		
		for(int particleNum = 0; particleNum < length; particleNum++){
			
			// we need the radius
			double radius = 0.5 * particles[particleNum].size;
			
			// collision with the left border
			if(((particles[particleNum].x - radius) < 0.0)){
				
				// check if velocity is going past the border
				if(particles[particleNum].velocityX < 0.0){
					
					particles[particleNum].velocityX = -particles[particleNum].velocityX;
					particles[particleNum].collidingWith = -1;
					
					if(particles[particleNum].bondingWith > -1){
						
						particles[particles[particleNum].bondingWith].velocityX = -particles[particles[particleNum].bondingWith].velocityX;
						particles[particles[particleNum].bondingWith].collidingWith = -1;
						
					}
					
				}
				
				if(options->ENABLE_BORDER_CLAMP){
					
					particles[particleNum].x = radius;
					
				}
				
			}
			
			// right border
			else if((particles[particleNum].x + radius) > options->WINDOW_WIDTH){
				
				if(particles[particleNum].velocityX > 0.0){
					
					particles[particleNum].velocityX = -particles[particleNum].velocityX;
					particles[particleNum].collidingWith = -1;
					
					if(particles[particleNum].bondingWith > -1){
						
						particles[particles[particleNum].bondingWith].velocityX = -particles[particles[particleNum].bondingWith].velocityX;
						particles[particles[particleNum].bondingWith].collidingWith = -1;
						
					}
					
				}
				
				if(options->ENABLE_BORDER_CLAMP){
					
					particles[particleNum].x = (double)(options->WINDOW_WIDTH) - radius;
					
				}
				
			}
			
			// top border
			if(((particles[particleNum].y - radius) < 0.0)){
				
				if(particles[particleNum].velocityY < 0.0){
					
					particles[particleNum].velocityY = -particles[particleNum].velocityY;
					particles[particleNum].collidingWith = -1;
					
					if(particles[particleNum].bondingWith > -1){
						
						particles[particles[particleNum].bondingWith].velocityY = -particles[particles[particleNum].bondingWith].velocityY;
						particles[particles[particleNum].bondingWith].collidingWith = -1;
						
					}
					
				}
				
				if(options->ENABLE_BORDER_CLAMP){
					
					particles[particleNum].y = radius;
					
				}
				
			}
			
			// bottom border
			else if(((particles[particleNum].y + radius) > options->WINDOW_HEIGHT)){
				
				if(particles[particleNum].velocityY > 0.0){
					
					particles[particleNum].velocityY = -particles[particleNum].velocityY;
					particles[particleNum].collidingWith = -1;
					
					if(particles[particleNum].bondingWith > -1){
						
						particles[particles[particleNum].bondingWith].velocityY = -particles[particles[particleNum].bondingWith].velocityY;
						particles[particles[particleNum].bondingWith].collidingWith = -1;
						
					}
					
				}
				
				if(options->ENABLE_BORDER_CLAMP){
					
					particles[particleNum].y = (double)(options->WINDOW_HEIGHT) - radius;
					
				}
				
			}
		
		}
		
	}
	
	return;
	
}

// check if two particles are bonded to anything
static inline char isBonding(int particleNum, int particleNumTo){
	
	for(int i = 0; i < bondLength; i+= 2){
		
		if((*(bonding + i) == particleNum) || (*(bonding + i + 1) == particleNum)
		|| (*(bonding + i) == particleNumTo) || (*(bonding + i + 1) == particleNumTo)){
			
			return 1;
			
		}
		
	}
	
	return 0;
	
}

// handle elastic collision (for particles
// that only exchanges kinetic energy on collison)
static inline void handleElasticCollision(int particleNumA, int particleNumB, double distance){
	
	double velXA = particles[particleNumA].velocityX;
	double velYA = particles[particleNumA].velocityY;
	double velXB = particles[particleNumB].velocityX;
	double velYB = particles[particleNumB].velocityY;
	double massA = particles[particleNumA].mass;
	double massB = particles[particleNumB].mass;
	
	// stolen from Javidx9's circle vs circle collision video, thanks! :)
	// some sort of maths magic going on here.... I have no idea what's
	// happening but it works great
	
	// get the normal vector between the two particles
	double nx = (particles[particleNumB].x - particles[particleNumA].x) / distance;
	double ny = (particles[particleNumB].y - particles[particleNumA].y) / distance;
	
	double kx = (velXA - velXB);
	double ky = (velYA - velYB);
	double p = 2.0 * (nx * kx + ny * ky) / (massA + massB);
	velXA = velXA - p * massB * nx;
	velYA = velYA - p * massB * ny;
	velXB = velXB + p * massA * nx;
	velYB = velYB + p * massA * ny;
	
	// apply the new velocities to both particles
	particles[particleNumA].velocityX = velXA;
	particles[particleNumA].velocityY = velYA;
	particles[particleNumB].velocityX = velXB;
	particles[particleNumB].velocityY = velYB;
	
	
	// also collide with bonded particles
	// The particles have spin 0 for now, which means that they don't rotate. Therefore, any collision
	// affect both particles equally
	if(particles[particleNumA].bondingWith > -1){
		
		particles[particles[particleNumA].bondingWith].velocityX = velXA;
		particles[particles[particleNumA].bondingWith].velocityY = velYA;
		
	}
	
	if(particles[particleNumB].bondingWith > -1){
		
		particles[particles[particleNumB].bondingWith].velocityX = velXB;
		particles[particles[particleNumB].bondingWith].velocityY = velYB;
		
	}
	
	return;
	
}

static inline void handleParticleInteraction(){ // new name, suits it better
	
	// check for collision with other particles
	if(options->ENABLE_PARTICLE_COLLISION){
			
		// loop through all particles checking for collision
		for(int i = 0; i < length; i++){
			
			//check if this particle is being collided
			char hasCollided = 0;
			
			// Particles are lazy, they take the shortest path possible. Therefore, 
			// they only react to the strongest force being acted on them. In other words, particles will only
			// collide away from the closest one they are colliding with.
			double lowestDistance = -1.0;
			
			// the particle that we will be colliding away from
			int lowestDistanceParticle = -1;
			
			for(int j = 0; j < length; j++){
			
				if(i != j){
					
					if(particles[i].bondingWith == j){
						
						continue;
						
					}
					
					// copy to the stack for faster processing & syntatic sugar :P
					double xa = particles[i].x;
					double ya = particles[i].y;
					double radiusA = 0.5 * particles[i].size;
					
					double xb = particles[j].x;
					double yb = particles[j].y;
					double radiusB = 0.5 * particles[j].size;
					
					// getting the distance with good old Pythagoras' Theorem
					double distance = sqrt((((xa - xb) * (xa - xb)) + ((ya - yb) * (ya - yb))));
					
					if(lowestDistance < 0.0){
						
						lowestDistance = distance;
						
					}
					
					// when red and blue particles bond, their velocities become the average
					double avgVelX, avgVelY;
					
					// check if the distance between them is
					// less than their radiuses combined
					// the + 1 is for floating point error
					if(distance < (radiusA + radiusB + 1.0)){
						
						switch (particles[i].type){
						
							case red_particle:
								
								if(particles[j].type == red_particle){
									
									if(distance < lowestDistance){
										
										lowestDistance = distance;
										
										lowestDistanceParticle = j;
										
										hasCollided = 1;
										
									}
									
								}
								
								if(particles[j].type == blue_particle){
									
									if((particles[i].bondingWith == -1) && (particles[j].bondingWith == -1)){
										
										goto bond;
										
									}
									
									else{
										
										if(distance < lowestDistance){
											
											lowestDistance = distance;
											
											lowestDistanceParticle = j;
											
											hasCollided = 1;
											
										}
										
									}
									
								}
								
								break;
								
							case blue_particle:
								
								if(particles[j].type == blue_particle){
									
									if(distance < lowestDistance){
										
										lowestDistance = distance;
										
										lowestDistanceParticle = j;
										
										hasCollided = 1;
										
									}
									
								}
								
								if(particles[j].type == red_particle){
									
									if((particles[i].bondingWith == -1) && (particles[j].bondingWith == -1)){
										
										goto bond;
										
									}
									
									else{
										
										if(distance < lowestDistance){
											
											lowestDistance = distance;
											
											lowestDistanceParticle = j;
											
											hasCollided = 1;
											
										}
										
									}
									
								}
								
								break;
								
							case green_particle:
								
								break;
								
							case yellow_particle:
								
								break;
								
							case pink_particle:
								
								break;
								
							default:
								
								break;
								
						}
						
						continue;
						
						// red-blue particles bonding
						bond:
						
						// the new velocities of the particles will be the average of both old ones
						avgVelX = (particles[i].velocityX + particles[j].velocityX) / 2.0;
						avgVelY = (particles[i].velocityY + particles[j].velocityY) / 2.0;
						
						particles[i].velocityX = avgVelX;
						particles[i].velocityY = avgVelY;
						particles[j].velocityX = avgVelX;
						particles[j].velocityY = avgVelY;
						
						double temp = particles[i].mass;
						
						particles[i].mass += particles[j].mass + particles[j].mass;
						particles[j].mass += temp + temp;
						
						particles[i].bondingWith = j;
						particles[j].bondingWith = i;
						
						continue;
						
					}
					
				}
				
			}
			
			if(hasCollided == 0){
				
				if(particles[i].bondingWith > -1){
					
					if(particles[particles[i].bondingWith].collidingWith == -1){
						
						particles[i].collidingWith = -1;
						particles[particles[i].bondingWith].collidingWith = -1;
						
					}
					
				}
				
				else{
					
					particles[i].collidingWith = -1;
					
					
				}
				
			}
			
			else{
				
				if((particles[i].collidingWith == -1) && (particles[lowestDistanceParticle].collidingWith == -1)){
					
					particles[i].collidingWith = lowestDistanceParticle;
					particles[lowestDistanceParticle].collidingWith = i;
					
					// we also want to move the particles' bonded particles
					if(particles[i].bondingWith > -1){
						
						particles[particles[i].bondingWith].collidingWith = lowestDistanceParticle;
						
					}
					
					if(particles[lowestDistanceParticle].bondingWith > -1){
						
						particles[particles[lowestDistanceParticle].bondingWith].collidingWith = i;
						
					}
					
					// change ONLY the velocities for both particles
					handleElasticCollision(i, lowestDistanceParticle, lowestDistance);
					
				}
				
			}
			
		}
		
		
	}
	
	return;
	
}

// update the position of each particle
// and handle border collision
static inline void updateParticles(){
	
	// loop through each particle
	for(int particleNum = 0; particleNum < length; particleNum++){
		
		// move the x and y position by the velocity and frame delta time
		particles[particleNum].x += particles[particleNum].velocityX * delta;
		particles[particleNum].y += particles[particleNum].velocityY * delta;
		
		// check if we need to reduce the speed via friction
		if(options->FRICTION > 0.0){
		
			// if both velocities are already at zero, no need to reduce them anymore, otherwise sqrt() will
			// return an undefined double
			if((particles[particleNum].velocityX != 0.0) && (particles[particleNum].velocityY != 0.0)){
				
				// reduce the speed of the vector by the friction * mass
				double speed = sqrt((particles[particleNum].velocityX * particles[particleNum].velocityX) +
					(particles[particleNum].velocityY * particles[particleNum].velocityY));
				
				double speedToReduce;
				
				if(particles[particleNum].bondingWith > -1){
					
					double totalMass = particles[particleNum].mass + particles[particles[particleNum].bondingWith].mass;
					
					speedToReduce = (1.0 / totalMass) * options->FRICTION;
					
				}
				
				else{
					
					speedToReduce = (1.0 / particles[particleNum].mass) * options->FRICTION;
					
				}
				
				// we need to check if the velocity has hit zero, if so, then stop drcreasing the magnitude
				char zero = particles[particleNum].velocityX > 0.0;
				
				particles[particleNum].velocityX -= ((particles[particleNum].velocityX / speed) * speedToReduce);
				
				if((zero && (particles[particleNum].velocityX < 0.0)) || (!zero && (particles[particleNum].velocityX > 0.0))){
					
					particles[particleNum].velocityX = 0.0;
					
				}
				
				
				
				zero = particles[particleNum].velocityY > 0.0;
				
				particles[particleNum].velocityY -= ((particles[particleNum].velocityY / speed) * speedToReduce);
				
				if((zero && (particles[particleNum].velocityY < 0.0)) || (!zero && (particles[particleNum].velocityY > 0.0))){
					
					particles[particleNum].velocityY = 0.0;
					
				}
				
			}
		
		}
	
	}
	
	return;
	
}

// get the options from the config file
static inline void getOptions(char* arg){
	
	// get the options
	config = 0;
	
	if(arg){
		
		// read the file supplied by the user
		config = fopen(arg, "r");
		
	}
	
	else{
		
		config = fopen("config.txt", "r");
		
	}
	
	// if we can't open the file then we quit
	if(config == 0){ exit(0); }
	
	// max of 128 characters per line
	char currentLine[128];
	// max of 20 digits for the value
	char value[20];
	
	// set all enablable options to disabled by default
	options->ENABLE_STARTING_PARTICLES = 0;
	options->ENABLE_BENCHMARK = 0;
	options->ENABLE_AUTO_ADD_PARTICLES = 0;
	options->ENABLE_CIRCLE_PARTICLES = 0;
	options->ENABLE_CIRCLE_FILLED = 0;
	options->ENABLE_BORDER_COLLISION = 0;
	options->ENABLE_BORDER_CLAMP = 0;
	options->ENABLE_PARTICLE_COLLISION = 0;
	options->MAX_MEMORY_ALLOCATION = 32768;
	options->ENABLE_GENERATE_ONCE = 0;
	
	while(!feof(config)){
		
		fgets(currentLine, sizeof(currentLine), config);
		
		// check for comments and empty lines
		if(currentLine[0] == '#' || currentLine[0] == '\n'){ continue; }
		
		for(int i = 0; i < (int)sizeof(currentLine); i++){
			
			if(currentLine[i] == ' '){
				
				for(int j = 1; j < (int)sizeof(value); j++){
					
					if(!((currentLine[i + j] >= '0' && currentLine[i + j] <= '9') || currentLine[i + j] == '.')){
						
						memset(&value, 0, sizeof(value));
						memcpy(&value, &(currentLine[i + 1]), (size_t)(j - 1));
						
						break;
						
					}
					
				}
				
				break;
				
			}
			
		}
		
		// for each individual line, check against each option string
		// and if they are the same, write the corresponding value
		// in the options struct
		if(!memcmp(&currentLine, &optStr1, (sizeof(optStr1) - 1))){ options->MAX_PARTICLE_COUNT = atoi(value); }
		if(!memcmp(&currentLine, &optStr2, (sizeof(optStr2) - 1))){ options->MAX_PARTICLE_SPEED = atof(value); }
		if(!memcmp(&currentLine, &optStr3, (sizeof(optStr3) - 1))){ options->MIN_PARTICLE_SPEED = atof(value); }
		if(!memcmp(&currentLine, &optStr7, (sizeof(optStr7) - 1))){ options->MAX_DIRECTION = atof(value); }
		if(!memcmp(&currentLine, &optStr8, (sizeof(optStr8) - 1))){ options->ENABLE_STARTING_PARTICLES = 1; }
		if(!memcmp(&currentLine, &optStr9, (sizeof(optStr9) - 1))){ options->ENABLE_BENCHMARK = 1; }
		if(!memcmp(&currentLine, &optStr10, (sizeof(optStr10) - 1))){ options->MAX_BENCHMARK_SPF = atof(value); }
		if(!memcmp(&currentLine, &optStr11, (sizeof(optStr11) - 1))){ options->ENABLE_AUTO_ADD_PARTICLES = 1; }
		if(!memcmp(&currentLine, &optStr12, (sizeof(optStr12) - 1))){ options->ENABLE_CIRCLE_PARTICLES = 1; }
		if(!memcmp(&currentLine, &optStr13, (sizeof(optStr13) - 1))){ options->ENABLE_CIRCLE_FILLED = 1; }
		if(!memcmp(&currentLine, &optStr16, (sizeof(optStr16) - 1))){ options->ENABLE_BORDER_COLLISION = 1; }
		if(!memcmp(&currentLine, &optStr17, (sizeof(optStr17) - 1))){ options->ENABLE_BORDER_CLAMP = 1; }
		if(!memcmp(&currentLine, &optStr18, (sizeof(optStr18) - 1))){ options->ENABLE_PARTICLE_COLLISION = 1; }
		if(!memcmp(&currentLine, &optStr19, (sizeof(optStr19) - 1))){ options->MAX_MEMORY_ALLOCATION = atoi(value); }
		if(!memcmp(&currentLine, &optStr24, (sizeof(optStr24) - 1))){ options->BACKGROUND_COL_R = atof(value); }
		if(!memcmp(&currentLine, &optStr25, (sizeof(optStr25) - 1))){ options->BACKGROUND_COL_G = atof(value); }
		if(!memcmp(&currentLine, &optStr26, (sizeof(optStr26) - 1))){ options->BACKGROUND_COL_B = atof(value); }
		if(!memcmp(&currentLine, &optStr27, (sizeof(optStr27) - 1))){ options->ENABLE_GENERATE_ONCE = 1; }
		if(!memcmp(&currentLine, &optStr28, (sizeof(optStr28) - 1))){ options->BUTTON_PADDING = atoi(value); }
		if(!memcmp(&currentLine, &optStr29, (sizeof(optStr29) - 1))){ options->BUTTON_TRANSPARENCY = atof(value); }
		if(!memcmp(&currentLine, &optStr30, (sizeof(optStr30) - 1))){ options->BUTTON_COL_R = atof(value); }
		if(!memcmp(&currentLine, &optStr31, (sizeof(optStr31) - 1))){ options->BUTTON_COL_G = atof(value); }
		if(!memcmp(&currentLine, &optStr32, (sizeof(optStr32) - 1))){ options->BUTTON_COL_B = atof(value); }
		if(!memcmp(&currentLine, &optStr33, (sizeof(optStr33) - 1))){ options->WINDOW_WIDTH = atoi(value); }
		if(!memcmp(&currentLine, &optStr34, (sizeof(optStr34) - 1))){ options->WINDOW_HEIGHT = atoi(value); }
		if(!memcmp(&currentLine, &optStr35, (sizeof(optStr35) - 1))){ options->FRICTION = atof(value); }
		
	}
	
	fclose(config);
	config = 0;
	
	return;
	
}

static inline void createButtons(){
	
	// height and width for each button
	int width = (max(options->WINDOW_WIDTH, options->WINDOW_HEIGHT) / 10) - (options->BUTTON_PADDING << 1);
	int height = (min(options->WINDOW_WIDTH, options->WINDOW_HEIGHT) / 8) - (options->BUTTON_PADDING << 1);
	
	// first button(pasue/resume) will be located at the top left of the screen
	buttons[0].x = options->BUTTON_PADDING;
	buttons[0].y = options->BUTTON_PADDING;
	buttons[0].w = width;
	buttons[0].h = height;
	
	// second button(select particle) will be located under the first
	buttons[1].x = options->BUTTON_PADDING;
	buttons[1].y = height + (options->BUTTON_PADDING << 1);
	buttons[1].w = width;
	buttons[1].h = height;
	
	buttons[2].x = options->BUTTON_PADDING;
	buttons[2].y = (height << 1) + (options->BUTTON_PADDING * 3);
	buttons[2].w = width;
	buttons[2].h = height;
	
	buttons[3].x = options->BUTTON_PADDING;
	buttons[3].y = (height * 3) + (options->BUTTON_PADDING * 4);
	buttons[3].w = width;
	buttons[3].h = height;
	
	buttons[4].x = options->BUTTON_PADDING;
	buttons[4].y = (height * 4) + (options->BUTTON_PADDING * 5);
	buttons[4].w = width;
	buttons[4].h = height;
	
	return;
	
}

static inline void drawButtons(){
	
	// iterate through each button and draw the boxes of the correct
	// transparency (the selected box will be twice as opaque)
	for(int i = 0; i < 5; i++){
		
		if(i == buttonPressed){
			
			SDL_SetRenderDrawColor(winRend, (Uint8)(options->BUTTON_COL_R),
				(Uint8)(options->BUTTON_COL_G), (Uint8)(options->BUTTON_COL_B), 
				(Uint8)(options->BUTTON_TRANSPARENCY * 2));
			
		}
		
		else{
			
			SDL_SetRenderDrawColor(winRend, (Uint8)(options->BUTTON_COL_R),
				(Uint8)(options->BUTTON_COL_G), (Uint8)(options->BUTTON_COL_B), 
				(Uint8)(options->BUTTON_TRANSPARENCY));
			
		}
		
		SDL_RenderFillRect(winRend, &(buttons[i]));
		
	}
	
	// set the colour of the play/pause icon to white
	SDL_SetRenderDrawColor(winRend, 255, 255, 255, 255);
	
	// draw the "pause" logo
	if(!isSimulating){
		
		for(int i = 0; i < (buttons[0].w / 3); i++){
			
			if((i > (buttons[0].w / 9)) && (i < ((buttons[0].w / 9) << 1))){
				
				continue;
				
			}
			
			SDL_RenderDrawLine(winRend, 
				(buttons[0].x + i + (buttons[0].w / 3)),
				(buttons[0].y + (buttons[0].h / 6)),
				(buttons[0].x + i + (buttons[0].w / 3)),
				(buttons[0].y + (((buttons[0].h / 6) * 5))));
			
		}
		
	}
	
	// draw the "play" logo
	else{
		
		drawTriangle((buttons[0].x + (buttons[0].w / 3)),
			(buttons[0].y + (buttons[0].h / 6)), 
			(buttons[0].x + ((buttons[0].w / 3) << 1)),
			(buttons[0].y + (buttons[0].h >> 1)),
			(buttons[0].x + (buttons[0].w / 3)),
			(buttons[0].y + ((buttons[0].h / 6) * 5)), 1);
		
	}
	
	// pick the right colour to draw on the particle type button
	switch(addParticleType){
		
		case red_particle:
			
			SDL_SetRenderDrawColor(winRend, 255, 0, 0, 255);
			
			break;
		
		case blue_particle:
			
			SDL_SetRenderDrawColor(winRend, 0, 0, 255, 255);

			break;
		
		case green_particle:
			
			SDL_SetRenderDrawColor(winRend, 0, 255, 0, 255);
		
			break;
		
		case yellow_particle:
			
			SDL_SetRenderDrawColor(winRend, 255, 255, 0, 255);
			
			break;
		
		case pink_particle:
			
			SDL_SetRenderDrawColor(winRend, 255, 0, 255, 255);
		
			break;
		
		case -1:
			
			// draw all colours next to each other
			SDL_SetRenderDrawColor(winRend, 255, 0, 0, 255);
			drawCircle(0, (buttons[1].x + ((buttons[1].w >> 1)) - (buttons[1].h / 6)), 
				((buttons[1].y + (buttons[1].h >> 1)) - (buttons[1].h / 6)), ((buttons[1].h / 6) << 1), 1);
				
			SDL_SetRenderDrawColor(winRend, 0, 0, 255, 255);
			drawCircle(0, (buttons[1].x + ((buttons[1].w >> 1)) - (buttons[1].h / 6)), 
				((buttons[1].y + (buttons[1].h >> 1)) + (buttons[1].h / 6)), ((buttons[1].h / 6) << 1), 1);
				
			SDL_SetRenderDrawColor(winRend, 0, 255, 0, 255);
			drawCircle(0, (buttons[1].x + ((buttons[1].w >> 1)) + (buttons[1].h / 6)), 
				((buttons[1].y + (buttons[1].h >> 1)) - (buttons[1].h / 6)), ((buttons[1].h / 6) << 1), 1);
			
			SDL_SetRenderDrawColor(winRend, 255, 255, 0, 255);
			drawCircle(0, (buttons[1].x + ((buttons[1].w >> 1)) + (buttons[1].h / 6)), 
				((buttons[1].y + (buttons[1].h >> 1)) + (buttons[1].h / 6)), ((buttons[1].h / 6) << 1), 1);
			
			SDL_SetRenderDrawColor(winRend, 255, 0, 255, 255);
			drawCircle(0, (buttons[1].x + (buttons[1].w >> 1)), 
				(buttons[1].y + (buttons[1].h >> 1)), ((buttons[1].h / 6) << 1), 1);
			
			
			break;
	}
	
	//draw the single ball
	if(addParticleType > -1){
		
		drawCircle(0, (buttons[1].x + (buttons[1].w >> 1)), 
			(buttons[1].y + (buttons[1].h >> 1)), ((buttons[1].h / 6) * 4), 1);
			
	}
	
	// white particle 
	SDL_SetRenderDrawColor(winRend, 255, 255, 255, 255);
	
	// check whether to draw one particle per tap/click or multiple
	if(options->ENABLE_GENERATE_ONCE){
		
		drawCircle(0, (buttons[2].x + (buttons[2].w >> 1)), 
			(buttons[2].y + (buttons[2].h >> 1)), ((buttons[2].h / 6) * 4), 1);
		
	}
	
	else{
		
		// draw 3 particles in a diagional, showing multiple particles
		drawCircle(0, (buttons[2].x + (buttons[2].w >> 1)), 
			(buttons[2].y + (buttons[2].h >> 1)), (buttons[2].h / 4), 1);
			
		drawCircle(0, (buttons[2].x + ((buttons[2].w >> 1)) + (buttons[1].h / 6)), 
			((buttons[2].y + (buttons[2].h >> 1)) + (buttons[1].h / 6)), (buttons[2].h / 4), 1);
			
		drawCircle(0, (buttons[2].x + ((buttons[2].w >> 1)) - (buttons[1].h / 6)), 
			((buttons[2].y + (buttons[2].h >> 1)) - (buttons[1].h / 6)), (buttons[2].h / 4), 1);
		
	}
	
	//draw the reset icon
	drawResetIcon();
	
	if(mode == addParticle){
		
		drawAddParticleIcon();
	
	}
	
	else{
		
		drawChangeVelocityIcon();
		
	}
	
	return;
	
}

static inline void drawSelectedParticle(){
	
	// get the distance between the particle and mouse and 
	// check if the velocity exceeds MAX_PARTICLE_SPEED
	int diffX = mouseDown.button.x - (int)(particles[selectedParticle].x);
	int diffY = mouseDown.button.y - (int)(particles[selectedParticle].y);
	
	double distance = sqrt(((double)diffX * (double)diffX) + ((double)diffY * (double)diffY));
	
	velocityXToChange = (double)(-diffX);
	velocityYToChange = (double)(-diffY);
	
	int triangleSizeX, triangleSizeY, triangleEndX, triangleEndY;
	
	// we will stop the triangle length and width when it exceeds the maximum
	// possible particle velocity
	if(distance > (options->MAX_PARTICLE_SPEED / 2.0)){
		
		// get the angle between the mouse and particle
		double angle = atan2((particles[selectedParticle].y - (double)mouseDown.button.y),
			(particles[selectedParticle].x - (double)mouseDown.button.x));
		
		// get the new direction
		// we set the amount of velocity to add to the maximum speed
		double dx = (cos(angle) * (options->MAX_PARTICLE_SPEED * 0.5));
		double dy = (sin(angle) * (options->MAX_PARTICLE_SPEED * 0.5));
		
		velocityXToChange = particles[selectedParticle].x - dx;
		velocityYToChange = particles[selectedParticle].y - dy;
		
		diffX = (int)(velocityXToChange - particles[selectedParticle].x);
		diffY = (int)(velocityYToChange - particles[selectedParticle].y);
		
		// We calculate how wide the triangle is based on the velocity we are about to apply,
		// the maximum width of the triangle is the same as the maximum particle speed / 2
		triangleSizeX = (int)(velocityXToChange) - (diffY >> 2);
		triangleSizeY = (int)(velocityYToChange) + (diffX >> 2);
		triangleEndX = (int)(velocityXToChange) + (diffY >> 2);
		triangleEndY = (int)(velocityYToChange) - (diffX >> 2);
		
		velocityXToChange = (double)(-diffX);
		velocityYToChange = (double)(-diffY);
	}
	
	else{
		
		// We calculate how wide the triangle is based on the velocity we are about to appply
		triangleSizeX = mouseDown.button.x - (int)((double)diffY * (distance / (options->MAX_PARTICLE_SPEED * 2.0)));
		triangleSizeY = mouseDown.button.y + (int)((double)diffX * (distance / (options->MAX_PARTICLE_SPEED * 2.0)));
		triangleEndX = mouseDown.button.x + (int)((double)diffY * (distance / (options->MAX_PARTICLE_SPEED * 2.0)));
		triangleEndY = mouseDown.button.y - (int)((double)diffX * (distance / (options->MAX_PARTICLE_SPEED * 2.0)));
		
	}
	
	SDL_SetRenderDrawColor(winRend, 255, 255, 255, 255);
	
	//draw a line too, in case the triangle is too thin
	SDL_RenderDrawLine(winRend, (int)(particles[selectedParticle].x), 
		(int)(particles[selectedParticle].y),
		(int)(particles[selectedParticle].x - velocityXToChange), 
		(int)(particles[selectedParticle].y - velocityYToChange));
	
	//draw the triangle, the pointy part on the particle
	drawTriangle(triangleSizeX, triangleSizeY,
		(int)(particles[selectedParticle].x),
		(int)(particles[selectedParticle].y),
		triangleEndX, triangleEndY, 1);
	
	velocityXToChange *= 2.0;
	velocityYToChange *= 2.0;
	
	// draw a white circle outline on the selected particle
	// to give us better visibility
	for(int i = 0; i < 5; i++){
		
		drawCircle(0, (int)(particles[selectedParticle].x), (int)(particles[selectedParticle].y),
			(int)(particles[selectedParticle].size) + i, 0);
		
	}
	
	return;
}

static inline void drawParticles(){
	
	// draw particles
	for(int i = 0; i < length; i++){
		
		// if a particle is outside of the window border, then we don't need to draw it
		if(((particles[i].x + (0.5 * particles[i].size )) < 0.0) || ((particles[i].x - (0.5 * particles[i].size)) > options->WINDOW_WIDTH)){ continue; }
		if(((particles[i].y + (0.5 * particles[i].size)) < 0.0) || ((particles[i].y - (0.5 * particles[i].size)) > options->WINDOW_HEIGHT)){ continue; }
		
		// pick the colour
		SDL_SetRenderDrawColor(winRend, (Uint8)particles[i].r, (Uint8)particles[i].g, (Uint8)particles[i].b, 255);
		
		// if circle particles are enabled, draw a circle
		// otherwise, draw a pixel
		if(options->ENABLE_CIRCLE_PARTICLES){
			
			drawCircle(i, -1, -1, 0, options->ENABLE_CIRCLE_FILLED);
			
		}
		
		else{
			
			SDL_RenderDrawPoint(winRend, (int)particles[i].x, (int)particles[i].y);
			
		}
		
	}
	
	return;
	
}

int main(int argc, char** argv){
	
	debug = fopen("debug.txt", "w");
	
	// allocate memory for the options
	options = malloc(sizeof(configOptions));
	
	// check if we were able to allocate successfully
	if(options == 0){ exit(0); }
	
	if(argc > 1){
		
		// get the required options
		getOptions(argv[1]);
		
	}
	
	else{
		
		getOptions(0);
		
	}
	
	// init SDL
	SDL_Init(SDL_INIT_VIDEO);
	
	//create a window and renderer objects
	SDL_CreateWindowAndRenderer(options->WINDOW_WIDTH, options->WINDOW_HEIGHT, SDL_WINDOW_SHOWN, &win, &winRend);
	
	// update x and y window size with actual values
	// cos on android it automatically changes
	SDL_GetWindowSize(win, &options->WINDOW_WIDTH, &options->WINDOW_HEIGHT);
	
	// we have three buttons right now - one to pause/resume,
	// one to select the particle type to add and one to toggle
	// ENABLE_GENERATE_ONCE on or off 
	buttons = malloc(sizeof(SDL_Rect) * 5);
	
	if(buttons == 0){ exit(0); }
	
	// we start off with the mode set to add particles
	mode = addParticle;
	
	// add the button Rects
	createButtons();
	
	buttonPressed = -1;
	
	selectedParticle = -1;
	
	// set the title of our window, very nice :)
	SDL_SetWindowTitle(win, "Particle Simulator v1.0");
	
	// we need to tell SDL that we wanna add transparency to our renderer,
	// because the buttons will be slightly transparent
	SDL_SetRenderDrawBlendMode(winRend, SDL_BLENDMODE_BLEND);
	
	// initialise with 0
	length = 0;
	delta = 0.0;
	randState = (unsigned int)SDL_GetPerformanceCounter();
	
	// allocate MAX_MEMORY_ALLOCATION bytes of memory from heap
	// for the particle themselves
	particles = malloc((size_t)(options->MAX_MEMORY_ALLOCATION));
	
	// if not able to allocate memory, we print and quit
	if(particles == 0){ exit(0); }
	
	// only two particles can bond at the moment
	bonding = malloc((((size_t)(options->MAX_MEMORY_ALLOCATION) / sizeof(particle))) * sizeof(int));
	
	if(bonding == 0){ exit(0); }
	
	isRunning = 1;
	
	// set the game to paused on startup
	isSimulating = 0;
	SDL_Event event;
	mouseDown.button.x = 0;
	mouseDown.button.y = 0;
	char isHoldingDown = 0;
	Uint64 startFrameTick, endFrameTick;
	double tickSpeed = (double)SDL_GetPerformanceFrequency();
	
	// red is the default particle to add
	addParticleType = red_particle;
	
	
	// generate initial particles
	if(options->ENABLE_STARTING_PARTICLES){
		
		generateRandomParticles(-1, -1);
		
	}
	
	while(isRunning){
		
		startFrameTick = SDL_GetPerformanceCounter();
		// poll events
		
		while(SDL_PollEvent(&event)){
			
			switch(event.type){
				
				case SDL_MOUSEBUTTONDOWN:
					
					// check if pressing pause/play button
					if(event.button.x > buttons[0].x && event.button.y > buttons[0].y &&
						event.button.x < (buttons[0].x + buttons[0].w) &&
						event.button.y < (buttons[0].y + buttons[0].h)){
						
						if(isSimulating){
							
							isSimulating = 0;
							
						}
						
						else{
							
							isSimulating = 1;
							
						}
						
						buttonPressed = 0;
						
					}
					
					// check if pressing particle type button
					else if(event.button.x > buttons[1].x && event.button.y > buttons[1].y &&
						event.button.x < (buttons[1].x + buttons[1].w) &&
						event.button.y < (buttons[1].y + buttons[1].h)){
						
						// switch to the next type
						addParticleType += 1;
						
						if(addParticleType == numOfParticleTypes){
							
							addParticleType = -1;
							
						}
						
						buttonPressed = 1;
						
					}
					
					// check if pressing generate one/multiple particle button
					else if(event.button.x > buttons[2].x && event.button.y > buttons[2].y &&
						event.button.x < (buttons[2].x + buttons[2].w) &&
						event.button.y < (buttons[2].y + buttons[2].h)){
						
						options->ENABLE_GENERATE_ONCE = options->ENABLE_GENERATE_ONCE ? 0 : 1;
						
						buttonPressed = 2;
						
					}
					
					// check if pressing generate one/multiple particle button
					else if(event.button.x > buttons[3].x && event.button.y > buttons[3].y &&
						event.button.x < (buttons[3].x + buttons[3].w) &&
						event.button.y < (buttons[3].y + buttons[3].h)){
						
						length = 0;
						
						buttonPressed = 3;
						
					}
					
					// check if pressing tap mode button
					else if(event.button.x > buttons[4].x && event.button.y > buttons[4].y &&
						event.button.x < (buttons[4].x + buttons[4].w) &&
						event.button.y < (buttons[4].y + buttons[4].h)){
						
						if(mode == addParticle){
							
							mode = changeVelocity;
							
						}
						
						else{
							
							mode = addParticle;
						}
						
						buttonPressed = 4;
						
					}
					
					else{
					
						if(!(event.button.x > options->WINDOW_WIDTH || event.button.x < 0.0 || 
							event.button.y > options->WINDOW_HEIGHT || event.button.y < 0.0)){
							
							isHoldingDown = 1;
							
							if(mode == changeVelocity){
									
								//loop through each particle and check if the mouse is over them
								for(int i = 0; i < length; i++){
									
									double w = (double)(mouseDown.button.x) - particles[i].x;
									double h = (double)(mouseDown.button.y) - particles[i].y;
									
									double distance = sqrt((w * w) + (h * h));
									
									if(distance < (particles[i].size * 0.5)){
										
										selectedParticle = i;
										
									}
									
								}
								
							}
							
							mouseDown = event;
							
						}
						
					}
					
					break;
					
				case SDL_MOUSEBUTTONUP:
				
					isHoldingDown = 0;
					buttonPressed = -1;
					
					if(selectedParticle > -1){
						
						particles[selectedParticle].velocityX = velocityXToChange;
						particles[selectedParticle].velocityY = velocityYToChange;
						
						if(particles[selectedParticle].bondingWith > -1){
							
							// Equal amounts of force are put on both particles in a single bond
							particles[particles[selectedParticle].bondingWith].velocityX = velocityXToChange;
							particles[particles[selectedParticle].bondingWith].velocityY = velocityYToChange;
							
						}
						
						selectedParticle = -1;
						
					}
					
					break;
					
				case SDL_MOUSEMOTION:
				
					// need to check if the mouse is inside the window
					// (there is a bug in SDL, sometimes when the user
					// clicks outside the window SDL will still capture it)
					if(!(event.motion.x > options->WINDOW_WIDTH || 
						event.motion.x < 0.0 || 
						event.motion.y > options->WINDOW_HEIGHT || 
						event.motion.y < 0.0)){
						
						mouseDown.button.x = event.motion.x;
						mouseDown.button.y = event.motion.y;
						
					}
					
					break;
					
				case SDL_KEYDOWN:
				
					if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE){
						
						isRunning = 0;
						
					}
					
					break;
					
				case SDL_QUIT:
					
					isRunning = 0;
					
			}
			
		}
		
		// check if user is holding down left mouse button
		// or pressing the screen
		if(isHoldingDown){
			
			if(options->ENABLE_GENERATE_ONCE){
				
				isHoldingDown = 0;
				
			}
			
			if(mode == addParticle){
				
				generateRandomParticles(mouseDown.button.x, mouseDown.button.y);
				
			}
			
		}
		
		// add particles every frame, regardless of input from user
		if(options->ENABLE_AUTO_ADD_PARTICLES){
			
			generateRandomParticles(-1, -1);
			
		}
		
		// update each particle and handle border and particle collisions
		if(isSimulating){
			
			updateParticles();
			
			handleBorderCollision();
			
			handleParticleInteraction();
			
		}
		
		// clear the screen first
		SDL_SetRenderDrawColor(winRend, (Uint8)options->BACKGROUND_COL_R, 
			(Uint8)options->BACKGROUND_COL_G, (Uint8)options->BACKGROUND_COL_B, 255);
		
		SDL_RenderClear(winRend);
		
		drawParticles();
		
		// draw line to add velocity to the selected particle
		if(selectedParticle > -1){
			
			drawSelectedParticle();
			
		}
		
		// draw the buttons
		drawButtons();
		
		// flip buffers
		SDL_RenderPresent(winRend);
		
		// take the next delta clock
		endFrameTick = SDL_GetPerformanceCounter();
		delta = (double)(endFrameTick - startFrameTick) / tickSpeed;
		
		if(options->ENABLE_BENCHMARK){
			
			// will stop the program when the FPS drops below 
			// the specified amount and writes the amount of 
			// particles to a file called benchmark.txt, the amount of memory used
			// and the the amount of different particles we have
			if(delta > options->MAX_BENCHMARK_SPF){
				
				FILE* benchmark = fopen("benchmark.txt", "w+");
				fprintf(benchmark, "\n%s%.2f%s%d\n", "Number of particles visible at ", options->MAX_BENCHMARK_SPF, " seconds per frame: ", length);
				fprintf(benchmark, "%s%d%s\n", "Memory used: ", (int)(sizeof(particle) * (size_t)length), " bytes");
				
				fclose(benchmark);
				isRunning = 0;
				
			}
			
		}
		
	}
	
	fclose(debug);
	
	// free all memory
	SDL_DestroyRenderer(winRend);
	winRend = 0;
	
	SDL_DestroyWindow(win);
	win = 0;
	
	free(options);
	options = 0;
	
	free(buttons);
	buttons = 0;
	
	free(particles);
	particles = 0;
	
	free(bonding);
	bonding = 0;
	
	SDL_Quit();
	
	return 0;
	
}