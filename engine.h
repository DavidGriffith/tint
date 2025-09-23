#ifndef ENGINE_H
#define ENGINE_H

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

#include "typedefs.h"		/* bool */

/*
 * Macros
 */

/* Number of shapes in the game */
#define NUMSHAPES	7

/* Number of blocks in each shape */
#define NUMBLOCKS	4

/* Number of rows and columns in board */
#define NUMROWS	23
#define NUMCOLS	13

/* Wall id - Arbitrary, but shouldn't have the same value as one of the colors */
#define WALL 16

/*
 * Type definitions
 */

typedef int board_t[NUMCOLS][NUMROWS];

typedef struct
{
   int x,y;
} block_t;

typedef struct
{
   int color;
   int type;
   bool flipped;
   block_t block[NUMBLOCKS];
} shape_t,shapes_t[NUMSHAPES];

typedef struct
{
   int moves;
   int rotations;
   int dropcount;
   int efficiency;
   int droppedlines;
   int currentdroppedlines;
} status_t;

typedef struct engine_struct
{
   bool shadow;                                     /* show shadow */
   int curx,cury,curx_shadow,cury_shadow;			/* coordinates of current piece */
   int curshape,nextshape;							/* current & next shapes */
   int score;										/* score */
   int bag_iterator;								/* iterator for randomized bag */
   int bag[NUMSHAPES];								/* pointer to bag of shapes */
   shapes_t shapes;									/* shapes */
   board_t board;									/* board */
   status_t status;									/* current status of shapes */
   void (*score_function)(struct engine_struct *);	/* score function */
} engine_t;

typedef enum { ACTION_LEFT, ACTION_ROTATE_CLOCKWISE, ACTION_ROTATE_COUNTERCLOCKWISE, ACTION_RIGHT, ACTION_DROP, ACTION_DOWN } action_t;

/*
 * Global variables
 */

extern const shapes_t SHAPES;

/*
 * Functions
 */

/*
 * Initialize specified tetris engine
 */
void engine_init (engine_t *engine,void (*score_function)(engine_t *));

/*
 * Perform the given action on the specified tetris engine
 */
void engine_move (engine_t *engine,action_t action);

/*
 * Evaluate the status of the specified tetris engine
 *
 * OUTPUT:
 *   1 = shape moved down one line
 *   0 = shape at bottom, next one released
 *  -1 = game over (board full)
 */
int engine_evaluate (engine_t *engine);

#endif	/* #ifndef ENGINE_H */
