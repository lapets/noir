#!/usr/bin/perl

#***********************************************************************************************************
#  noir.pl
#  noir terminal editor
#  ver 0.7.1(a)(w)(p)
#
#  everything you need is contained in this file
#  this version depends on TermReadKey, available through CPAN:
#  % ppm; install TermReadKey; exit;
#
#  runs on most machines using some variant of:
#  % perl noir.pl
#
#  if using linux, just drop the file into your /bin/ folder and rename it to "noir".
#  to use "noir" on the command line, don't forget to change permissions, eg.
#  % chmod u+x noir
#
#  if you're using windows there should be no problems on the ActiveState distribution
#  which is available for free at <http://www.activestate.com>
#  you can probably just drop the file into the directory
#  %SystemRoot%\system32, C:\Windows\System32, or some equivalent
#
#  ----- <PORTING> ---------------------------------------------------------------------------
#
#  if you're adapting the source to another library or platform, not much work is
#  involved, here's a list of what you have to do:
#
#  - adjust the values for keyboard constant to match your platform/library;
#       this is the most time-consuming part on most occasions
#  - adjust the get_input() function if your keyboard input library is different
#  - adjust the following display library front-end functions to call
#       the equivalent functions from your library:
#                            
#    sub _display_init                      # display library initialization calls
#    sub _display_cursor_update(\%cursor)   # updates cursor properties
#    sub _display_move_cursor($row, $col)   # move cursor to specified coordinates
#    sub _display_dump_bare                 # flushes changes/refreshes display
#    sub _display_exit                      # display library exit calls
#
#    even if a display library does not have near equivalents for these functions,
#    their intended function can probably be coded using the new library's functions
#
#
#  ----- <THINGS LEFT TO DO> -----------------------------------------------------------------
#
#  - write function for catching quit event and dump the buffer to a file
#  - smart bracing/tabbing
#  - backspace multiple times to erase line for backspace+shift/ctrl functionality
#  - other misc. functionality (check reserved key #def's)
#
#  ----- <README> ----------------------------------------------------------------------------
#
#     This terminal editor was conceived as an extremely compact, relatively portable,
#     functional text editor for command line/terminal environments. Right now some
#     important functionality is not yet complete, and I may not get around to it myself.
#
#     Currently included functionality...
#
#      - Check the keyboard constants at the beginning of the source for key functions
#        which are already implemented.
#      - The current clipboard function is as follows:
#
#           Press the select key on one end of the text selection you want to make,
#           "sel-" will be displayed on the top line. Now select another character
#           either on the same line ("sel-single" will be displayed) or on a different
#           line ("sel-multi." will be displayed). If you selected text on a single,
#           line, the cut key will copy the stretch of text to the keyboard and delete
#           it from the line. It can now be pasted anywhere in the buffer. If you
#           selected points on different lines, all lines including the ones you
#           selected and all lines in between will be copied to the clipboard and
#           removed if the cut key is pressed. Pasting will insert these whole lines
#           above the line upon which the cursor is located.
#
#      - To edit an existing file or create a new file type "noir filename" on the
#        command line; noir will confirm the creation of a new file. If you do not save
#        the buffer in the new file before you quit, and you confirm that you do not
#        want to save, the file will not be created.
#      - If you type "noir" without a command line argument, noir will assume you
#        are editing the default file "_bufdump". In this situation, your changes will
#        be saved automatically upon exit.
#      - If you quit and the changes in the buffer are unsaved, noir will query if you
#        want to save. If you type 'N' or 'n', the current buffer will still be saved
#        to the file "_bufdump" in your current working directory. If you type 'Y' or 'y'
#        the buffer will be saved to the last file you listed on the command line.
#
#
#     What's going on...
#
#                                                    _ (2) indicates buffer is saved
#         (1) dimensions of display_                /          _ (3) displays clip status
#                                   \              /          /
#      +--------------------------------------------------------------------------------+
#      |noir terminal editor. display: (78, 22)   saved.  sel-multi.                    |
#      |     1: ~                                                                       |
#      |     2: Active text area; move your cursor to where you want to see text~       |
#      |     3: and begin typing.~                                                      |
#      |     4: ~                                                                       |
#      |     5: The tilde represents the end of a line (defined by _ENDCHAR) -> ~       |
#      |     6: The tilde is not saved to your text file, it merely replaces~           |
#      |     7:    the standard '\n' symbol on the active display.~                     |
#      |     8: ~                                                                       |
#      |     9:                                                                         |
#      |    10:                                                                         |
#      |    11:                                                                         |
#      |    12:                                                                         |
#      |    13:                                                                         |
#      |    14:                                                                         |
#      |    15:                                                                         |
#      |    16:                                                                         |
#      |    17:                                                                         |
#      |    18:                                                                         |
#      |    19:                                                                         |
#      |    20:                                                                         |
#      |    21:                                                                         |
#      |    22:                                                                         |
#      | 103                                                                            |
#      |                                                                                |
#      +--------------------------------------------------------------------------------+
#         \   \_ (4) line numbers listed on left
#          \
#           \_ (5) the ascii code or keyboard scancode of the last keypress
#
#     (1) The dimensions of the current display are displayed in (width, height) format.
#     (2) If the current displayed buffer matches the file on disk, "saved." appears.
#     (3) The current status of the clipboard functions displayed...
#            sel-             ... the first coordinate has been selected, awaiting second
#            sel-single       ... you have a single-line stretch of text selected
#            sel-multi.       ... you have multiple lines selected
#     (4) The line numbers are displayed on left. Resets to 0 when 1,000,000 is reached.
#     (5) The ascii code of the last keypress is displayed. Useful for adjusting keycode
#            constants, among other uses.
#
#  ----- </README> ---------------------------------------------------------------------------
#
#  2004/08/10 - 2004/12/28
#
#  andrei lapets
#  a@lapets.io
#
#***********************************************************************************************************

