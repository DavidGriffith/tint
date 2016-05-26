
/*
 * Copyright (c) Abraham vd Merwe <abz@blio.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "typedefs.h"
#include "utils.h"
#include "io.h"
#include "engine.h"

/*
 * Global variables
 */

const shapes_t SHAPES =
{
   { COLOR_CYAN,    0, FALSE, { {  1,  0 }, {  0,  0 }, {  0, -1 }, { -1, -1 } } },
   { COLOR_GREEN,   1, FALSE, { {  1, -1 }, {  0, -1 }, {  0,  0 }, { -1,  0 } } },
   { COLOR_YELLOW,  2, FALSE, { { -1,  0 }, {  0,  0 }, {  1,  0 }, {  0,  1 } } },
   { COLOR_BLUE,    3, FALSE, { { -1, -1 }, {  0, -1 }, { -1,  0 }, {  0,  0 } } },
   { COLOR_MAGENTA, 4, FALSE, { { -1,  1 }, { -1,  0 }, {  0,  0 }, {  1,  0 } } },
   { COLOR_WHITE,   5, FALSE, { {  1,  1 }, {  1,  0 }, {  0,  0 }, { -1,  0 } } },
   { COLOR_RED,     6, FALSE, { { -1,  0 }, {  0,  0 }, {  1,  0 }, {  2,  0 } } }
};

/*
 * Functions
 */

/* This rotates a shape */
static void real_rotate (shape_t *shape,bool clockwise)
{
   int i,tmp;
   if (clockwise)
	 {
		for (i = 0; i < NUMBLOCKS; i++)
		  {
			 tmp = shape->block[i].x;
			 shape->block[i].x = -shape->block[i].y;
			 shape->block[i].y = tmp;
		  }
	 }
   else
	 {
		for (i = 0; i < NUMBLOCKS; i++)
		  {
			 tmp = shape->block[i].x;
			 shape->block[i].x = shape->block[i].y;
			 shape->block[i].y = -tmp;
		  }
	 }
}

/* Rotate shapes the way tetris likes it (= not mathematically correct) */
static void fake_rotate (shape_t *shape)
{
   switch (shape->type)
	 {
	  case 0:	/* Just rotate this one anti-clockwise and clockwise */
		if (shape->flipped) real_rotate (shape,TRUE); else real_rotate (shape,FALSE);
		shape->flipped = !shape->flipped;
		break;
	  case 1:	/* Just rotate these two clockwise and anti-clockwise */
	  case 6:
		if (shape->flipped) real_rotate (shape,FALSE); else real_rotate (shape,TRUE);
		shape->flipped = !shape->flipped;
		break;
	  case 2:	/* Rotate these three anti-clockwise */
	  case 4:
	  case 5:
		real_rotate (shape,FALSE);
		break;
	  case 3:	/* This one is not rotated at all */
	 }
}

/* Draw a shape on the board */
static void drawshape (board_t board,shape_t *shape,int x,int y)
{
   int i;
   for (i = 0; i < NUMBLOCKS; i++) board[x + shape->block[i].x][y + shape->block[i].y] = shape->color;
}

/* Erase a shape from the board */
static void eraseshape (board_t board,shape_t *shape,int x,int y)
{
   int i;
   for (i = 0; i < NUMBLOCKS; i++) board[x + shape->block[i].x][y + shape->block[i].y] = COLOR_BLACK;
}

/* Check if shape is allowed to be in this position */
static bool allowed (board_t board,shape_t *shape,int x,int y)
{
   int i,occupied = FALSE;
   for (i = 0; i < NUMBLOCKS; i++) if (board[x + shape->block[i].x][y + shape->block[i].y]) occupied = TRUE;
   return (!occupied);
}

/* Move the shape left if possible */
static bool shape_left (board_t board,shape_t *shape,int *x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,*x,y);
   if (allowed (board,shape,*x - 1,y))
	 {
		(*x)--;
		result = TRUE;
	 }
   drawshape (board,shape,*x,y);
   return result;
}

/* Move the shape right if possible */
static bool shape_right (board_t board,shape_t *shape,int *x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,*x,y);
   if (allowed (board,shape,*x + 1,y))
	 {
		(*x)++;
		result = TRUE;
	 }
   drawshape (board,shape,*x,y);
   return result;
}

/* Rotate the shape if possible */
static bool shape_rotate (board_t board,shape_t *shape,int x,int y)
{
   bool result = FALSE;
   shape_t test;
   eraseshape (board,shape,x,y);
   memcpy (&test,shape,sizeof (shape_t));
   fake_rotate (&test);
   if (allowed (board,&test,x,y))
	 {
		memcpy (shape,&test,sizeof (shape_t));
		result = TRUE;
	 }
   drawshape (board,shape,x,y);
   return result;
}

/* Move the shape one row down if possible */
static bool shape_down (board_t board,shape_t *shape,int x,int *y)
{
   bool result = FALSE;
   eraseshape (board,shape,x,*y);
   if (allowed (board,shape,x,*y + 1))
	 {
		(*y)++;
		result = TRUE;
	 }
   drawshape (board,shape,x,*y);
   return result;
}

