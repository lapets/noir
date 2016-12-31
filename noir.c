/***********************************************************************************************************
   noir.c
   noir terminal editor
   ver 0.7.0(a)(l)

   everything you need is contained in this file
   ncurses or some equivalent is required for this version

   compiles on most machines using some variant of:
   % gcc noir.c -o noir -Wall -lcurses

   just drop the compiled binary into your /bin/ folder to use "noir" on the command line
   don't forget to change permissions, eg.
   % chmod u+x noir

   ----- <PORTING> ---------------------------------------------------------------------------

   if you're adapting the source to another library or platform, not much work is
   involved, here's a list of what you have to do:

   - fix the #include's by removing anything linux-specific (see below) or ncurses-specific
        and including the headers for your library/platform
   - comment out #include <curses.h> (ncurses library header) and #include <signal.h> (header
        for linux-specific SIGWINCH functionality)
   - remove the sighandler typedef if you're not using #include <signal.h> anymore
   - adjust the values for keyboard constant #define's to match your platform/library;
        this is the most time-consuming part on most occasions
   - if you aren't using linux, you'll need to remove the SIGWINCH call from the
        _display_init() function; if you want to retain resize capture functionality,
        you'll need to find a platform-specific method of capturing resize events
   - adjust the get_input() function if your keyboard input library is different
   - adjust the following display library front-end functions to call
        the equivalent functions from your library:

     void _display_init()                              //display library initialization calls
     void _display_cursor_update(_cursor_inst *cursor) //updates cursor properties
     void _display_move_cursor(int row, int col)       //move cursor to specified coordinates
     void _display_dump_bare()                         //flushes changes/refreshes display
     void _display_clear_eol()                         //clears to the end of the line
     void _display_string(char* str)                   //outputs string at cursor location
     void _display_exit()                              //display library exit calls

     even if a display library does not have near equivalents for these functions,
     their intended function can probably be coded using the new library's functions

   ----- <THINGS LEFT TO DO> -----------------------------------------------------------------

   - write function for catching quit event and dump the buffer to a file (platform-
        specific)
   - smart bracing/tabbing
   - backspace multiple times to erase line for backspace+shift/ctrl functionality
   - other misc. functionality (check reserved key #def's)
   - for undo, push each character action or delete action onto stack, view it
        and reverse to undo; do not pop them off completely until a new action
        is taken etc.

   ----- <README> ----------------------------------------------------------------------------

      This terminal editor was conceived as an extremely compact, relatively portable,
      functional text editor for command line/terminal environments. Right now some
      important functionality is not yet complete, and I may not get around to it myself.
      As is the source code is about 1200 lines and under 40 KB in size; a compiled
      binary is about 25 KB in most unix/linux environments, and about 50 KB in a
      dos16/win32 environment. The environment, keyboard layout, and runtime options
      are all designed to be as intuitive and minimalist as possible.

      Currently included functionality...

       - Check the keyboard #define's at the beginning of the source for key functions
         which are already implemented.
       - The current clipboard function is as follows:

            Press the select key on one end of the text selection you want to make,
            "sel-" will be displayed on the top line. Now select another character
            either on the same line ("sel-single" will be displayed) or on a different
            line ("sel-multi." will be displayed). If you selected text on a single,
            line, the cut key will copy the stretch of text to the keyboard and delete
            it from the line. It can now be pasted anywhere in the buffer. If you
            selected points on different lines, all lines including the ones you
            selected and all lines in between will be copied to the clipboard and
            removed if the cut key is pressed. Pasting will insert these whole lines
            above the line upon which the cursor is located.

       - To edit an existing file or create a new file type "noir filename" on the
         command line; noir will confirm the creation of a new file. If you do not save
         the buffer in the new file before you quit, and you confirm that you do not
         want to save, the file will not be created.
       - If you type "noir" without a command line argument, noir will assume you
         are editing the default file "_bufdump". In this situation, your changes will
         be saved automatically upon exit.
       - If you quit and the changes in the buffer are unsaved, noir will query if you
         want to save. If you type 'N' or 'n', the current buffer will still be saved
         to the file "_bufdump" in your current working directory. If you type 'Y' or 'y'
         the buffer will be saved to the last file you listed on the command line.


      What's going on...

                                                     _ (2) indicates buffer is saved
          (1) dimensions of display_                /          _ (3) displays clip status
                                    \              /          /
       +--------------------------------------------------------------------------------+
       |noir terminal editor. display: (78, 22)   saved.  sel-multi.                    |
       |     1: ~                                                                       |
       |     2: Active text area; move your cursor to where you want to see text~       |
       |     3: and begin typing.~                                                      |
       |     4: ~                                                                       |
       |     5: The tilde represents the end of a line (defined by _ENDCHAR) -> ~       |
       |     6: The tilde is not saved to your text file, it merely replaces~           |
       |     7:    the standard '\n' symbol on the active display.~                     |
       |     8: ~                                                                       |
       |     9:                                                                         |
       |    10:                                                                         |
       |    11:                                                                         |
       |    12:                                                                         |
       |    13:                                                                         |
       |    14:                                                                         |
       |    15:                                                                         |
       |    16:                                                                         |
       |    17:                                                                         |
       |    18:                                                                         |
       |    19:                                                                         |
       |    20:                                                                         |
       |    21:                                                                         |
       |    22:                                                                         |
       |    23:                                                                         |
       | 103                                                                            |
       +--------------------------------------------------------------------------------+
          \   \_ (4) line numbers listed on left
           \
            \_ (5) the ascii code or keyboard scancode of the last keypress

      (1) The dimensions of the current display are displayed in (width, height) format.
      (2) If the current displayed buffer matches the file on disk, "saved." appears.
      (3) The current status of the clipboard functions displayed...
             sel-             ... the first coordinate has been selected, awaiting second
             sel-single       ... you have a single-line stretch of text selected
             sel-multi.       ... you have multiple lines selected
      (4) The line numbers are displayed on left. Resets to 0 when 1,000,000 is reached.
      (5) The ascii code of the last keypress is displayed. Useful for adjusting keycode
             constants, among other uses.

   ----- </README> ---------------------------------------------------------------------------

   2004/05/31 - 2004/06/09

   andrei lapets
   a@lapets.io

***********************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curses.h>        //if you can't find this in your includes, install ncurses
#include <signal.h>        //this one's only going to work in unix/linux


#ifndef    ERR
#define    ERR                -1
#endif

#define    TRUE               1
#define    FALSE              0

#define    _MAX_LINES         1000

#define    _MD_OPEN           111
#define    _MD_NEW            112
#define    _MD_QUIT           113
#define    _MD_BUF            114

#define    _BUFDUMP           "_bufdump"         //default save buffer/open buffer file
#define    _ENDCHAR           '~'                //character to display as endline
#define    _TAB_LEN           3                  //number of spaces equaling one tab

//Keyboard

#define    _KB_SHFT_BKS       8                  //kill everything on line to left    *
#define    _KB_CTRL_UDRSCR    31                 //kill everythingon line to right    *
#define    _KB_CTRL_T         20                 //time                               *
#define    _KB_CTRL_H         8                  //goto                               *
#define    _KB_CTRL_F         6                  //find                               *
#define    _KB_CTRL_R         18                 //replace                            *
#define    _KB_CTRL_Z         26                 //undo                               *
#define    _KB_CTRL_A         1                  //redo                               *

#define    _KB_ENT_N          '\n'               //catch both just in case
#define    _KB_TB             '\t'               //tab key
#define    _KB_ESC            27                 //quit

#define    _KB_CTRL_D         4                  //select
#define    _KB_CTRL_X         24                 //cut
#define    _KB_CTRL_V         22                 //paste

#define    _KB_CTRL_W         23                 //
#define    _KB_CTRL_E         5                  //
#define    _KB_CTRL_Y         25                 //
#define    _KB_CTRL_G         7                  //
#define    _KB_CTRL_Q         17                 //quit
#define    _KB_CTRL_S         19                 //save buffer to file
#define    _KB_CTRL_C         3                  //quit
#define    _KB_CTRL_B         2                  //cursor control  - buffer start
#define    _KB_CTRL_N         14                 //cursor control  - buffer end
#define    _KB_CTRL_L         12                 //center the screen on the cursor

#define    _KB_CTRL_BKSLSH    28                 //cursor control  - line end
#define    _KB_CTRL_RTBRKT    29                 //cursor control  - line start

#define    _KB_F06            270                //quit

#define    _KB_UP             KEY_UP             //cursor control  - up               %
#define    _KB_DN             KEY_DOWN           //cursor control  - down             %
#define    _KB_LF             KEY_LEFT           //cursor control  - left             %
#define    _KB_RT             KEY_RIGHT          //cursor control  - right            %

#define    _KB_PD             KEY_NPAGE          //cursor control  - page down        %
#define    _KB_PU             KEY_PPAGE          //cursor control  - page up          %
#define    _KB_HM             KEY_HOME           //cursor control  - line start       %
#define    _KB_ED             KEY_END            //cursor control  - line end         %
#define    _KB_BKS            KEY_BACKSPACE      //backspace                          %
#define    _KB_ENT            KEY_ENTER          //newline etc.                       %

//                                                                           *not done
//                                                                           %platform

////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef __sighandler_t _handle;      //specific to signal.h

typedef struct                       //for cursor management
{
   int x;                            //cursor position on screen display
   int y;
   int max_x;                        //cursor area dimensions
   int max_y;
   int min_x;
   int min_y;
   int buf_x;                        //cursor position in text buffer
   int buf_y;
   int cushion;                      //horizontal cushion

   char *clip;                       //clipboard/cursor properties
   int clip_type;
   int data_type;
   int clip_lf_off;                  //clip selection coordinates
   int clip_rt_off;
   long clip_tp_off;
   long clip_bt_off;
} _cursor_inst;


////////////////////////////////////////////////////////////////////////////////////////////////////////////


char *init_new_line();
char **init_ptr_buf(int n);
char ***init_txt_buf();
void init_blank_lines(char ***txt_buf, int k, int j);
void init_null_sections(char ***txt_buf, int k, int j);

char *del_char_from_line(char *old, int offset, int curlen);
char *add_char_to_line(char *old, char add, int offset, int curlen);
char *add_char_to_line_end(char *old, char add, int offset, int curlen);

long num_lines(char ***txt_buf);
int alphanum(int ch);

int parse_input(int c, char **v);
void load_file(char ***txt_buf, char *filename);
int save_file(char ***txt_buf, char *filename, int saved, int exiting);

void fix_cursor(_cursor_inst *cursor);
void move_cursor_to_target(char ***txt_buf, _cursor_inst *cursor, int offset, long linenum);
int move_cursor(char*** txt_buf, _cursor_inst *cursor, int direction);
int move_cursor_advanced(char*** txt_buf, _cursor_inst *cursor, int key);

int show_bool_query(char *query);
void format_line_num_out(long n);
int draw_screen_text(char ***txt_buf, _cursor_inst cursor, int ch, int saved);


//*** the platform-specific functions start here...

void* handle_size(int sig);                          //misc. platform-dependent
int get_input();

void _display_init();                                //terminal display library frontend
void _display_cursor_update(_cursor_inst *cursor);
void _display_move_cursor(int row, int col);
void _display_dump_bare();
void _display_clear_eol();
void _display_string(char* str);
void _display_exit();


////////////////////////////////////////////////////////////////////////////////////////////////////////////

//globals... messy yes, but results in a more concise program and easier event handling.


int resize_scr = 1;                                     //for the resize display event


////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main (int argc, char **argv)
{
   char *open_file = _BUFDUMP;                          //file to open
   int mode = parse_input(argc, argv);                  //check input, set mode
   char ***txt_buf = init_txt_buf();                    //the text-buffering mess...
   _cursor_inst cursor = {0, 0, 0, 0, 0, 0, 0, 0, 4, 
                          NULL, -1, 0, 0, 0, 0, 0};     //our text cursor

   int ch = 0;                                          //input

   int update_scr = 1;                                  //draw the screen first time
   int update_sav = 0;                                  //file saved flag

   if (mode == _MD_OPEN)                                //if specified change the file
      open_file = argv[1];                              //to open in buffer
   else if (mode == _MD_BUF)                            //if unspecified, use default,
      mode = _MD_OPEN;                                  //so no changes

   if (mode == _MD_OPEN)                                //open a file and load it into
      load_file(txt_buf, open_file);                    //buffer

   if (mode != _MD_QUIT)                                //initialize our display
      _display_init();

   while(mode != _MD_QUIT)                              //program operation loop
   {
      switch(ch)
      {
         case _KB_ESC:             //user wants to exit
         case _KB_CTRL_C:
         case _KB_CTRL_Q:
         {
            if(save_file(txt_buf, open_file, update_sav, TRUE) == TRUE)
               mode = _MD_QUIT;
            break;
         }

         case _KB_CTRL_S:          //user wants to save
         {
            update_sav = save_file(txt_buf, open_file, FALSE, FALSE);
            update_scr = 1;
            break;
         }

         default:                  //user input text or moved cursor
         {
            //check for more complex cursor actions
            update_scr = (move_cursor_advanced(txt_buf, &cursor, ch) || update_scr);

            //if anything  changed above, clearly the saved file is our of sync
            update_sav = update_sav ? !(update_scr) : 0;

            //check input for basic cursor motion
            update_scr = (move_cursor(txt_buf, &cursor, ch) || update_scr);

            break;
         }
      } //switch

      //(re)initialize our cursor properties if necessary
      if (resize_scr)
         fix_cursor(&cursor);

      //draw our text buffer area if we changed anything
      if ((update_scr) || (resize_scr))
         update_scr = !(draw_screen_text(txt_buf, cursor, ch, update_sav));

      _display_move_cursor(cursor.y, cursor.x);         //update our cursor
      _display_dump_bare();                             //dump buffer to output

      //if all is already redrawn and we're not quitting
      if ((!resize_scr) && (mode != _MD_QUIT))
         ch = get_input();
      else if (resize_scr)
         resize_scr = 0;           //loop around once to redraw, then we're done
   } //while

   _display_exit();                //clean up


   return (0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


char *init_new_line()
{
   //initializes new blank single line, returns pointer

   char* new_line = (char*) malloc(2 * sizeof(char));

   new_line[0] = _ENDCHAR;
   new_line[1] = '\0';

   return(new_line);
}


char **init_ptr_buf(int n)
{
   //intializes the array pointer elements to null and
   //returns the pointer to new array

   //works on the n-line pointer set pointers only

   char **new_buf = (char**) malloc(n * sizeof(char*));

   int i;
   for (i = 0; i < n; i++)
      new_buf[i] = NULL;

   return(new_buf);
}


char ***init_txt_buf()
{
   //initializes text buffer, an array of sets of _MAX_LINES strings

   int i;
   char ***txt_buf = (char***) malloc(_MAX_LINES * sizeof(char**));

   for (i = 0; i < _MAX_LINES; i++)                     //initialize this thing to null
      txt_buf[i] = NULL;

   txt_buf[0] = init_ptr_buf(_MAX_LINES);               //init. the first 1000 lines
   txt_buf[0][0] = init_new_line();                     //create blank new text buffer

   return(txt_buf);
}


void init_blank_lines(char ***txt_buf, int k, int j)
{
   //intializes the blank lines up to the
   //specified point

   while (txt_buf[k][j] == NULL)                        //loop through the lines
   {
      txt_buf[k][j] = init_new_line();                  //init. any blank ones
      j--;
      if (j < 0)
      {
         j = _MAX_LINES - 1;
         k--;
      }
   } //while
}


void init_null_sections(char ***txt_buf, int k, int j)
{
   //initializes all the sections and lines that are still null
   //up to the specified location

   int q = k;                                           //counter

   while (txt_buf[q] == NULL)                           //loop through the null sets
   {                                                    //and init. them
      txt_buf[q] = init_ptr_buf(_MAX_LINES);
      q--;
   }

   if (txt_buf[k][j] == NULL)                           //init. current set
      init_blank_lines(txt_buf, k, j);
}


char *del_char_from_line(char *old, int offset, int curlen)
{
   //deletes a character from the string

   char *str = malloc((curlen) * sizeof(char));

   strncpy(str, old, offset);
   strncpy(&str[offset - 1], &old[offset], curlen - offset);
   str[curlen - 1] = '\0';

   free(old);

   return(str);
}


char *add_char_to_line(char *old, char add, int offset, int curlen)
{
   //inserts a character into the string

   char *str = malloc((curlen + 2) * sizeof(char));

   strncpy(str, old, offset);
   strncpy(&str[offset + 1], &old[offset], curlen - offset);
   str[offset] = add;
   str[curlen + 1] = '\0';

   free(old);

   return(str);
}


char *add_char_to_line_end(char *old, char add, int offset, int curlen)
{
   //inserts character beyond the end of line

   char *str = malloc((offset + 3) * sizeof(char));

   memset(&str[curlen - 1], 32, ((offset + 1) - curlen));
   strncpy(str, old, curlen - 1);
   str[offset] = add;
   str[offset + 1] = _ENDCHAR;
   str[offset + 2] = '\0';

   free(old);

   return(str);
}


long num_lines(char ***txt_buf)
{
   //returns number of lines until NULL

   long l = 0, s = 0;

   while(txt_buf[s + 1] != NULL)
      s++;

   while(txt_buf[s][l] != NULL)
      l++;

   return((_MAX_LINES * s) + l);
}


int alphanum(int ch)
{
   //returns TRUE if ch represents alphanumerical ASCII value

   return((ch >= 32) && (ch <= 126));
}


int parse_input(int c, char **v)
{
   //parses input arguments, returns appropriate mode

   int mode = 0;

   if (c == 1)                                //if no command line arguments,
   {                                          //set default _bufdump mode
      mode = _MD_BUF;
   }
   else if (c == 2)                           //file specified
   {
      FILE *fp;
      char ch[_MAX_LINES];

      if ((fp = fopen(v[1], "r")) == NULL)    //check if file specified exists
      {
         //confirm we want to create the new file
         printf("\nfile does not exist; create");
         while ((ch[0] != 'y') && (ch[0] != 'n') && (ch[0] != 'Y') && (ch[0] != 'N'))
         {
            printf(" (y/n)? ");
            scanf("%s", ch);
         }

         if ((ch[0] == 'y') || (ch[0] == 'Y'))
            mode = _MD_OPEN;
         else
            mode = _MD_QUIT;
      }
      else
      {
         fclose(fp);                          //everything worked fine, file exists
         mode = _MD_OPEN;
      }
   }
   else
   {
      printf("\ncommand line format: noir filepath\n");
      mode = _MD_QUIT;
   }

   return(mode);
}


void load_file(char ***txt_buf, char *filename)
{
   //loads an ascii text file into the buffer

   FILE *fp;
   
   if ((fp = fopen(filename, "r")) == NULL)
   {
      printf("\nerror opening file.\n");
      exit(0);
   }
   else
   {
      int cur = getc(fp);
      int counter = 0;
      long line_count = 0;

      while (cur != EOF)
      {
         int curlen = strlen(txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES]);
         if (cur != '\n')
         {
            if (!(alphanum(cur)))    //if we encounter non-displayable characters...
               cur = 'X';
            txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES] =
               add_char_to_line(txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES],
                                cur, counter, curlen);
            counter++;
         }
         else                        //encountered a newline
         {
            line_count++;
            counter = 0;

            if (txt_buf[line_count / _MAX_LINES] == NULL)
               txt_buf[line_count / _MAX_LINES] = init_ptr_buf(_MAX_LINES);

            txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES] = init_new_line();
         }

         cur = getc(fp);
      } //while

      fclose(fp);
   }
}

int save_file(char ***txt_buf, char *filename, int saved, int exiting)
{
   //saves the buffer to an ascii text file

   int success = saved;
   int save = !exiting;

   if ((exiting) && (!success))          //if we're exiting and buffer is unsaved
   {
      if (strcmp(filename, _BUFDUMP))
         save = show_bool_query("save (y/n)? ");
      else                               //if we're editing _BUFDUMP, no choice
         save = TRUE;                    //since both choices save to _bufdump

      if (!save)                         //if we're not saving...
      {
         success = TRUE;                 //save anyway to _BUFDUMP

         filename = realloc(filename, 60 * sizeof(char));
         filename = _BUFDUMP;
      }
   } //if

   if (TRUE)                             //*** DEBUG/MOD: for later modification
   {
      FILE *fp;

      if ((fp = fopen(filename, "w")) == NULL)
         printf("\nerror opening file.\n");
      else
      {
         int char_count = 0;
         long line_count = 0;
         long buf_end = num_lines(txt_buf);

         for (line_count = 0; line_count < buf_end; line_count++)
         {
            int line_len = strlen(txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES]) - 1;

            //put the line into the file
            for (char_count = 0; char_count < line_len; char_count++)
               putc(txt_buf[line_count / _MAX_LINES][line_count % _MAX_LINES][char_count], fp);

            //end the line
            if (line_count < (buf_end - 1))
               putc('\n', fp);
         } //for

         fclose(fp);
         success = TRUE;
      }
   } //if

   return(success);
}


void fix_cursor(_cursor_inst *cursor)
{
   //fixes the cursor if the screen was initialized or resized

   _display_cursor_update(cursor);   //adjusts the screen dimensions

   cursor->max_y -= 3;               //adjust for scrollbars/last scroll-line
   cursor->max_x -= 2;
   cursor->min_y = 1;                //adjust for title
   cursor->min_x = 8;                //adjust for line numbers

   cursor->x = cursor->min_x;        //move the cursor to the top left
   cursor->y = cursor->min_y;        //corner
}


void move_cursor_to_target(char ***txt_buf, _cursor_inst *cursor, int offset, long linenum)
{
   //moves cursor, taking into account scrolling etc. to the specified
   //location in the active text currently in the buffer

   while ((cursor->x - cursor->min_x + cursor->buf_x) < offset)
      move_cursor(txt_buf, cursor, _KB_RT);
   while ((cursor->x - cursor->min_x + cursor->buf_x) > offset)
      move_cursor(txt_buf, cursor, _KB_LF);
   while ((cursor->buf_y + (cursor->y - cursor->min_y)) < linenum)
      move_cursor(txt_buf, cursor, _KB_DN);
   while ((cursor->buf_y + (cursor->y - cursor->min_y)) > linenum)
      move_cursor(txt_buf, cursor, _KB_UP);
}


int move_cursor(char ***txt_buf, _cursor_inst *cursor, int direction)
{
   //moves the cursor in the specified direction, making sure to
   //take the display, buffer offset, and cushion into account

   int update = TRUE;  //assume something will happen, if not later set to FALSE

   switch(direction)
   {
      case _KB_UP:
      {
         if (cursor->y > cursor->min_y)                         //move cursor up
            cursor->y--;
         else if (cursor->buf_y > 0)                            //scroll up
            cursor->buf_y--;
         else 
            update = FALSE;

         break;
      }

      case _KB_DN:
      {
         if (cursor->y <= cursor->max_y)                        //move cursor down
            cursor->y++;
         else                                                   //scroll down
            cursor->buf_y++;

         break;
      }

      case _KB_LF:
      {
         if (cursor->x > (cursor->min_x + cursor->cushion))     //move cursor left
            cursor->x--;
         else if (cursor->buf_x > 0)                            //scroll left
            cursor->buf_x--;
         else if (cursor->x > cursor->min_x)                    //no more scroll, only move
            cursor->x--;
         else if (cursor->x == cursor->min_x)                   //beginnig of line, move to
         {                                                      //end of previous line
            if (move_cursor(txt_buf, cursor, _KB_UP))
               move_cursor(txt_buf, cursor, _KB_ED);
         }

         break;
      }

      case _KB_RT:
      {
         if (cursor->x <= (cursor->max_x - cursor->cushion))    //move cursor right
            cursor->x++;
         else                                                   //scroll right
            cursor->buf_x++;

         break;
      }

      case _KB_PU:
      {
         cursor->buf_y -= (cursor->max_y - cursor->cushion);    //scroll a screen-height
         if (cursor->buf_y < 0)                                 //down
            cursor->buf_y = 0;

         break;
      }

      case _KB_PD:
      {
         cursor->buf_y += (cursor->max_y - cursor->cushion);    //scroll screen-hgt. up

         break;
      }

      case _KB_HM:
      case _KB_CTRL_RTBRKT:
      {
         cursor->x = cursor->min_x;                             //beginning of line
         cursor->buf_x = 0;

         break;
      }

      case _KB_ED:
      case _KB_CTRL_BKSLSH:
      {
         long txt_count = cursor->buf_y + (cursor->y - cursor->min_y);
         int txt_c_1 = txt_count / _MAX_LINES;
         int txt_c_2 = txt_count % _MAX_LINES;

         //move to end of line or to first position
         int line_len = 1;
         if ((txt_buf[txt_c_1] != NULL) && (txt_buf[txt_c_1][txt_c_2] != NULL))
            line_len = strlen(txt_buf[txt_c_1][txt_c_2]);

         if (line_len < (cursor->max_x - cursor->cushion))
         {
            cursor->buf_x = 0;
            cursor->x = line_len + cursor->min_x - 1;
         }
         else
         {
            cursor->x = cursor->max_x - cursor->cushion;
            cursor->buf_x = line_len - cursor->x + cursor->min_x - 1;
         }

         break;
      }

      case _KB_CTRL_B:
      {
         cursor->buf_x = 0;                                    //beginning of buffer
         cursor->buf_y = 0;
         cursor->x = cursor->min_x;
         cursor->y = cursor->min_y;

         break;
      }

      case _KB_CTRL_N:
      {
         cursor->buf_y = num_lines(txt_buf) - cursor->cushion;       //end of buffer
         cursor->buf_y = (cursor->buf_y < 0) ? 0 : cursor->buf_y;
         cursor->y = cursor->min_y + (num_lines(txt_buf) - cursor->buf_y) - 1;

         move_cursor(txt_buf, cursor, _KB_ED);

         break;
      }

      case _KB_CTRL_L:
      {
         //center display on cursor
         int dist_center_y = (cursor->max_y / 2) - cursor->y;
         int dist_center_x = (cursor->max_x / 2) - cursor->x;

         cursor->buf_y -= dist_center_y;
         cursor->y += dist_center_y + ((cursor->buf_y < 0) * (cursor->buf_y));
         cursor->buf_y = (cursor->buf_y < 0) ? 0 : cursor->buf_y;
         cursor->y = (cursor->y < 0) ? 0 : cursor->y;

         cursor->buf_x -= dist_center_x;
         cursor->x += dist_center_x + ((cursor->buf_x < 0) * (cursor->buf_x));
         cursor->buf_x = (cursor->buf_x < 0) ? 0 : cursor->buf_x;
         cursor->x = (cursor->x < 0) ? 0 : cursor->x;

         break;
      }

      default:
      {
         update = FALSE;    //nothing happened, don't update
         break;
      }
   } //switch

   return(update);
}


int move_cursor_advanced(char*** txt_buf, _cursor_inst *cursor, int key)
{
   //does all the more complex cursor operations

   char *new_str;
   int i, j, line_len, update = 0;

   long txt_count = cursor->buf_y + (cursor->y - cursor->min_y);
   int txt_c_1 = txt_count / _MAX_LINES;
   int txt_c_2 = txt_count % _MAX_LINES;
   int offset = cursor->x - cursor->min_x + cursor->buf_x;


   switch(key)
   {
      case _KB_CTRL_X:
      {
         if (cursor->clip_type == 1)           //clip off a single line
         {
            //assign some new space for the clipped text
            free(cursor->clip);
            cursor->clip = malloc((cursor->clip_rt_off - cursor->clip_lf_off + 2) * sizeof(char));

            //get the cursor where we want it
            move_cursor_to_target(txt_buf, cursor, cursor->clip_rt_off, cursor->clip_tp_off);
            offset = cursor->x - cursor->min_x + cursor->buf_x;
            txt_count = cursor->buf_y + (cursor->y - cursor->min_y);

            //copy the new text into the clipboard string
            memcpy(&cursor->clip[0],
                   &txt_buf[txt_count / _MAX_LINES][txt_count % _MAX_LINES][cursor->clip_lf_off], 
                   (cursor->clip_rt_off - cursor->clip_lf_off + 1));
            cursor->clip[(cursor->clip_rt_off - cursor->clip_lf_off + 1)] = '\0';

            //erase all the text we just copied
            move_cursor(txt_buf, cursor, _KB_RT);
            for (i = cursor->clip_rt_off + 1; i > cursor->clip_lf_off; i--)
               move_cursor_advanced(txt_buf, cursor, _KB_BKS);
         }
         else if (cursor->clip_type == 2)      //clip multiple lines
         {
            //free and initialize the clipped text space
            free(cursor->clip);
            cursor->clip = malloc(sizeof(char));
            cursor->clip[0] = '\0';

            //if we're cutting the first line, leave some breathing space
            if (cursor->clip_tp_off == 0)
            {
               move_cursor_to_target(txt_buf, cursor, 0, cursor->clip_tp_off);
               move_cursor_advanced(txt_buf, cursor, _KB_ENT);
               cursor->clip_tp_off++;
               cursor->clip_bt_off++;
            }

            //get the cursor where we want it
            move_cursor_to_target(txt_buf, cursor, 1, cursor->clip_bt_off);
            move_cursor(txt_buf, cursor, _KB_ED);
            txt_count = cursor->buf_y + (cursor->y - cursor->min_y);

            //record and delete, sequentially, going backwards
            //making sure we aren't stuck on first line in an infinite loop
            while ((txt_count != (cursor->clip_tp_off - 1)) && !((txt_count == 0) && (offset == 0)))
            {
               int txt_count_old = cursor->buf_y + (cursor->y - cursor->min_y);
               offset = cursor->x - cursor->min_x + cursor->buf_x;

               if (offset > 0)                 //can record a character
                  cursor->clip = add_char_to_line(cursor->clip, 
                      txt_buf[txt_count_old / _MAX_LINES][txt_count_old % _MAX_LINES][offset - 1],
                      0, strlen(cursor->clip));

               move_cursor_advanced(txt_buf, cursor, _KB_BKS);

               //if we moved up a line, put an '\n' into the clip string we're building
               txt_count = cursor->buf_y + (cursor->y - cursor->min_y);
               if ((txt_count_old != txt_count) && (txt_count_old != 0))
                  cursor->clip = add_char_to_line(cursor->clip, '\n', 0, strlen(cursor->clip));
            }
         }

         cursor->data_type = ((cursor->clip_type == 1) || (cursor->clip_type == 2));
         cursor->clip_type = -1;

         update = 1;
         break;
      }

      case _KB_CTRL_D:
      {
         if (((txt_buf[txt_c_1] != NULL)) && ((txt_buf[txt_c_1][txt_c_2] != NULL)))
         {
            int curlen = strlen(txt_buf[txt_c_1][txt_c_2]);

            if (offset < (curlen - 1))                //selections only work on active text
            {
               if (cursor->clip_type == 0)            //second point selection
               {
                  //get the new point and make sure left < right and top < bottom
                  cursor->clip_rt_off = (offset < cursor->clip_lf_off) ? cursor->clip_lf_off : offset;
                  cursor->clip_lf_off = (offset < cursor->clip_lf_off) ? offset : cursor->clip_lf_off;

                  cursor->clip_bt_off = (txt_count < cursor->clip_tp_off) ? cursor->clip_tp_off : txt_count;
                  cursor->clip_tp_off = (txt_count < cursor->clip_tp_off) ? txt_count : cursor->clip_tp_off;

                  //single(type = 1) or multiline(type = 2) selection?
                  cursor->clip_type = 1 + (cursor->clip_tp_off != cursor->clip_bt_off);
               } //if
               else if (cursor->clip_type > 0)        //cancels selection
                  cursor->clip_type = -1;
               else                                   //first point selection
               {
                  cursor->clip_lf_off = offset;
                  cursor->clip_tp_off = txt_count;
                  cursor->clip_type = 0;
               }
            } //if
         } //if

         update = 1;
         break;
      }

      case _KB_CTRL_V:
      {
         int clip_len = strlen(cursor->clip);

         //roll through the clipboard data and spit the characters
         //into the buffer starting at the current cursor location
         if ((cursor->data_type == 1) && (cursor->clip != NULL))
            for (i = 0; i < clip_len; i++)
               move_cursor_advanced(txt_buf, cursor, cursor->clip[i]);

         cursor->clip_type = -1;                     //deselect

         update = 1;
         break;
      }

      case _KB_BKS:
      {
         if (((txt_buf[txt_c_1] != NULL)) && ((txt_buf[txt_c_1][txt_c_2] != NULL)))
         {
            //we'll need this to adjust all the subsequent text up a line
            long lst_count = num_lines(txt_buf);
            int curlen = strlen(txt_buf[txt_c_1][txt_c_2]);

            //move the line up and to the end of the previous line...
            if ((offset == 0) && (txt_count != 0))
            {
               //move cursor up and to the end of the previous line
               move_cursor(txt_buf, cursor, _KB_UP);
               move_cursor(txt_buf, cursor, _KB_ED);

               //build the new string from this line and the previous one
               line_len =
                  strlen(txt_buf[(txt_count - 1) / _MAX_LINES][(txt_count - 1) % _MAX_LINES]);

               new_str = malloc((line_len + curlen) * sizeof(char));
               memset(new_str, 32, line_len + curlen);
               strncpy(new_str,
                       txt_buf[(txt_count - 1) / _MAX_LINES][(txt_count - 1) % _MAX_LINES],
                       line_len - 1);
               strncpy(&new_str[line_len - 1], txt_buf[txt_c_1][txt_c_2], curlen - 1);
               new_str[line_len + curlen - 2] = _ENDCHAR;
               new_str[line_len + curlen - 1] = '\0';

               //empty out the line below
               free(txt_buf[(txt_count - 1) / _MAX_LINES][(txt_count - 1) % _MAX_LINES]);

               //assign the newly fused line to the line above
               txt_buf[(txt_count - 1) / _MAX_LINES][(txt_count - 1) % _MAX_LINES] = new_str;

               //move all the lines after this one up
               for (i = txt_count; i < lst_count; i++)
                  txt_buf[i / _MAX_LINES][i % _MAX_LINES] =
                     txt_buf[(i + 1) / _MAX_LINES][(i + 1) % _MAX_LINES];

               //clear the last line, no longer needed
               free(txt_buf[(i + 1) / _MAX_LINES][(i + 1) % _MAX_LINES]);
            }

            //delete a character...
            if ((offset < curlen) && (offset != 0))
            {
               txt_buf[txt_c_1][txt_c_2] =
                  del_char_from_line(txt_buf[txt_c_1][txt_c_2], offset, curlen);
               move_cursor(txt_buf, cursor, _KB_LF);
            }

            //or move to the end of the line if we're not in active text
            if (offset >= curlen)
               move_cursor(txt_buf, cursor, _KB_ED);

            update = 1;
         } //if
         break;
      }

      case _KB_ENT:
      case _KB_ENT_N:
      {
         //get the coordinates of the two lines we'll be splitting with an endline
         long txt_count_new = cursor->buf_y + (cursor->y - cursor->min_y) + 1;
         long lst_count = num_lines(txt_buf);
         int txt_c_1_new = txt_count_new / _MAX_LINES;
         int txt_c_2_new = txt_count_new % _MAX_LINES;

         int curlen = 0;
         if ((txt_buf[txt_count / _MAX_LINES] != NULL) &&
             (txt_buf[txt_count / _MAX_LINES][txt_count % _MAX_LINES] != NULL))
            curlen = strlen(txt_buf[txt_c_1][txt_c_2]);

         //make sure we have clean, initialized lines to work with
         init_null_sections(txt_buf, txt_c_1_new, txt_c_2_new);

         //move all the lines one down in front of the current one
         for (i = lst_count; i > txt_count_new; i--)
            txt_buf[i / _MAX_LINES][i % _MAX_LINES] =
               txt_buf[(i - 1) / _MAX_LINES][(i - 1) % _MAX_LINES];

         if (offset >= (curlen - 1))                 //nothing to move
            txt_buf[txt_c_1_new][txt_c_2_new] = init_new_line();
         else                                        //stuff to move
         {
            //put some text in the new line we emptied out
            txt_buf[txt_c_1_new][txt_c_2_new] =
               malloc((curlen - offset + 1) * sizeof(char));
            strcpy(txt_buf[txt_c_1_new][txt_c_2_new],
                   &txt_buf[txt_count / _MAX_LINES][txt_count % _MAX_LINES][offset]);

            //cut off the last part of the previous line
            txt_buf[txt_c_1][txt_c_2] = 
               realloc(txt_buf[txt_c_1][txt_c_2], (offset + 2) * sizeof(char));
            txt_buf[txt_c_1][txt_c_2][offset] = _ENDCHAR;
            txt_buf[txt_c_1][txt_c_2][offset + 1] = '\0';
         }

         move_cursor(txt_buf, cursor, _KB_HM);       //adjust the cursor
         move_cursor(txt_buf, cursor, _KB_DN);

         update = 1;
         break;
      }

      default:
      {
         //check for input characters (also a function involving cursor motion)
         if (alphanum(key) || (key == _KB_TB))
         {
            //make sure we are inserting characters into initialized lines
            init_null_sections(txt_buf, txt_c_1, txt_c_2);

            j = (key == _KB_TB) ? _TAB_LEN : 1;      //adjust in case we're tabbing
            key = (key == _KB_TB) ? ' ' : key;

            for (i = 0; i < j; i++)                  //multiple times if we're tabbing
            {
               int offset = cursor->x - cursor->min_x + cursor->buf_x;
               int curlen = strlen(txt_buf[txt_c_1][txt_c_2]);

               if (offset < curlen)
                  txt_buf[txt_c_1][txt_c_2] =
                     add_char_to_line(txt_buf[txt_c_1][txt_c_2], key, offset, curlen);
               else
                  txt_buf[txt_c_1][txt_c_2] =
                     add_char_to_line_end(txt_buf[txt_c_1][txt_c_2], key, offset, curlen);

               move_cursor(txt_buf, cursor, _KB_RT); //move the cursor
            } //for
         } //if

         update = 1;   //since something was clearly pressed, need to display the code
         break;
      }
   } //switch

   return(update);
}


int show_bool_query(char *query)
{
   //asks a y/n question of the user, returns response

   int ch;
   _display_move_cursor(0, 42);
   _display_clear_eol();
   _display_string(query);
   _display_dump_bare();

   while ((ch != 'y') && (ch != 'n') && (ch != 'Y') && (ch != 'N'))
      ch = get_input();

   _display_move_cursor(0, 42);
   _display_clear_eol();                        //clear that area
   _display_dump_bare();

   return((ch == 'Y') || (ch == 'y'));
}


void format_line_num_out(long n)
{
   //outputs a line number with necessary number of spaces
   char *disp_str = malloc(8 * sizeof(char));

   if (n > ((_MAX_LINES * _MAX_LINES) - 1))     //probably won't be editing files
      n = n % (_MAX_LINES * _MAX_LINES);        //over 1000000 lines anyway.

   sprintf(disp_str, "%6ld:", n);
   _display_string(disp_str);
}


int draw_screen_text(char ***txt_buf, _cursor_inst cursor, int ch, int saved)
{
   //draws the active text area of the screen
   char *disp_str = malloc((cursor.max_x * sizeof(char)) + 2);
   int i;

   //output terminal title and display size
   _display_move_cursor(0, 0);
   _display_clear_eol();
   _display_string("noir terminal editor.");
   _display_move_cursor(0, 22);

   sprintf(disp_str, "display: (%d, %d)", cursor.max_x + 2, cursor.max_y + 3);
   _display_string(disp_str);

   //output if the buffer is synced with the output file
   if (saved)
   {
      _display_move_cursor(0, 42);
      _display_string("saved.");
   }

   //display clipboard function indicator
   _display_move_cursor(0, 49);
   if (cursor.clip_type == 0)
      _display_string("sel-");
   else if (cursor.clip_type == 1)
      _display_string("sel-single");
   else if (cursor.clip_type == 2)
      _display_string("sel-multi.");

   //display all the active text display lines
   for (i = cursor.min_y; i <= cursor.max_y + 1; i++)
   {
      long txt_count = cursor.buf_y + i - 1;
      int txt_c_1 = txt_count / _MAX_LINES;
      int txt_c_2 = txt_count % _MAX_LINES;

      _display_move_cursor(i, 0);
      _display_clear_eol();
      format_line_num_out((long)(cursor.buf_y + i));
      _display_move_cursor(i, cursor.min_x);

      if ((txt_buf[txt_c_1] != NULL) && (txt_buf[txt_c_1][txt_c_2] != NULL))
         if (strlen(txt_buf[txt_c_1][txt_c_2]) > cursor.buf_x)
            _display_string(&txt_buf[txt_c_1][txt_c_2][cursor.buf_x]);
   } //for

   //clean that last terminal blank command line space
   _display_move_cursor(cursor.max_y + 2, 0);
   _display_clear_eol();

   //output the value our last character input
   _display_move_cursor(cursor.max_y + 2, 1);

   sprintf(disp_str, "%d", ch);
   _display_string(disp_str);

   return(TRUE);     //done successfully
}


/***********************************************************************************************************

  back-end display functionality...the only functions you'll need to modify for cross-platform adaptation
  
***********************************************************************************************************/