# use strict;
use Term::ReadKey;          ## our only dependency, go get it on CPAN if you need it
                            ## eg. % ppm; install TermReadKey; exit;


$GCON_ERR                   = -1;

$GCON_TRUE                  = 1;
$GCON_FALSE                 = 0;

$GCON_MAX_LINES             = 1000;

$GCON_MD_OPEN               = 111;
$GCON_MD_NEW                = 112;
$GCON_MD_QUIT               = 113;
$GCON_MD_BUF                = 114;

$GCON_BUFDUMP               = '_bufdump';       ## default save buffer/open buffer file
$GCON_CURCHAR               = '*';              ## character to display as cursor
$GCON_ENDCHAR               = '~';              ## character to display as endline
$GCON_TAB_LEN               = 3;                ## number of spaces equaling one tab

## Keyboard

$GCON_KB_SHFT_BKS           = -1;               ## kill everything on line to left    *
$GCON_KB_CTRL_UDRSCR        = -1;               ## kill everythingon line to right    *
$GCON_KB_CTRL_T             = -1;               ## time                               *
$GCON_KB_CTRL_H             = -1;               ## goto                               *
$GCON_KB_CTRL_F             = -1;               ## find                               *
$GCON_KB_CTRL_R             = -1;               ## replace                            *
$GCON_KB_CTRL_Z             = -1;               ## undo                               *
$GCON_KB_CTRL_A             = -1;               ## redo                               *

$GCON_KB_ENT_N              = ord("\n");        ## catch both just in case
$GCON_KB_TB                 = 9;                ## tab key
$GCON_KB_ESC                = 27;               ## quit

$GCON_KB_CTRL_D             = 4;                ## select
$GCON_KB_CTRL_X             = 24;               ## cut
$GCON_KB_CTRL_V             = 22;               ## paste

$GCON_KB_CTRL_L             = 12;               ##
$GCON_KB_CTRL_E             = 5;                ##
$GCON_KB_CTRL_G             = 7;                ##
$GCON_KB_CTRL_Q             = 17;               ## quit
$GCON_KB_CTRL_E             = 5;                ## save buffer to file
$GCON_KB_CTRL_C             = 3;                ## quit
$GCON_KB_CTRL_B             = 2;                ## cursor control  - buffer start
$GCON_KB_CTRL_N             = 14;               ## cursor control  - buffer end
$GCON_KB_CTRL_W             = 23;               ## center the screen on the cursor

$GCON_KB_CTRL_BKSLSH        = 28;               ## cursor control  - line end
$GCON_KB_CTRL_RTBRKT        = 29;               ## cursor control  - line start

$GCON_KB_F06                = 270;              ## quit

$GCON_KB_BKS                = 8;                ## backspace
$GCON_KB_ENT                = 13;               ## newline etc.

$GCON_KB_UP                 = 15;               ## cursor control  - up               %
$GCON_KB_DN                 = 12;               ## cursor control  - down             %
$GCON_KB_LF                 = 11;               ## cursor control  - left             %
$GCON_KB_RT                 = 16;               ## cursor control  - right            %
$GCON_KB_UP_B               = 15;               ## cursor control  - alternate up
$GCON_KB_DN_B               = 12;               ## cursor control  - alternate down
$GCON_KB_LF_B               = 11;               ## cursor control  - alternate left
$GCON_KB_RT_B               = 16;               ## cursor control  - alternate right

$GCON_KB_PD                 = 21;               ## cursor control  - page down        %
$GCON_KB_PU                 = 25;               ## cursor control  - page up          %
$GCON_KB_PD_B               = 21;               ## cursor control  - alt. page down
$GCON_KB_PU_B               = 25;               ## cursor control  - alt. page up

$GCON_KB_HM                 = 29;               ## cursor control  - line start
$GCON_KB_ED                 = 28;               ## cursor control  - line end

##                                                                           *not done
##                                                                           %platform

############################################################################################################