/* Check if shape can move down (= in the air) or not (= at the bottom */
/* of the board or on top of one of the resting shapes) */
static bool shape_bottom (board_t board,shape_t *shape,int x,int y)
{
   bool result = FALSE;
   eraseshape (board,shape,x,y);
   result = !allowed (board,shape,x,y + 1);
   drawshape (board,shape,x,y);
   return result;
}

/* Drop the shape until it comes to rest on the bottom of the board or */
/* on top of a resting shape */
static int shape_drop (board_t board,shape_t *shape,int x,int *y)
{
   int droppedlines = 0;
   eraseshape (board,shape,x,*y);
   while (allowed (board,shape,x,*y + 1))
	 {
		(*y)++;
		droppedlines++;
	 }
   drawshape (board,shape,x,*y);
   return droppedlines;
}

/* This removes all the rows on the board that is completely filled with blocks */
static int droplines (board_t board)
{
   int i,x,y,ny,status,droppedlines;
   board_t newboard;
   /* initialize new board */
   memset (newboard,0,sizeof (board_t));
   for (i = 0; i < NUMCOLS; i++) newboard[i][NUMROWS - 1] = newboard[i][NUMROWS - 2] = WALL;
   for (i = 0; i < NUMROWS; i++) newboard[0][i] = newboard[NUMCOLS - 1][i] = newboard[NUMCOLS - 2][i] = WALL;
   /* ... */
   ny = NUMROWS - 3;
   droppedlines = 0;
   for (y = NUMROWS - 3; y > 0; y--)
	 {
		status = 0;
		for (x = 1; x < NUMCOLS - 2; x++) if (board[x][y]) status++;
		if (status < NUMCOLS - 3)
		  {
			 for (x = 1; x < NUMCOLS - 2; x++) newboard[x][ny] = board[x][y];
			 ny--;
		  }
		else droppedlines++;
	 }
   memcpy (board,newboard,sizeof (board_t));
   return droppedlines;
}

/*
 * Initialize specified tetris engine
 */
void engine_init (engine_t *engine,void (*score_function)(engine_t *))
{
   int i;
   engine->score_function = score_function;
   /* intialize values */
   engine->curx = 5;
   engine->cury = 1;
   engine->curshape = rand_value (NUMSHAPES);
   engine->nextshape = rand_value (NUMSHAPES);
   engine->score = 0;
   engine->status.moves = engine->status.rotations = engine->status.dropcount = engine->status.efficiency = engine->status.droppedlines = 0;
   /* initialize shapes */
   memcpy (engine->shapes,SHAPES,sizeof (shapes_t));
   /* initialize board */
   memset (engine->board,0,sizeof (board_t));
   for (i = 0; i < NUMCOLS; i++) engine->board[i][NUMROWS - 1] = engine->board[i][NUMROWS - 2] = WALL;
   for (i = 0; i < NUMROWS; i++) engine->board[0][i] = engine->board[NUMCOLS - 1][i] = engine->board[NUMCOLS - 2][i] = WALL;
}

/*
 * Perform the given action on the specified tetris engine
 */
void engine_move (engine_t *engine,action_t action)
{
   switch (action)
	 {
		/* move shape to the left if possible */
	  case ACTION_LEFT:
		if (shape_left (engine->board,&engine->shapes[engine->curshape],&engine->curx,engine->cury)) engine->status.moves++;
		break;
		/* rotate shape if possible */
	  case ACTION_ROTATE:
		if (shape_rotate (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury)) engine->status.rotations++;
		break;
		/* move shape to the right if possible */
	  case ACTION_RIGHT:
		if (shape_right (engine->board,&engine->shapes[engine->curshape],&engine->curx,engine->cury)) engine->status.moves++;
		break;
		/* drop shape to the bottom */
	  case ACTION_DROP:
		engine->status.dropcount += shape_drop (engine->board,&engine->shapes[engine->curshape],engine->curx,&engine->cury);
	 }
}

/*
 * Evaluate the status of the specified tetris engine
 *
 * OUTPUT:
 *   1 = shape moved down one line
 *   0 = shape at bottom, next one released
 *  -1 = game over (board full)
 */
int engine_evaluate (engine_t *engine)
{
   if (shape_bottom (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury))
	 {
		/* increase score */
		engine->score_function (engine);
		/* update status information */
		engine->status.droppedlines += droplines (engine->board);
		engine->curx -= 5;
		engine->curx = abs (engine->curx);
		engine->status.rotations = 4 - engine->status.rotations;
		engine->status.rotations = engine->status.rotations > 0 ? 0 : engine->status.rotations;
		engine->status.efficiency += engine->status.dropcount + engine->status.rotations + (engine->curx - engine->status.moves);
		engine->status.efficiency >>= 1;
		engine->status.dropcount = engine->status.rotations = engine->status.moves = 0;
		/* intialize values */
		engine->curx = 5;
		engine->cury = 1;
		engine->curshape = engine->nextshape;
		engine->nextshape = rand_value (NUMSHAPES);
		/* initialize shapes */
		memcpy (engine->shapes,SHAPES,sizeof (shapes_t));
		/* return games status */
		return allowed (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury) ? 0 : -1;
	 }
   shape_down (engine->board,&engine->shapes[engine->curshape],engine->curx,&engine->cury);
   return 1;
}