void* handle_size(int sig)
{
   //unix/linux handler for terminal resizing

   _display_exit(); //reset our display
   _display_init();
   resize_scr = 1;

   return ((void*) 0);
}


int get_input()
{
   //gets a character of input

   while (TRUE)
   {
      int ch = getch();          //wait for the next keypress.
      if (ch != ERR)
         return(ch);
   }
}


void _display_init()
{
   //display library initialization calls

   signal(SIGWINCH, (_handle) handle_size);      //event handling, linux/unix-specific

   initscr();                                    //ncurses initialization calls
   //cbreak();
   raw();
   noecho();
   keypad(stdscr, TRUE);
   refresh();
   nodelay(stdscr, TRUE);
}


void _display_cursor_update(_cursor_inst *cursor)
{
   //updates cursor properties using library routine

   getmaxyx(stdscr, cursor->max_y, cursor->max_x);
}


void _display_move_cursor(int row, int col)
{
   //moves cursor to specified coordinates

   move(row, col);
}


void _display_dump_bare()
{
   //flushes changes/refreshes display without cursor update

   refresh();
}

void _display_clear_eol()
{
   //clears from cursor position to the end of the line

   clrtoeol();
}


void _display_string(char* str)
{
   //outputs supplied string at the current cursor location

   printw("%s", str);
}


void _display_exit()
{
   //display library exit calls

   noraw();
   endwin();
}

/* eof */