%_cursor_inst = (
   'x' => 0,                        ## cursor position on screen display
   'y' => 0,
   'max_x' => 0,                    ## cursor area dimensions
   'max_y' => 0,
   'min_x' => 0,
   'min_y' => 0,
   'buf_x' => 0,                    ## cursor position in text buffer
   'buf_y' => 0,
   'cushion' => 4,                  ## horizontal cushion

   'clip' => '',                    ## clipboard/cursor properties
   'clip_type' => -1,
   'data_type' => 0,

   'clip_lf_off' => 0,              ## clip selection coordinates
   'clip_rt_off' => 0,
   'clip_tp_off' => 0,
   'clip_bt_off' => 0
);


############################################################################################################


# sub init_null_sections(\@txt_buf, $txt_count);

# sub del_char_from_line($old, $offset);
# sub add_char_to_line($old, $add, $offset);
# sub add_char_to_line_end($old, $add, $offset, $curlen);

# sub alphanum($num);

# sub parse_input(@list_of_args);
# sub load_file(\@txt_buf, $filename);
# sub save_file(\@txt_buf, $filename, $saved, $exiting);

# sub fix_cursor(\%cursor);
# sub move_cursor_to_target(\@txt_buf, \%cursor, $offset, $linenum);
# sub move_cursor(\@txt_buf, \%cursor, $ch);
# sub move_cursor_advanced(\@txt_buf, \%cursor, $key);

# sub show_bool_query($query);
# sub format_line_num_out($n);
# sub draw_screen_text(\@txt_buf, \%cursor, $ch, $saved);

# sub handle_size;                          
# sub get_input;

# sub _display_init;
# sub _display_cursor_update(\%cursor);
# sub _display_move_cursor($row, $col);
# sub _display_dump_bare;
# sub _display_exit;


############################################################################################################


$open_file = $GCON_BUFDUMP;                                 ## file to open
$mode = &parse_input(@ARGV);                                ## check input, set mode
@txt_buf = $GCON_ENDCHAR;                                   ## the text-buffering mess...
%cursor = %_cursor_inst;                                    ## our text cursor

$ch = 0;                                                    ## input

$resize_scr = 1;                                            ## for the resize display event
$update_scr = 1;                                            ## draw the screen first time
$update_sav = 0;                                            ## file saved flag


$open_file = $ARGV[0] if ($mode == $GCON_MD_OPEN);          ## if specified change file to open in buffer
$mode = $GCON_MD_OPEN if ($mode == $GCON_MD_BUF);           ## if unspecified, use default, so no changes
&load_file(\@txt_buf, $open_file) if ($mode == $GCON_MD_OPEN);     ## open a file and load it into buffer


&_display_init if ($mode != $GCON_MD_QUIT);                 ## initialize our display


while($mode != $GCON_MD_QUIT)                                   ## program operation loop
{
   ## must always update in this implementation, otherwise delete
   $update_scr = 1;

   if (($ch eq $GCON_KB_CTRL_Q) || ($ch eq $GCON_KB_CTRL_C) || ($ch eq $GCON_KB_ESC))
   {
      $update_scr = 0;
         $mode = $GCON_MD_QUIT                                  ## user wants to exit
      if(&save_file(\@txt_buf, $open_file, $update_sav, $GCON_TRUE) == $GCON_TRUE);
   }
   elsif ($ch eq $GCON_KB_CTRL_E)                               ## user wants to save
   {
      $update_sav = &save_file(\@txt_buf, $open_file, $GCON_FALSE, $GCON_FALSE);
      $update_scr = 1;
   }
   else                                                         ## user input text or moved cursor
   {
      ## check for more complex cursor actions
      $update_scr = (&move_cursor_advanced(\@txt_buf, \%cursor, $ch) || $update_scr);

      ## if anything  changed above, clearly the saved file is our of sync
      $update_sav = $update_sav ? !($update_scr) : 0;

      ## check input for basic cursor motion
      $update_scr = (&move_cursor(\@txt_buf, \%cursor, $ch) || $update_scr);
   } ## end if

   ## (re)initialize our cursor properties if necessary
   &fix_cursor(\%cursor) if ($resize_scr == 1);

   ## draw our text buffer area if we changed anything
      $update_scr = !(&draw_screen_text(\@txt_buf, \%cursor, $ch, $update_sav))
   if (($update_scr) || ($resize_scr));

   &_display_move_cursor($cursor{'y'}, $cursor{'x'});           ## update our cursor
   &_display_dump_bare;                                         ## dump buffer to output

   ## if all is already redrawn and we're not quitting
   if (($resize_scr == 0) && ($mode != $GCON_MD_QUIT))
   {
      $ch = &get_input;
   }
   elsif ($resize_scr)
   {
      $resize_scr = 0;         ## loop around once to redraw, then we're done
   }
} ## while


_display_exit;                 ## clean up

############################################################################################################


sub init_null_sections
{
   ## initializes all the sections and lines that are still null
   ## up to the specified location

   my ($txt_buf, $txt_count) = @_;
   @$txt_buf = (@$txt_buf, (("$GCON_ENDCHAR") x ($txt_count - @$txt_buf)));
}


sub del_char_from_line
{
   ## deletes a character from the string

   my ($old, $offset) = @_;
   return(substr($old, 0, $offset) . substr($old, $offset + 1));
}


