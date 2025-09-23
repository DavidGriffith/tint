
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

#include <stdlib.h>
#include <string.h>

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
static void fake_rotate (shape_t *shape,bool clockwise)
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
	  case 2:	/* Rotate these three according to supplied direction */
	  case 4:
	  case 5:
		real_rotate (shape,clockwise);
		break;
	  case 3:	/* This one is not rotated at all */
		break;
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

/* Set y coordinate of shadow */
static void place_shadow_to_bottom (board_t board,shape_t *shape,int x_shadow,int *y_shadow,int y) {
   while (allowed(board,shape,x_shadow,y+1)) y++;
   *y_shadow = y;
}

/* Move the shape left if possible */
static bool shape_left (engine_t *engine)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   bool result = FALSE;
   eraseshape (*board,shape,engine->curx,engine->cury);
   if (engine->shadow) eraseshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   if (allowed (*board,shape,engine->curx - 1,engine->cury))
	 {
        engine->curx--;
        result = TRUE;
        if (engine->shadow)
        {
            engine->curx_shadow--;
            place_shadow_to_bottom(*board,shape,engine->curx_shadow,&engine->cury_shadow,engine->cury);
        }
	 }
   if (engine->shadow) drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   drawshape (*board,shape,engine->curx,engine->cury);
   return result;
}

/* Move the shape right if possible */
static bool shape_right (engine_t *engine)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   bool result = FALSE;
   eraseshape (*board,shape,engine->curx,engine->cury);
   if (engine->shadow) eraseshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   if (allowed (*board,shape,engine->curx + 1,engine->cury))
	 {
		engine->curx++;
		result = TRUE;
		if (engine->shadow)
		{
            engine->curx_shadow++;
            place_shadow_to_bottom(*board,shape,engine->curx_shadow,&engine->cury_shadow,engine->cury);
		}
	 }
   if (engine->shadow) drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   drawshape (*board,shape,engine->curx,engine->cury);
   return result;
}

/* Rotate the shape if possible */
static bool shape_rotate (engine_t *engine,bool clockwise)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   bool result = FALSE;
   shape_t test;
   eraseshape (*board,shape,engine->curx,engine->cury);
   if (engine->shadow) eraseshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   memcpy (&test,shape,sizeof (shape_t));
   fake_rotate (&test,clockwise);
   if (allowed (*board,&test,engine->curx,engine->cury))
	 {
		memcpy (shape,&test,sizeof (shape_t));
		result = TRUE;
		if (engine->shadow) place_shadow_to_bottom(*board,shape,engine->curx_shadow,&engine->cury_shadow,engine->cury);
	 }
   if (engine->shadow) drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   drawshape (*board,shape,engine->curx,engine->cury);
   return result;
}

/* Move the shape one row down if possible */
static bool shape_down (engine_t *engine)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   bool result = FALSE;
   eraseshape (*board,shape,engine->curx,engine->cury);
   if (engine->shadow) eraseshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   if (allowed (*board,shape,engine->curx,engine->cury + 1))
	 {
		engine->cury++;
		result = TRUE;
		if (engine->shadow) place_shadow_to_bottom(*board,shape,engine->curx_shadow,&engine->cury_shadow,engine->cury);
	 }
   if (engine->shadow) drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   drawshape (*board,shape,engine->curx,engine->cury);
   return result;
}

/* Check if shape can move down (= in the air) or not (= at the bottom */
/* of the board or on top of one of the resting shapes) */
static bool shape_bottom (engine_t *engine)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   bool result = FALSE;
   eraseshape (*board,shape,engine->curx,engine->cury);
   if (engine->shadow) eraseshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   result = !allowed (*board,shape,engine->curx,engine->cury + 1);
   if (engine->shadow) drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
   drawshape (*board,shape,engine->curx,engine->cury);
   return result;
}

