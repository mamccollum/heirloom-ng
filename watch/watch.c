/* 
 * watch.c - Keep an eye on a command output
 */
/*
 * Copyright (C) 2023: Luiz Antônio Rangel (takusuman)
 *
 * SPDX-Licence-Identifier: Zlib
 *
 */

#include <curses.h>
#include <errno.h>
#include <pfmt.h>
#include <prerror.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>

static char *progname;
int main(int argc, char *argv[]);
void usage(void);

struct Flag {
	int Beep_on_error, No_title;
}; static struct Flag flag;

int main(int argc, char *argv[]) {
	progname = argv[0];
	// Initialize every integer as 0, to avoid warnings when compiling
	// with -Wconditional-uninitialized.
	int option = 0,
	    interval = 0,
	    ec = 0,
	    term_x = 0,
	    term_y = 0;
	pid_t exec_pid;
	
	// Variables for the information header.
	// Defining nodename from now, since it shouldn't change while we're
	// watching the command.
	time_t now;
	struct utsname u;
	if ( uname(&u) == -1 ) {
		prerror(errno);
		exit(-1);
	}

	while ( (option = getopt(argc, argv, "n:hbt")) != -1 ) {
			switch (option) {
				case 'n':
					interval = atoi(optarg);
					break;
				case 'b':
					flag.Beep_on_error = 1;
					break;
				case 't':
					flag.No_title = 1;
					break;
				case 'h':
				default:
					usage();
			}
	}
	argc -= optind;
	argv += optind;
	
	// Missing operand
	if ( argc < 1 ){
		usage();
	}
	
	// Initialize curses terminal with colours to be used.
	// Get terminal size too, we're going to need it.
	newterm(getenv("TERM"), stdout, stdin);
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	getmaxyx(stdscr, term_y, term_x);

	for (;;) {
		// Get current time to be passed as a string with ctime(3).
		time(&now);
		if ( ! flag.No_title ){
			attron(COLOR_PAIR(1) | A_BOLD);
			// Use a factor of terminal width (term_x) divided by 5,
			// since it seems to work the best to keep the header at
			// a "confortable" size.
			printw("Every %d second(s): %-*s %s: %s\n",
				interval, (term_x/5), argv[0], u.nodename, ctime(&now));
			attroff(COLOR_PAIR(1) | A_BOLD);
		}

		// Not using endwin(3x), since it breaks with multiline
		// also-curses programs, such as ls(1) with the "-l" option.
		reset_shell_mode();
		refresh();

		if ( (exec_pid = fork()) == 0 ) {
			if ( (execvp(argv[0], argv)) == -1 ) {
				prerror(errno);
				exit(-1);
			}
		}
		waitpid(exec_pid, &ec, 0);
	
		// execvp'd command hasn't exit with success and we have
		// flag.Beep_on_error activated.
		if ( ec != 0 && flag.Beep_on_error ) {
			beep();
		}
		
		sleep( (uint)(interval) );

		// Clear terminal for the next cycle.
		clear();
	}
}

void usage(void) {
	pfmt(stderr, MM_NOSTD,
		"usage: %s -n [seconds] [-bt] [command [args...]]\n", progname);
	exit(1);
}