sub add_char_to_line
{
   ## inserts a character into the string

   my ($old, $add, $offset) = @_;
   return(substr($old, 0, $offset) . $add . substr($old, $offset));
}


sub add_char_to_line_end
{
   ## inserts character beyond the end of line

   my ($old, $add, $offset, $curlen) = @_;
   return(substr($old, 0, $curlen - 1) . ' 'x($offset - $curlen) . $add . $GCON_ENDCHAR);
}


sub alphanum
{
   ## returns TRUE if $num represents alphanumerical ASCII value

   my ($num) = @_;
   return(($num >= 32) && ($num <= 126));
}


sub parse_input
{
   ## parses input arguments, returns appropriate mode

   my @list_of_args = @_;
   my $num_args = $#list_of_args + 1;
   my $mode = '';

   if ($num_args == 0)                       ## if no command line arguments,
   {                                         ## set default _bufdump mode
      $mode = $GCON_MD_BUF;
   }
   elsif ($num_args == 1)                    ## file specified
   {
      $file_path = $list_of_args[0];

      if (!(-e $file_path))                  ## check if file specified exists
      {
         ## confirm we want to create the new file
         printf("\n$file_path: file does not exist, ");

         do
         {
            printf("create (y/n then enter)? ");
            $k = <STDIN>;
            chomp($k);
            $k = ord($k);
         }
         while (($k != 121) && ($k != 89) && ($k != 110) && ($k != 78));

         $mode = (($k == 121) || ($k == 89)) ? $GCON_MD_OPEN : $GCON_MD_QUIT;
      }
      else
      {
         $mode = $GCON_MD_OPEN;
      }
   }
   else
   {
      printf("\ncommand line format: noir filepath\n");
      $mode = $GCON_MD_QUIT;
   }

   return($mode);
}


sub load_file
{
   ## loads an ascii text file into the buffer

   my ($txt_buf, $filename) = @_;

   if (-e $filename)
   {
      open(TARGET, "$filename") || die "\nerror opening file.\n";
      @$txt_buf = <TARGET>;
      close(TARGET);

      foreach $i (0..(@$txt_buf - 1))
      {
         chomp($$txt_buf[$i]);
         $cur_line = '';
         
         for($j = 0; $j < length($$txt_buf[$i]); $j++)
         {
            $cur = substr($$txt_buf[$i], $j, 1);
            $cur = 'X' if(!(&alphanum(ord($cur))));
            $cur_line = $cur_line . $cur;
         }
         
         $$txt_buf[$i] = $cur_line . $GCON_ENDCHAR;
      }
   }
}


sub save_file
{
   ## saves the buffer to an ascii text file

   my ($txt_buf, $filename, $saved, $exiting) = @_;
   my $success = $saved;
   my $save = !($exiting);

   if (($exiting) && (!($success)))     ## if we're exiting and buffer is unsaved
   {
      if (!($filename eq $GCON_BUFDUMP))
      {
         $save = show_bool_query("save (y/n)? ");
      }
      else                              ## if we're editing _BUFDUMP, no choice
      {
         $save = $GCON_TRUE;            ## since both choices save to _bufdump
      }

      if (!($save))                     ## if we're not saving...
      {
         $success = $GCON_TRUE;         ## save anyway to _BUFDUMP
         $filename = $GCON_BUFDUMP;
      }
   } ## end if

   open (DATA, ">" . "$filename");
   for ($i = 0; $i < @$txt_buf; $i++)
   {
      print DATA substr($$txt_buf[$i], 0, length($$txt_buf[$i]) - 1) . "\n";
   }
   close(DATA);
   
   $success = $GCON_TRUE;
   return($success);
}


sub fix_cursor
{
   ## fixes the cursor if the screen was initialized or resized

   my ($cursor_ref) = @_;

   &_display_cursor_update($cursor_ref);       ## adjusts the screen dimensions

   $$cursor_ref{max_y} -= 3;                   ## adjust for scrollbars/last scroll-line
   $$cursor_ref{max_x} -= 2;
   $$cursor_ref{min_y} = 1;                    ## adjust for title
   $$cursor_ref{min_x} = 7;                    ## adjust for line numbers

   $$cursor_ref{y} = $$cursor_ref{min_y};      ## move the cursor to the top left
   $$cursor_ref{x} = $$cursor_ref{min_x};      ## corner
}


sub move_cursor_to_target
{
   ## moves cursor, taking into account scrolling etc. to the specified
   ## location in the active text currently in the buffer

   my ($txt_buf, $cursor, $offset, $linenum) = @_;

   while (($$cursor{x} - $$cursor{min_x} + $$cursor{buf_x}) < $offset)
   {
      &move_cursor($txt_buf, $cursor, $GCON_KB_RT);
   }
   while (($$cursor{x} - $$cursor{min_x} + $$cursor{buf_x}) > $offset)
   {
      &move_cursor($txt_buf, $cursor, $GCON_KB_LF);
   }
   while (($$cursor{buf_y} + ($$cursor{y} - $$cursor{min_y})) < $linenum)
   {
      &move_cursor($txt_buf, $cursor, $GCON_KB_DN);
   }
   while (($$cursor{buf_y} + ($$cursor{y} - $$cursor{min_y})) > $linenum)
   {
      &move_cursor($txt_buf, $cursor, $GCON_KB_UP);
   }
}