/* Drop the shape until it comes to rest on the bottom of the board or */
/* on top of a resting shape */
static int shape_drop (engine_t *engine)
{
   board_t *board = &engine->board;
   shape_t *shape = &engine->shapes[engine->curshape];
   eraseshape (*board,shape,engine->curx,engine->cury);
   int droppedlines = 0;

   if (engine->shadow) {
       drawshape (*board,shape,engine->curx_shadow,engine->cury_shadow);
       droppedlines = engine->cury_shadow - engine->cury;
       engine->cury = engine->cury_shadow;
       return droppedlines;
   }

   while (allowed (*board,shape,engine->curx,engine->cury + 1))
	 {
		engine->cury++;
		droppedlines++;
	 }
   drawshape (*board,shape,engine->curx,engine->cury);
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

/* shuffle int array */
void shuffle (int *array, size_t n)
{
   size_t i;
   for (i = 0; i < n - 1; i++)
   {
      int range = (int)(n - i);
      size_t j = i + rand_value(range);
      int t = array[j];
      array[j] = array[i];
      array[i] = t;
   }
}

/*
 * Initialize specified tetris engine
 */
void engine_init (engine_t *engine,void (*score_function)(engine_t *))
{
   int i;
   engine->shadow = FALSE;
   engine->score_function = score_function;
   /* intialize values */
   engine->curx = 5;
   engine->cury = 1;
   engine->curx_shadow = 5;
   engine->cury_shadow = 1;
   engine->bag_iterator = 0;
   /* create and randomize bag */
   for (int j = 0; j < NUMSHAPES; j++) engine->bag[j] = j;
   shuffle (engine->bag,NUMSHAPES);
   engine->curshape = engine->bag[engine->bag_iterator%NUMSHAPES];
   engine->nextshape = engine->bag[(engine->bag_iterator+1)%NUMSHAPES];
   engine->bag_iterator++;
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
        if (shape_left (engine)) engine->status.moves++;
		break;
		/* rotate shape clockwise if possible */
	  case ACTION_ROTATE_CLOCKWISE:
		if (shape_rotate (engine, true)) engine->status.rotations++;
		break;
		/* rotate shape counterclockwise if possible */
	  case ACTION_ROTATE_COUNTERCLOCKWISE:
		if (shape_rotate (engine, false)) engine->status.rotations++;
		break;
		/* move shape to the right if possible */
	  case ACTION_RIGHT:
	    if (shape_right (engine)) engine->status.moves++;
		break;
		/* move shape to the down if possible */
	  case ACTION_DOWN:
		if (shape_down (engine)) engine->status.moves++;
		break;
		/* drop shape to the bottom */
	  case ACTION_DROP:
		engine->status.dropcount += shape_drop (engine);
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
   if (shape_bottom (engine))
	 {
		/* update status information */
		int dropped_lines = droplines(engine->board);
		engine->status.droppedlines += dropped_lines;
		engine->status.currentdroppedlines = dropped_lines;
		/* increase score */
		engine->score_function (engine);
		engine->curx -= 5;
		engine->curx = abs (engine->curx);
		engine->curx_shadow -= 5;
		engine->curx_shadow = abs (engine->curx_shadow);
		engine->status.rotations = 4 - engine->status.rotations;
		engine->status.rotations = engine->status.rotations > 0 ? 0 : engine->status.rotations;
		engine->status.efficiency += engine->status.dropcount + engine->status.rotations + (engine->curx - engine->status.moves);
		engine->status.efficiency >>= 1;
		engine->status.dropcount = engine->status.rotations = engine->status.moves = 0;
		/* intialize values */
		engine->curx = 5;
		engine->cury = 1;
		engine->curx_shadow = 5;
		engine->cury_shadow = 1;
		engine->curshape = engine->bag[engine->bag_iterator%NUMSHAPES];
		/* shuffle bag before first item in bag would be reused */
		if ((engine->bag_iterator+1) % NUMSHAPES == 0) shuffle(engine->bag, NUMSHAPES);
		engine->nextshape = engine->bag[(engine->bag_iterator+1)%NUMSHAPES];
		engine->bag_iterator++;
		/* initialize shapes */
		memcpy (engine->shapes,SHAPES,sizeof (shapes_t));
		/* return games status */
		return allowed (engine->board,&engine->shapes[engine->curshape],engine->curx,engine->cury) ? 0 : -1;
	 }
   shape_down (engine);
   return 1;
}
