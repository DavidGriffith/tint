
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "typedefs.h"
#include "utils.h"
#include "io.h"
#include "config.h"
#include "engine.h"

/*
 * Macros
 */

/* Upper left corner of board */
#define XTOP ((out_width () - NUMROWS - 3) >> 1)
#define YTOP ((out_height () - NUMCOLS - 9) >> 1)

/* Maximum digits in a number (i.e. number of digits in score, */
/* number of blocks, etc. should not exceed this value */
#define MAXDIGITS 5

/* Number of levels in the game */
#define MINLEVEL	1
#define MAXLEVEL	9

/* This calculates the time allowed to move a shape, before it is moved a row down */
#define DELAY (1000000 / (level + 2))

/* The score is multiplied by this to avoid losing precision */
#define SCOREFACTOR 2

/* This calculates the stored score value */
#define SCOREVAL(x) (SCOREFACTOR * (x))

/* This calculates the real (displayed) value of the score */
#define GETSCORE(score) ((score) / SCOREFACTOR)

static bool shownext;
static bool dottedlines;
static int level = MINLEVEL - 1,shapecount[NUMSHAPES];
static char blockchar = ' ';

/*
 * Functions
 */

/* This function is responsible for increasing the score appropriately whenever
 * a block collides at the bottom of the screen (or the top of the heap */
static void score_function (engine_t *engine)
{
   int score = SCOREVAL (level * (engine->status.dropcount + 1));
   score += SCOREVAL ((level + 10) * engine->status.currentdroppedlines * engine->status.currentdroppedlines);

   if (shownext) score /= 2;
   if (dottedlines) score /= 2;

   engine->score += score;
}

/* Draw the board on the screen */
static void drawboard (board_t board)
{
   int x,y;
   out_setattr (ATTR_OFF);
   for (y = 1; y < NUMROWS - 1; y++) for (x = 0; x < NUMCOLS - 1; x++)
	 {
		out_gotoxy (XTOP + x * 2,YTOP + y);
		switch (board[x][y])
		  {
			 /* Wall */
		   case WALL:
			 out_setattr (ATTR_BOLD);
			 out_setcolor (COLOR_BLUE,COLOR_BLACK);
			 out_putch ('<');
			 out_putch ('>');
			 out_setattr (ATTR_OFF);
			 break;
			 /* Background */
		   case 0:
			 if (dottedlines)
			   {
				  out_setcolor (COLOR_BLUE,COLOR_BLACK);
				  out_putch ('.');
				  out_putch (' ');
			   }
			 else
			   {
				  out_setcolor (COLOR_BLACK,COLOR_BLACK);
				  out_putch (' ');
				  out_putch (' ');
			   }
			 break;
			 /* Block */
		   default:
			 out_setcolor (COLOR_BLACK,board[x][y]);
			 out_putch (blockchar);
			 out_putch (blockchar);
		  }
	 }
   out_setattr (ATTR_OFF);
}

/* Show the next piece on the screen */
static void drawnext (int shapenum,int x,int y)
{
   int i;
   block_t ofs[NUMSHAPES] =
	 { { 1,  0 }, { 1,  0 }, { 1, -1 }, { 2,  0 }, { 1, -1 }, { 1, -1 }, { 0, -1 } };
   out_setcolor (COLOR_BLACK,COLOR_BLACK);
   for (i = y - 2; i < y + 2; i++)
	 {
		out_gotoxy (x - 2,i);
		out_printf ("        ");
	 }
   out_setcolor (COLOR_BLACK,SHAPES[shapenum].color);
   for (i = 0; i < NUMBLOCKS; i++)
	 {
		out_gotoxy (x + SHAPES[shapenum].block[i].x * 2 + ofs[shapenum].x,
					y + SHAPES[shapenum].block[i].y + ofs[shapenum].y);
		out_putch (' ');
		out_putch (' ');
	 }
}