sub move_cursor
{
   ## moves the cursor in the specified direction, making sure to
   ## take the display, buffer offset, and cushion into account

   my ($txt_buf, $cursor, $ch) = @_;
   my $update = $GCON_TRUE;                 ## assume something will happen, if not, later set to FALSE

   if (($ch == $GCON_KB_UP) || ($ch == $GCON_KB_UP_B))
   {
      if ($$cursor{'y'} > $$cursor{'min_y'}) {                 ## move cursor up
         $$cursor{'y'}--;
      } elsif ($$cursor{'buf_y'} > 0) {                        ## scroll up
         $$cursor{'buf_y'}--;
      } else { 
         $update = $GCON_FALSE;
      }
   }
   elsif (($ch == $GCON_KB_DN) || ($ch == $GCON_KB_DN_B))
   {
      if ($$cursor{'y'} <= $$cursor{'max_y'}) {                ## move cursor down
         $$cursor{'y'}++;
      } else {                                                 ## scroll down
         $$cursor{'buf_y'}++;
      }
   }
   elsif (($ch == $GCON_KB_LF) || ($ch == $GCON_KB_LF_B))
   {
      if ($$cursor{'x'} > ($$cursor{'min_x'} + $$cursor{'cushion'})) {   ## move cursor left
         $$cursor{'x'}--;
      } elsif ($$cursor{'buf_x'} > 0) {                        ## scroll left
         $$cursor{'buf_x'}--;
      } elsif ($$cursor{'x'} > $$cursor{'min_x'}) {            ## no more scroll, only move
         $$cursor{'x'}--;
      } elsif ($$cursor{'x'} == $$cursor{'min_x'}) {           ## start of line, move to end of prev line
            &move_cursor($txt_buf, $cursor, $GCON_KB_ED)
         if (&move_cursor($txt_buf, $cursor, $GCON_KB_UP));
      }
   }
   elsif (($ch == $GCON_KB_RT) || ($ch == $GCON_KB_RT_B))
   {
      if ($$cursor{'x'} <= ($$cursor{'max_x'} - $$cursor{'cushion'})) {  ## move cursor right
         $$cursor{'x'}++;
      } else {                                                           ## scroll right
         $$cursor{'buf_x'}++;
      }
   }
   elsif (($ch == $GCON_KB_PU) || ($ch == $GCON_KB_PU_B))
   {
      $$cursor{'buf_y'} -= ($$cursor{'max_y'} - $$cursor{'cushion'});    ## scroll a screen-height
      $$cursor{'buf_y'} = 0 if ($$cursor{'buf_y'} < 0);                  ## down
   }
   elsif (($ch == $GCON_KB_PD) || ($ch == $GCON_KB_PD_B))
   {
      $$cursor{'buf_y'} += ($$cursor{'max_y'} - $$cursor{'cushion'});    ## scroll screen-hgt. up
   }
   elsif (($ch == $GCON_KB_HM) || ($ch == $GCON_KB_CTRL_RTBRKT))
   {
      $$cursor{'x'} = $$cursor{'min_x'};                       ## beginning of line
      $$cursor{'buf_x'} = 0;
   }
   elsif (($ch == $GCON_KB_ED) || ($ch == $GCON_KB_CTRL_BKSLSH))
   {
      $txt_count = $$cursor{'buf_y'} + ($$cursor{'y'} - $$cursor{'min_y'});

      ## move to end of line or to first position
      $line_len = 1;
         $line_len = length($$txt_buf[$txt_count])
      if (defined($$txt_buf[$txt_count]));
            
      if ($line_len < ($$cursor{'max_x'} - $$cursor{'cushion'}))
      {
         $$cursor{'buf_x'} = 0;
         $$cursor{'x'} = $line_len + $$cursor{'min_x'} - 1;
      }
      else
      {
         $$cursor{'x'} = $$cursor{'max_x'} - $$cursor{'cushion'};
         $$cursor{'buf_x'} = $line_len - $$cursor{'x'} + $$cursor{'min_x'} - 1;
      }
   }
   elsif ($ch == $GCON_KB_CTRL_B)
   {
      $$cursor{'buf_x'} = 0;                                   ## beginning of buffer
      $$cursor{'buf_y'} = 0;
      $$cursor{'x'} = $$cursor{'min_x'};
      $$cursor{'y'} = $$cursor{'min_y'};
   }
   elsif ($ch == $GCON_KB_CTRL_N)
   {
      $$cursor{'buf_y'} = @$txt_buf - $$cursor{'cushion'};      ## end of buffer
      $$cursor{'buf_y'} = ($$cursor{'buf_y'} < 0) ? 0 : $$cursor{'buf_y'};
      $$cursor{'y'} = $$cursor{'min_y'} + (@$txt_buf - $$cursor{'buf_y'}) - 1;

      &move_cursor($txt_buf, $cursor, $GCON_KB_ED);
   }
   elsif ($ch == $GCON_KB_CTRL_W)
   {
      ## center display on cursor
      $dist_center_y = ($$cursor{'max_y'} / 2) - $$cursor{'y'};
      $dist_center_x = ($$cursor{'max_x'} / 2) - $$cursor{'x'};

      $$cursor{'buf_y'} -= $dist_center_y;
      $$cursor{'y'} += $dist_center_y + (($$cursor{'buf_y'} < 0) * ($$cursor{'buf_y'}));
      $$cursor{'buf_y'} = ($$cursor{'buf_y'} < 0) ? 0 : $$cursor{'buf_y'};
      $$cursor{'y'} = ($$cursor{'y'} < 0) ? 0 : $$cursor{'y'};

      $$cursor{'buf_x'} -= $dist_center_x;
      $$cursor{'x'} += $dist_center_x + (($$cursor{'buf_x'} < 0) * ($$cursor{'buf_x'}));
      $$cursor{'buf_x'} = ($$cursor{'buf_x'} < 0) ? 0 : $$cursor{'buf_x'};
      $$cursor{'x'} = ($$cursor{'x'} < 0) ? 0 : $$cursor{'x'};
   }
   else
   {
      $update = $GCON_FALSE;                                   ## nothing happened, don't update
   } ## end if

   return($update);
}


