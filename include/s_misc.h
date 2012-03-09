/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  s_misc.h: A header for the miscellaneous functions.
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: s_misc.h,v 1.2 2003/05/24 00:07:44 demond Exp $
 */

#ifndef INCLUDED_s_misc_h
#define INCLUDED_s_misc_h

#define CHECK_DRONE_TIME 	3
#define GRACE_TIME_DEFAULT 	10
#define SPAM_WAIT_DEFAULT 	5
#define DRONEMAX 			20
#define DRONE_RMAX_DEF 		3
#define DRONE_RPERIOD_DEF 	20
#define DRONE_KTIME_DEF 	300*60

struct Client;
struct ConfItem;

extern char*   date(time_t);
extern const char* smalldate(time_t);
extern char    *small_file_date(time_t);

extern void check_warn_drone(void *);
extern void check_drone_queue(void *);
extern void purge_repeats(void *);
extern void do_plugin(char *, char *);

extern void cluster_prefix(struct Client *, int, char **, int, int, int);

#define _1MEG     (1024.0)
#define _1GIG     (1024.0*1024.0)
#define _1TER     (1024.0*1024.0*1024.0)
#define _GMKs(x)  ( (x > _1TER) ? "Terabytes" : ((x > _1GIG) ? "Gigabytes" : \
                  ((x > _1MEG) ? "Megabytes" : "Kilobytes")))
#define _GMKv(x)  ( (x > _1TER) ? (float)(x/_1TER) : ((x > _1GIG) ? \
               (float)(x/_1GIG) : ((x > _1MEG) ? (float)(x/_1MEG) : (float)x)))
	       

#endif