/* Draw the background */
static void drawbackground ()
{
   out_setattr (ATTR_OFF);
   out_setcolor (COLOR_WHITE,COLOR_BLACK);
   out_gotoxy (4,YTOP + 7);   out_printf ("H E L P");
   out_gotoxy (1,YTOP + 9);   out_printf ("p: Pause");
   out_gotoxy (1,YTOP + 10);  out_printf ("j: Left");
   out_gotoxy (1,YTOP + 11);  out_printf ("l: Right");
   out_gotoxy (1,YTOP + 12);  out_printf ("k: Rotate");
   out_gotoxy (1,YTOP + 13);  out_printf ("s: Draw next");
   out_gotoxy (1,YTOP + 14);  out_printf ("d: Toggle lines");
   out_gotoxy (1,YTOP + 15);  out_printf ("a: Speed up");
   out_gotoxy (1,YTOP + 16);  out_printf ("q: Quit");
   out_gotoxy (2,YTOP + 17);  out_printf ("SPACE: Drop");
   out_gotoxy (3,YTOP + 19);  out_printf ("Next:");
}

static int getsum ()
{
   int i,sum = 0;
   for (i = 0; i < NUMSHAPES; i++) sum += shapecount[i];
   return (sum);
}

/* This show the current status of the game */
static void showstatus (engine_t *engine)
{
   static const int shapenum[NUMSHAPES] = { 4, 6, 5, 1, 0, 3, 2 };
   char tmp[MAXDIGITS + 1];
   int i,sum = getsum ();
   out_setattr (ATTR_OFF);
   out_setcolor (COLOR_WHITE,COLOR_BLACK);
   out_gotoxy (1,YTOP + 1);   out_printf ("Your level: %d",level);
   out_gotoxy (1,YTOP + 2);   out_printf ("Full lines: %d",engine->status.droppedlines);
   out_gotoxy (2,YTOP + 4);   out_printf ("Score");
   out_setattr (ATTR_BOLD);
   out_setcolor (COLOR_YELLOW,COLOR_BLACK);
   out_printf ("  %d",GETSCORE (engine->score));
   if (shownext) drawnext (engine->nextshape,3,YTOP + 22);
   out_setattr (ATTR_OFF);
   out_setcolor (COLOR_WHITE,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 12,YTOP + 1);
   out_printf ("STATISTICS");
   out_setcolor (COLOR_BLACK,COLOR_MAGENTA);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 3);
   out_printf ("      ");
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 4);
   out_printf ("  ");
   out_setcolor (COLOR_MAGENTA,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 3);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[0]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 3);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_BLACK,COLOR_RED);
   out_gotoxy (out_width () - MAXDIGITS - 13,YTOP + 5);
   out_printf ("        ");
   out_setcolor (COLOR_RED,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 5);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[1]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 5);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_BLACK,COLOR_WHITE);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 7);
   out_printf ("      ");
   out_gotoxy (out_width () - MAXDIGITS - 13,YTOP + 8);
   out_printf ("  ");
   out_setcolor (COLOR_WHITE,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 7);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[2]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 7);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_BLACK,COLOR_GREEN);
   out_gotoxy (out_width () - MAXDIGITS - 9,YTOP + 9);
   out_printf ("    ");
   out_gotoxy (out_width () - MAXDIGITS - 11,YTOP + 10);
   out_printf ("    ");
   out_setcolor (COLOR_GREEN,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 9);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[3]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 9);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_BLACK,COLOR_CYAN);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 11);
   out_printf ("    ");
   out_gotoxy (out_width () - MAXDIGITS - 15,YTOP + 12);
   out_printf ("    ");
   out_setcolor (COLOR_CYAN,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 11);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[4]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 11);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_BLACK,COLOR_BLUE);
   out_gotoxy (out_width () - MAXDIGITS - 9,YTOP + 13);
   out_printf ("    ");
   out_gotoxy (out_width () - MAXDIGITS - 9,YTOP + 14);
   out_printf ("    ");
   out_setcolor (COLOR_BLUE,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 13);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[5]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 13);
   out_printf ("%s",tmp);
   out_setattr (ATTR_OFF);
   out_setcolor (COLOR_BLACK,COLOR_YELLOW);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 15);
   out_printf ("      ");
   out_gotoxy (out_width () - MAXDIGITS - 15,YTOP + 16);
   out_printf ("  ");
   out_setcolor (COLOR_YELLOW,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 3,YTOP + 15);
   out_putch ('-');
   snprintf (tmp,MAXDIGITS + 1,"%d",shapecount[shapenum[6]]);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 15);
   out_printf ("%s",tmp);
   out_setcolor (COLOR_WHITE,COLOR_BLACK);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 17);
   for (i = 0; i < MAXDIGITS + 16; i++) out_putch ('-');
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 18);
   out_printf ("Sum          :");
   snprintf (tmp,MAXDIGITS + 1,"%d",sum);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 18);
   out_printf ("%s",tmp);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 20);
   for (i = 0; i < MAXDIGITS + 16; i++) out_putch (' ');
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 20);
   out_printf ("Score ratio  :");
   snprintf (tmp,MAXDIGITS + 1,"%d",GETSCORE (engine->score) / sum);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 20);
   out_printf ("%s",tmp);
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 21);
   for (i = 0; i < MAXDIGITS + 16; i++) out_putch (' ');
   out_gotoxy (out_width () - MAXDIGITS - 17,YTOP + 21);
   out_printf ("Efficiency   :");
   snprintf (tmp,MAXDIGITS + 1,"%d",engine->status.efficiency);
   out_gotoxy (out_width () - strlen (tmp) - 1,YTOP + 21);
   out_printf ("%s",tmp);
}

          /***************************************************************************/
          /***************************************************************************/
          /***************************************************************************/