sub move_cursor_advanced
{
   ## does all the more complex cursor operations

   my ($txt_buf, $cursor, $key) = @_;
   my $update = 0;
   my $txt_count = $$cursor{'buf_y'} + ($$cursor{'y'} - $$cursor{'min_y'});
   my $offset = $$cursor{'x'} - $$cursor{'min_x'} + $$cursor{'buf_x'};


   if ($key == $GCON_KB_CTRL_X)
   {
      if ($$cursor{'clip_type'} == 1)          ## clip off a single line
      {
         ## get the cursor where we want it
         &move_cursor_to_target($txt_buf, $cursor, $$cursor{'clip_rt_off'} + 1, $$cursor{'clip_tp_off'});

         ## copy the new text into the clipboard string
         $$cursor{'clip'} = substr($$txt_buf[$txt_count], $$cursor{'clip_lf_off'}, 
                            ($$cursor{'clip_rt_off'} - $$cursor{'clip_lf_off'} + 1));

         ## erase all the text we just copied
         &move_cursor(txt_buf, cursor, _KB_RT);
         for ($i = $$cursor{'clip_rt_off'} + 1; $i > $$cursor{'clip_lf_off'}; $i--)
         {
               &move_cursor_advanced($txt_buf, $cursor, $GCON_KB_BKS);
         }
      }
      elsif ($$cursor{'clip_type'} == 2)       ## clip multiple lines
      {
         ## initialize the clipped text space
         $$cursor{'clip'} = '';

         ## if we're cutting the first line, leave some breathing space
         if ($$cursor{'clip_tp_off'} == 0)
         {
            &move_cursor_to_target($txt_buf, $cursor, 0, $$cursor{'clip_tp_off'});
            &move_cursor_advanced($txt_buf, $cursor, $GCON_KB_ENT);
            $$cursor{'clip_tp_off'}++;
            $$cursor{'clip_bt_off'}++;
         }

         ## get the cursor where we want it
         &move_cursor_to_target($txt_buf, $cursor, 1, $$cursor{'clip_bt_off'});
         &move_cursor($txt_buf, $cursor, $GCON_KB_ED);
         $txt_count = $$cursor{'buf_y'} + ($$cursor{'y'} - $$cursor{'min_y'});

         ## record and delete, sequentially, going backwards
         ## making sure we aren't stuck on first line in an infinite loop
         while (($txt_count != ($$cursor{'clip_tp_off'} - 1)) && !(($txt_count == 0) && ($offset == 0)))
         {
            $txt_count_old = $$cursor{'buf_y'} + ($$cursor{'y'} - $$cursor{'min_y'});
            $offset = $$cursor{'x'} - $$cursor{'min_x'} + $$cursor{'buf_x'};

               $$cursor{'clip'} = substr($$txt_buf[$txt_count_old], $offset - 1, 1) . $$cursor{'clip'}
            if ($offset > 0);                 ## can record a character

            &move_cursor_advanced($txt_buf, $cursor, $GCON_KB_BKS);

            ## if we moved up a line, put an '\n' into the clip string we're building
            $txt_count = $$cursor{'buf_y'} + ($$cursor{'y'} - $$cursor{'min_y'});

               $$cursor{'clip'} = "\n" . $$cursor{'clip'}
            if (($txt_count_old != $txt_count) && ($txt_count_old != 0));
         }
      }

      $$cursor{'data_type'} = (($$cursor{'clip_type'} == 1) || ($$cursor{'clip_type'} == 2));
      $$cursor{'clip_type'} = -1;

      $update = 1;
   }
   elsif ($key == $GCON_KB_CTRL_D)
   {
      ## selections only work on active text
      if ((length($$txt_buf[$txt_count]) > 0) && ($offset < (length($$txt_buf[$txt_count]) - 1)))
      {
         if ($$cursor{'clip_type'} == 0)        ## second point selection
         {
            ## get the new point and make sure left < right and top < bottom
            $$cursor{'clip_rt_off'} = ($offset < $$cursor{'clip_lf_off'}) ? $$cursor{'clip_lf_off'} : $offset;
            $$cursor{'clip_lf_off'} = ($offset < $$cursor{'clip_lf_off'}) ? $offset : $$cursor{'clip_lf_off'};

            $$cursor{'clip_bt_off'} = ($txt_count < $$cursor{'clip_tp_off'}) ? $$cursor{'clip_tp_off'} : $txt_count;
            $$cursor{'clip_tp_off'} = ($txt_count < $$cursor{'clip_tp_off'}) ? $txt_count : $$cursor{'clip_tp_off'};

            ## single(type = 1) or multiline(type = 2) selection?
            $$cursor{'clip_type'} = 1 + ($$cursor{'clip_tp_off'} != $$cursor{'clip_bt_off'});
         }
         elsif ($$cursor{'clip_type'} > 0)          ## cancels selection
         {
            $$cursor{'clip_type'} = -1;
         }
         else                                   ## first point selection
         {
            $$cursor{'clip_lf_off'} = $offset;
            $$cursor{'clip_tp_off'} = $txt_count;
            $$cursor{'clip_type'} = 0;
         }
      }

      $update = 1;
   }
   elsif ($key == $GCON_KB_CTRL_V)
   {
      ## roll through the clipboard data and spit the characters
      ## into the buffer starting at the current cursor location
      if (($$cursor{'data_type'} == 1) && (length($$cursor{'clip'}) > 0))
      {
         for ($k = 0; $k < (length($$cursor{'clip'})); $k++)
         {
            &move_cursor_advanced($txt_buf, $cursor, ord(substr($$cursor{'clip'}, $k, 1)));
         }
      }
      
      $$cursor{'clip_type'} = -1;    ## deselect
      $update = 1;
   }
   elsif ($key == $GCON_KB_BKS)
   {
      if (defined($$txt_buf[$txt_count]))
      {
         $curlen = length($$txt_buf[$txt_count]);

         ## move the line up and to the end of the previous line...
         if (($offset == 0) && ($txt_count != 0))
         {
            ## move cursor up and to the end of the previous line
            &move_cursor($txt_buf, $cursor, $GCON_KB_UP);
            &move_cursor($txt_buf, $cursor, $GCON_KB_CTRL_BKSLSH);

            ## build the new string from this line and the previous one
            $$txt_buf[$txt_count - 1] = 
                       substr($$txt_buf[$txt_count - 1], 0, length($$txt_buf[$txt_count - 1]) - 1) .
                       substr($$txt_buf[$txt_count], 0, $curlen - 1) . $GCON_ENDCHAR;

            ## clear the line, no longer needed
            splice(@$txt_buf, $txt_count, 1);
         }

         ## delete a character...
         if (($offset < $curlen) && ($offset != 0))
         {
            $$txt_buf[$txt_count] = &del_char_from_line($$txt_buf[$txt_count], $offset - 1);
            &move_cursor($txt_buf, $cursor, $GCON_KB_LF);
         }

         ## or move to the end of the line if we're not in active text
         &move_cursor($txt_buf, $cursor, $GCON_KB_CTRL_BKSLSH) if ($offset >= $curlen);

         $update = 1;
      } ## end if
   }
   elsif (($key == $GCON_KB_ENT) || ($key == $GCON_KB_ENT_N))
   {
      $curlen = (defined($$txt_buf[$txt_count])) ? length($$txt_buf[$txt_count]) : 0;

      ## make sure we have clean, initialized lines to work with
      &init_null_sections($txt_buf, $txt_count + 1);

      ## move all the lines one down in front of the current one
      splice(@$txt_buf, $txt_count, 1, ($$txt_buf[$txt_count], ($GCON_ENDCHAR)));

      if ($offset >= ($curlen - 1))                       ## nothing to move
      {
         $$txt_buf[$txt_count + 1] = $GCON_ENDCHAR;
      }
      else                                                ## stuff to move
      {
         ## put some text in the new line we emptied out
         $$txt_buf[$txt_count + 1] = substr($$txt_buf[$txt_count], $offset);

         ## cut off the last part of the previous line
         $$txt_buf[$txt_count] = substr($$txt_buf[$txt_count], 0, $offset) . $GCON_ENDCHAR;
      }

      &move_cursor($txt_buf, $cursor, $GCON_KB_DN);       ## adjust the cursor
      &move_cursor($txt_buf, $cursor, $GCON_KB_HM);

      $update = 1;
   }
   else
   {
      ## check for input characters (also a function involving cursor motion)
      if (&alphanum($key) || ($key == $GCON_KB_TB))
      {
         ## make sure we are inserting characters into initialized lines
         &init_null_sections($txt_buf, $txt_count);

         $j = ($key == $GCON_KB_TB) ? $GCON_TAB_LEN : 1;  ## adjust in case we're tabbing
         $key = ($key == $GCON_KB_TB) ? ' ' : $key;

         for ($i = 0; $i < $j; $i++)                      ## multiple times if we're tabbing
         {
            $curlen = length($$txt_buf[$txt_count]);

            if ($offset < $curlen)
            {
               $$txt_buf[$txt_count] =
                  add_char_to_line($$txt_buf[$txt_count], chr($key), $offset);
            }
            else
            {
               $$txt_buf[$txt_count] =
                  add_char_to_line_end($$txt_buf[$txt_count], chr($key), $offset, $curlen);
            }
            &move_cursor($txt_buf, $cursor, $GCON_KB_RT); ## move the cursor
         } ## for

         $update = 1;  ## since something was clearly pressed, need to display the code
      }
   } ## end if

   return($update);
}