/* Header for scorefile */
#define SCORE_HEADER	"Tint 0.02b (c) Abraham vd Merwe - Scores"

/* Header for score title */
static const char scoretitle[] = "\n\t   TINT HIGH SCORES\n\n\tRank   Score        Name\n\n";

/* Length of a player's name */
#define NAMELEN 20

/* Number of scores allowed in highscore list */
#define NUMSCORES 10

typedef struct
{
   char name[NAMELEN];
   int score;
   time_t timestamp;
} score_t;

static void getname (char *name)
{
   struct passwd *pw = getpwuid (geteuid ());

   fprintf (stderr,"Congratulations! You have a new high score.\n");
   fprintf (stderr,"Enter your name [%s]: ",pw != NULL ? pw->pw_name : "");

   fgets (name,NAMELEN - 1,stdin);
   name[strlen (name) - 1] = '\0';

   if (!strlen (name) && pw != NULL)
	 {
		strncpy (name,pw->pw_name,NAMELEN);
		name[NAMELEN - 1] = '\0';
	 }
}

static void err1 ()
{
   fprintf (stderr,"Error creating %s\n",scorefile);
   exit (EXIT_FAILURE);
}

static void err2 ()
{
   fprintf (stderr,"Error writing to %s\n",scorefile);
   exit (EXIT_FAILURE);
}

void showplayerstats (engine_t *engine)
{
   fprintf (stderr,
			"\n\t   PLAYER STATISTICS\n\n\t"
			"Score       %11d\n\t"
			"Efficiency  %11d\n\t"
			"Score ratio %11d\n",
			GETSCORE (engine->score),engine->status.efficiency,GETSCORE (engine->score) / getsum ());
}

static void createscores (int score)
{
   FILE *handle;
   int i,j;
   score_t scores[NUMSCORES];
   char header[strlen (SCORE_HEADER)+1];
   if (score == 0) return;	/* No need saving this */
   for (i = 1; i < NUMSCORES; i++)
	 {
		strcpy (scores[i].name,"None");
		scores[i].score = -1;
		scores[i].timestamp = 0;
	 }
   getname (scores[0].name);
   scores[0].score = score;
   scores[0].timestamp = time (NULL);
   if ((handle = fopen (scorefile,"w")) == NULL) err1 ();
   strcpy (header,SCORE_HEADER);
   i = fwrite (header,strlen (SCORE_HEADER),1,handle);
   if (i != 1) err2 ();
   for (i = 0; i < NUMSCORES; i++)
	 {
		j = fwrite (scores[i].name,strlen (scores[i].name) + 1,1,handle);
		if (j != 1) err2 ();
		j = fwrite (&(scores[i].score),sizeof (int),1,handle);
		if (j != 1) err2 ();
		j = fwrite (&(scores[i].timestamp),sizeof (time_t),1,handle);
		if (j != 1) err2 ();
	 }
   fclose (handle);

   fprintf (stderr,"%s",scoretitle);
   fprintf (stderr,"\t  1* %7d        %s\n\n",score,scores[0].name);
}

static int cmpscores (const void *a,const void *b)
{
   int result;
   result = (int) ((score_t *) a)->score - (int) ((score_t *) b)->score;
   /* a < b */
   if (result < 0) return 1;
   /* a > b */
   if (result > 0) return -1;
   /* a = b */
   result = (time_t) ((score_t *) a)->timestamp - (time_t) ((score_t *) b)->timestamp;
   /* a is older */
   if (result < 0) return -1;
   /* b is older */
   if (result > 0) return 1;
   /* timestamps is equal */
   return 0;
}

static void savescores (int score)
{
   FILE *handle;
   int i,j,ch;
   score_t scores[NUMSCORES];
   char header[strlen (SCORE_HEADER)+1];
   time_t tmp = 0;
   if ((handle = fopen (scorefile,"r")) == NULL)
	 {
		createscores (score);
		return;
	 }
   i = fread (header,strlen (SCORE_HEADER),1,handle);
   if ((i != 1) || (strncmp (SCORE_HEADER,header,strlen (SCORE_HEADER)) != 0))
	 {
		createscores (score);
		return;
	 }
   for (i = 0; i < NUMSCORES; i++)
	 {
		j = 0;
		while ((ch = fgetc (handle)) != '\0')
		  {
			 if ((ch == EOF) || (j >= NAMELEN - 2))
			   {
				  createscores (score);
				  return;
			   }
			 scores[i].name[j++] = (char) ch;
		  }
		scores[i].name[j] = '\0';
		j = fread (&(scores[i].score),sizeof (int),1,handle);
		if (j != 1)
		  {
			 createscores (score);
			 return;
		  }
		j = fread (&(scores[i].timestamp),sizeof (time_t),1,handle);
		if (j != 1)
		  {
			 createscores (score);
			 return;
		  }
	 }
   fclose (handle);
   if (score > scores[NUMSCORES - 1].score)
	 {
		getname (scores[NUMSCORES - 1].name);
		scores[NUMSCORES - 1].score = score;
		scores[NUMSCORES - 1].timestamp = tmp = time (NULL);
	 }
   qsort (scores,NUMSCORES,sizeof (score_t),cmpscores);
   if ((handle = fopen (scorefile,"w")) == NULL) err2 ();
   strcpy (header,SCORE_HEADER);
   i = fwrite (header,strlen (SCORE_HEADER),1,handle);
   if (i != 1) err2 ();
   for (i = 0; i < NUMSCORES; i++)
	 {
		j = fwrite (scores[i].name,strlen (scores[i].name) + 1,1,handle);
		if (j != 1) err2 ();
		j = fwrite (&(scores[i].score),sizeof (int),1,handle);
		if (j != 1) err2 ();
		j = fwrite (&(scores[i].timestamp),sizeof (time_t),1,handle);
		if (j != 1) err2 ();
	 }
   fclose (handle);

   fprintf (stderr,"%s",scoretitle);
   i = 0;
   while ((i < NUMSCORES) && (scores[i].score != -1))
	 {
		j = scores[i].timestamp == tmp ? '*' : ' ';
		fprintf (stderr,"\t %2d%c %7d        %s\n",i + 1,j,scores[i].score,scores[i].name);
		i++;
	 }
   fprintf (stderr,"\n");
}

          /***************************************************************************/
          /***************************************************************************/
          /***************************************************************************/

static void showhelp ()
{
   fprintf (stderr,"USAGE: tint [-h] [-l level] [-n] [-d] [-b char]\n");
   fprintf (stderr,"  -h           Show this help message\n");
   fprintf (stderr,"  -l <level>   Specify the starting level (%d-%d)\n",MINLEVEL,MAXLEVEL);
   fprintf (stderr,"  -n           Draw next shape\n");
   fprintf (stderr,"  -d           Draw vertical dotted lines\n");
   fprintf (stderr,"  -b <char>    Use this character to draw blocks instead of spaces\n");
   exit (EXIT_FAILURE);
}

static void parse_options (int argc,char *argv[])
{
   int i = 1;
   while (i < argc)
	 {
		/* Help? */
		if (strcmp (argv[i],"-h") == 0)
		  showhelp ();
		/* Level? */
		else if (strcmp (argv[i],"-l") == 0)
		  {
			 i++;
			 if (i >= argc || !str2int (&level,argv[i])) showhelp ();
			 if ((level < MINLEVEL) || (level > MAXLEVEL))
			   {
				  fprintf (stderr,"You must specify a level between %d and %d\n",MINLEVEL,MAXLEVEL);
				  exit (EXIT_FAILURE);
			   }
		  }
		/* Show next? */
		else if (strcmp (argv[i],"-n") == 0)
		  shownext = TRUE;
		else if(strcmp(argv[i],"-d")==0)
		  dottedlines = TRUE;
		else if(strcmp(argv[i], "-b")==0)
		  {
		    i++;
		    if (i >= argc || strlen(argv[i]) < 1) showhelp();
		    blockchar = argv[i][0];
		  }
		else
		  {
			 fprintf (stderr,"Invalid option -- %s\n",argv[i]);
			 showhelp ();
		  }
		i++;
	 }
}