sub show_bool_query
{
   ## asks a y/n question of the user, returns response

   my ($query_to_show) = @_;
   my $ch = '';

   printf($query_to_show . "\n");

   $loop_this = $GCON_TRUE;
   while ($loop_this)
   {
      $ch = &get_input;
         $loop_this = $GCON_FALSE
      if (($ch == ord('y')) || ($ch == ord('n')) || ($ch == ord('Y')) || ($ch == ord('N')));
   }

   return(($ch == ord('Y')) || ($ch == ord('y')));
}


sub format_line_num_out
{
   ## outputs a line number with necessary number of spaces

   my ($n) = @_;

   ## probably won't be editing files over 1000000 lines anyway, but just in case
   $n %= ($GCON_MAX_LINES * $GCON_MAX_LINES) if ($n > (($GCON_MAX_LINES * $GCON_MAX_LINES) - 1));

   printf("%6ld:", $n);
}


sub draw_screen_text
{
   ## draws the active text area of the screen

   my ($txt_buf, $cursor_ref, $ch, $saved) = @_;
   my $i;

   ## clear our display, whichever system we're on
   system(($^O eq 'MSWin32') ? 'cls' : 'clear');

   ## output terminal title and display size
   printf("noir terminal editor. display: (%d, %d)   ", 
           $$cursor_ref{'max_x'} + 2, $$cursor_ref{'max_y'} + 3);

   ## output if the buffer is synced with the output file
   printf("saved.   ") if ($saved == $GCON_TRUE);

   ## display clipboard function indicator
   printf("sel-") if ($$cursor_ref{'clip_type'} == 0);
   printf("sel-single") if ($$cursor_ref{'clip_type'} == 1);
   printf("sel-multi.") if ($$cursor_ref{'clip_type'} == 2);
   printf("\n");

   ## display all the active text display lines
   for ($i = $$cursor_ref{'min_y'}; $i <= ($$cursor_ref{'max_y'} + 1); $i++)
   {
      $disp_str = '';
      $txt_count = $$cursor_ref{'buf_y'} + $i - 1;

      &format_line_num_out($$cursor_ref{'buf_y'} + $i);

        $disp_str = substr($$txt_buf[$txt_count], $$cursor_ref{'buf_x'}, $$cursor_ref{'max_x'} - 7)
      if ((defined($$txt_buf[$txt_count])) && (length($$txt_buf[$txt_count]) > $$cursor_ref{'buf_x'}));

      $disp_str = $disp_str . ' 'x($$cursor_ref{'max_x'} - 7 - (length($disp_str)));

      if ($$cursor_ref{'y'} == $i)
      {
         $cur_pos = $$cursor_ref{'x'} - 7;
         $disp_str = '>' . substr($disp_str, 0, $cur_pos) . $GCON_CURCHAR . substr($disp_str, $cur_pos + 1);
      }
      else
      {
         $disp_str = ' ' . $disp_str;
      }

      printf($disp_str . "\n");
   } ## for

   ## output the value our last character input
   printf($ch . "\n");

   return($GCON_TRUE);     ## done successfully
}


#***********************************************************************************************************
#
# display functionality...
#
#***********************************************************************************************************


sub handle_size
{
   ## handle screen resizing

   # not implemented in this version
}


sub get_input
{
   ## gets a character of input

   my $key = '';
   while (not defined ($key = ReadKey(-1))) {}

   return(ord($key));
}


sub _display_init
{
   ## display library initialization calls

   ReadMode 4;
}


sub _display_cursor_update
{
   ## updates cursor properties using library routine

   my ($cursor_ref) = @_;

   $$cursor_ref{max_y} = 27;
   $$cursor_ref{max_x} = 80;
}


sub _display_move_cursor
{
   ## moves cursor to specified coordinates

   # nothing required for this implementation
}


sub _display_dump_bare
{
   ## flushes changes/refreshes display without cursor update

   # nothing required for this implementation
}


sub _display_exit
{
   ## display library exit calls

   ReadMode 0;
}

## eof