static void choose_level ()
{
   char buf[NAMELEN];

   do
	 {
		fprintf (stderr,"Choose a level to start [%d-%d]: ",MINLEVEL,MAXLEVEL);
		fgets (buf,NAMELEN - 1,stdin);
		buf[strlen (buf) - 1] = '\0';
	 }
   while (!str2int (&level,buf) || level < MINLEVEL || level > MAXLEVEL);
}

          /***************************************************************************/
          /***************************************************************************/
          /***************************************************************************/

int main (int argc,char *argv[])
{
   bool finished;
   int ch;
   engine_t engine;
   /* Initialize */
   rand_init ();							/* must be called before engine_init () */
   engine_init (&engine,score_function);	/* must be called before using engine.curshape */
   finished = shownext = FALSE;
   memset (shapecount,0,NUMSHAPES * sizeof (int));
   shapecount[engine.curshape]++;
   parse_options (argc,argv);				/* must be called after initializing variables */
   if (level < MINLEVEL) choose_level ();
   io_init ();
   drawbackground ();
   in_timeout (DELAY);
   /* Main loop */
   do
	 {
		/* draw shape */
		showstatus (&engine);
		drawboard (engine.board);
		out_refresh ();
		/* Check if user pressed a key */
		if ((ch = in_getch ()) != ERR)
		  {
			 switch (ch)
			   {
				case 'j':
				case KEY_LEFT:
				  engine_move (&engine,ACTION_LEFT);
				  break;
				case 'k':
				case '\n':
				  engine_move (&engine,ACTION_ROTATE);
				  break;
				case 'l':
				case KEY_RIGHT:
				  engine_move (&engine,ACTION_RIGHT);
				  break;
				case ' ':
				case KEY_DOWN:
				  engine_move (&engine,ACTION_DROP);
				  break;
				  /* show next piece */
				case 's':
				  shownext = TRUE;
				  break;
				  /* toggle dotted lines */
				case 'd':
				  dottedlines = !dottedlines;
				  break;
				  /* next level */
				case 'a':
				case KEY_UP:
				  if (level < MAXLEVEL)
					{
					   level++;
					   in_timeout (DELAY);
					}
				  else out_beep ();
				  break;
				  /* quit */
				case 'q':
				  finished = TRUE;
				  break;
				  /* pause */
				case 'p':
				  out_setcolor (COLOR_WHITE,COLOR_BLACK);
				  out_gotoxy ((out_width () - 34) / 2,out_height () - 2);
				  out_printf ("Paused - Press any key to continue");
				  while ((ch = in_getch ()) == ERR) ;	/* Wait for a key to be pressed */
				  in_flush ();							/* Clear keyboard buffer */
				  out_gotoxy ((out_width () - 34) / 2,out_height () - 2);
				  out_printf ("                                  ");
				  break;
				  /* unknown keypress */
				default:
				  out_beep ();
			   }
			 in_flush ();
		  }
		else
		  {
			 switch (engine_evaluate (&engine))
			   {
				  /* game over (board full) */
				case -1:
				  if ((level < MAXLEVEL) && ((engine.status.droppedlines / 10) > level)) level++;
				  finished = TRUE;
				  break;
				  /* shape at bottom, next one released */
				case 0:
				  if ((level < MAXLEVEL) && ((engine.status.droppedlines / 10) > level))
					{
					   level++;
					   in_timeout (DELAY);
					}
				  shapecount[engine.curshape]++;
				  break;
				  /* shape moved down one line */
				case 1:
				  break;
			   }
		  }
	 }
   while (!finished);
   /* Restore console settings and exit */
   io_close ();
   /* Don't bother the player if he want's to quit */
   if (ch != 'q')
	 {
		showplayerstats (&engine);
		savescores (GETSCORE (engine.score));
	 }
   exit (EXIT_SUCCESS);
}

