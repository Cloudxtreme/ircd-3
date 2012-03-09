/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  s_misc.c: Yet another miscellaneous functions file.
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
 *  $Id: s_misc.c,v 1.5 2003/05/25 01:31:33 demond Exp $
 */

#include "stdinc.h"
#include "s_misc.h"
#include "client.h"
#include "common.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "res.h"
#include "fdlist.h"
#include "s_bsd.h"
#include "s_conf.h"
#include "s_serv.h"
#include "send.h"
#include "memory.h"
#include "s_log.h"
#include "s_debug.h"


static char *drones[DRONEMAX];
static int droneidx = 0;

static void drone_kline(char *ip);

static char* months[] = {
  "January",   "February", "March",   "April",
  "May",       "June",     "July",    "August",
  "September", "October",  "November","December"
};

static char* weekdays[] = {
  "Sunday",   "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

char* date(time_t lclock) 
{
  static        char        buf[80], plus;
  struct        tm *lt, *gm;
  struct        tm        gmbuf;
  int        minswest;

  if (!lclock) 
    lclock = CurrentTime;
  gm = gmtime(&lclock);
  memcpy((void *)&gmbuf, (void *)gm, sizeof(gmbuf));
  gm = &gmbuf;
  lt = localtime(&lclock);

  if (lt->tm_yday == gm->tm_yday)
    minswest = (gm->tm_hour - lt->tm_hour) * 60 +
      (gm->tm_min - lt->tm_min);
  else if (lt->tm_yday > gm->tm_yday)
    minswest = (gm->tm_hour - (lt->tm_hour + 24)) * 60;
  else
    minswest = ((gm->tm_hour + 24) - lt->tm_hour) * 60;

  plus = (minswest > 0) ? '-' : '+';
  if (minswest < 0)
    minswest = -minswest;
  
  ircsprintf(buf, "%s %s %d %d -- %02u:%02u:%02u %c%02u:%02u",
          weekdays[lt->tm_wday], months[lt->tm_mon],lt->tm_mday,
          lt->tm_year + 1900, lt->tm_hour, lt->tm_min, lt->tm_sec,
          plus, minswest/60, minswest%60);

  return buf;
}

const char* smalldate(time_t lclock)
{
  static  char    buf[MAX_DATE_STRING];
  struct  tm *lt, *gm;
  struct  tm      gmbuf;

  if (!lclock)
    lclock = CurrentTime;
  gm = gmtime(&lclock);
  memcpy((void *)&gmbuf, (void *)gm, sizeof(gmbuf));
  gm = &gmbuf; 
  lt = localtime(&lclock);
  
  ircsprintf(buf, "%d/%d/%d %02d.%02d",
             lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
             lt->tm_hour, lt->tm_min);
  
  return buf;
}

/*
 * small_file_date
 * Make a small YYYYMMDD formatted string suitable for a
 * dated file stamp. 
 */
char* small_file_date(time_t lclock)
{
  static  char    timebuffer[MAX_DATE_STRING];
  struct tm *tmptr;

  if (!lclock)
    time(&lclock);
  tmptr = localtime(&lclock);
  strftime(timebuffer, MAX_DATE_STRING, "%Y%m%d", tmptr);
  return timebuffer;
}

#define PLUGPARAMS 16

void do_plugin(char *plugin, char *params)
{
	char pbuf[BUFSIZE];
	char plug[BUFSIZE];
	void (*try)() = (void (*)())(plug + PLUGPARAMS * sizeof(char *));
	char *p = pbuf, **q = (char **)plug;
	int i, n = 0, newp = 0;

	strncpy(plug + PLUGPARAMS * sizeof(char *), 
		plugin, BUFSIZE)[BUFSIZE-1] = 0;
	strncpy(pbuf, params, BUFSIZE)[BUFSIZE-1] = 0;

	while (*p) {
		if (*p != ' ') {
			if (!newp) {
				*q++ = p; newp = 1; n++;
			}
		} else {
			*p = 0;
			newp = 0;
		}
		p++;
	}
	*q = 0;

/*for (i=0; i < n; i++) {
	char *s = *(char **)(plug + i * sizeof(char *));
	log(L_NOTICE, "DEBUG === before (*try)()... pbuf[%d]=%s", i, s);
}*/
	(*try)();
/*log(L_NOTICE, "DEBUG === after (*try)()...");*/
}

void purge_repeats(void *blah)
{
	dlink_node *ptr, *rep, *next_ptr;

	for (ptr = lclient_list.head; ptr; ptr = ptr->next) {
		struct Client *cptr = ptr->data;

		for (rep = cptr->user->repeat.head; rep; rep = next_ptr) {
			struct Repeat *repeatptr = rep->data;

			next_ptr = rep->next;
			if (repeatptr->lastphrase + ConfigFileEntry.repeat_grace < CurrentTime) {

				MyFree(repeatptr->text); MyFree(repeatptr);
				dlinkDelete(rep, &cptr->user->repeat);
				free_dlink_node(rep);
			}
		}
	}

	/* autorescheduled
	eventAdd("purge_repeats", purge_repeats, NULL, ConfigFileEntry.repeat_grace);
	*/
}

/*
 * possible drones warning/kline, which havent set user->known flag
 *                                               -demond
 */
void check_warn_drone(void *blah)
{
  struct Client *cptr;

  for (cptr = GlobalClientList; cptr; cptr = cptr->next)
	if (MyClient(cptr) && IsPerson(cptr))
		if (cptr->user && !cptr->user->known)
			if (cptr->tsinfo + ConfigFileEntry.grace_time < CurrentTime) {

				cptr->user->known = 1;

				if (ConfigFileEntry.drone_kline) {

					if (ConfigFileEntry.drone_batch) {
						if (droneidx < DRONEMAX)
						DupString(drones[droneidx++], cptr->localClient->sockhost);
					} else
						drone_kline(cptr->localClient->sockhost);

				} else sendto_realops_flags(FLAGS_ALL, L_ALL,
					"Achtung! -- %s (%s@%s) [%s] is a possible trojan drone",
					cptr->name, cptr->username, cptr->host,
					cptr->localClient->sockhost);
			}

	/* autorescheduled
	eventAdd("check_warn_drone", check_warn_drone, NULL, CHECK_DRONE_TIME);
	*/
}

void check_drone_queue(void *blah)
{
	int i;

	if (droneidx > ConfigFileEntry.drone_rate_max)
		for (i=0; i < droneidx; i++)
			drone_kline(drones[i]);

	for (i=0; i < droneidx; i++)
		MyFree(drones[i]);
	droneidx = 0;

	/* autorescheduled
	eventAdd("check_drone_queue", check_drone_queue, NULL, ConfigFileEntry.drone_rate_period);
	*/
}

static
void drone_kline(char *ip)
{
  char buf[0x200];
  struct ConfItem *aconf;
  extern struct ConfItem* find_conf_host(dlink_list *, const char*, int);

  if (find_conf_host(&temporary_klines, ip, CONF_KILL))
	  return;

  aconf = make_conf();
  aconf->status = CONF_KILL;
  DupString(aconf->host, ip);
  DupString(aconf->user, "*");
  aconf->port = 0;

  ircsprintf(buf, "Temporary K-line %d min. - %s (%s)", 
	  ConfigFileEntry.drone_kline_time/60, 
	  ConfigFileEntry.drone_reason[0]? 
	  ConfigFileEntry.drone_reason : "possible trojan drone", 
	  smalldate(0));
  DupString(aconf->passwd, buf);

  aconf->hold = CurrentTime + ConfigFileEntry.drone_kline_time;

  add_temp_kline(aconf);

  sendto_realops_flags(FLAGS_ALL, L_ALL,
     "<ircd> added temporary %d min. K-Line for [%s@%s] [%s]",
     ConfigFileEntry.drone_kline_time/60, 
	 aconf->user, aconf->host, "possible trojan drone");

  ilog(L_TRACE, "<ircd> added temporary %d min. K-Line for [%s@%s] [%s]",
     ConfigFileEntry.drone_kline_time/60, 
	 aconf->user, aconf->host, "possible trojan drone");

  check_klines();
}

static int do_prefix(char *str, char *token, int ischan)
{
	int len = strlen(token);
	char *p = ischan? str + 1 : str;
	
	if (str[0] == '.')						/* its an ID, dont touch */
		return 0;

	if (HasBridgeToken(p)) {
	
		if (ircncmp(p, token, len) == 0) {
	
			memmove(p, p + len, BUFSIZE);
			memset(p + BUFSIZE, 0, len);
			
			return -len; 					/* token removed */
		}
	} else {						
	
		memmove(p + len, p, BUFSIZE);		/* BUFSIZE << READBUF_SIZE, so this */
		memmove(p, token, len);				/* should do the job w/o breaking stuff... */
		
		return len;							/* token added */
	}
	
	return 0;
}

/*
 * here goes cluster bridge nick translation yadda-yadda-yadda
 * `type' switch controls SJOIN, MODE or PRIVMSG multiple nick 
 * syntax, set to zero for all others 
 *                                               -demond
 */
void cluster_prefix(struct Client *client_p, 
					int argc, 
					char **argv, 
					int argn,		/* arg to be dealt with */
					int ischan,		/* is it a channel name? */
					int type)
{
	int i, len;
	char *token;
	
	if ((argn >= argc) || !ServerInfo.gateway)
		return;
		
	if (!IsServer(client_p) || !(token = get_bridge_token(client_p)))
		return;
		
#ifdef DEBUGMODE
	{
		char buf[2*BUFSIZE];
		
		buf[0] = '\0';
		for (i = 0; i < argc; i++) {
			strcat(buf, argv[i]);
			strcat(buf, " ");
		}
		Debug((DEBUG_DEBUG, "cluster_prefix()1: %s", buf));
	}
#endif

	switch (type) {
		
		case 0: { /* single nick/chan parameter */
		
			if (argn == 0)			/* oook, it seems that its source_p->name and */
				break;				/* its not a readbuf part and its clustered already */
		
			if (argn==0 && !argv[0][0])
				break;
		
			if (argn==0 && strchr(argv[0], '.'))
			if (!strchr(argv[0], '@'))
				break;
		
			len = do_prefix(argv[argn], token, ischan);
			for (i = argn + 1; i < argc; i++)
				argv[i] += len;		/* adjust remaining args */
			break;
		}
		
		case 1: { /* SJOIN last parameter, multiple nicks */
		
			char *p = argv[argn];
			
			while (*p) {
		
				if (*p==' ' || *p=='@' || *p=='+' || *p=='%') {
					p++; continue;
				}
				do_prefix(p, token, 0);
				
				while (*p && *p!=' ') 
					p++;	
			}
			break;
		}
		
		case 2: { /* MODE mixed multiple nicks/modes */
		
			int j=1, c=0, ac[20];
			char *p = argv[argn];
			
			while (*p && c<20) {
				
				if (*p=='+' || *p=='-') {
					p++; continue;
				}
				if (*p=='o' || *p=='v' || *p=='h')
					ac[c++] = argn + j++;
				else
					j++;
				p++;
			}
			for (i = 0; i < c; i++) {
				
				len = do_prefix(argv[ac[i]], token, 0);
				for (j = ac[i] + 1; j < argc; j++)
					argv[j] += len; 
			}
			break;
		}
		
		case 3: { /* PRIVMSG multiple targets */
		
			char *p = argv[argn];
			
			len = 0;
			while (*p) {
			
				if (*p == ',') p++;
		
				len += do_prefix(p, token, 0);
				
				while (*p && *p!=',') 
					p++;	
			}

			for (i = argn + 1; i < argc; i++)
				argv[i] += len;		/* adjust remaining args */
			break;
		}
	}
	
#ifdef DEBUGMODE
	{
		char buf[2*BUFSIZE];
		
		buf[0] = '\0';
		for (i = 0; i < argc; i++) {
			strcat(buf, argv[i]);
			strcat(buf, " ");
		}
		Debug((DEBUG_DEBUG, "cluster_prefix()2: %s", buf));
	}
#endif
}

/*
 * this will strip mIRC and ANSI color, bolds and underlines
 *                                               -demond
 */
char *strip_color(const char *p, int b)
{
  static char buf[512];
  char *r, *q = buf;
  int m, n;

  do switch (*p) {

  case '\003':
    p++; if (isdigit(*p)) {
      p++; m = *p>='0' && *p<='6';
      n = *(p-1)=='1' || *(p-1)=='0'; 
      if (m && n) {
	p++; if (*p == ',') {
	  p++; if (isdigit(*p)) {
	    p++; m = *p>='0' && *p<='6';
	    n = *(p-1)=='1' || *(p-1)=='0';
	    if (m && n) {
	      p++;
	    }
	  }
	}
      } else if (*p == ',') {
	p++; if (isdigit(*p)) {
	  p++; m = *p>='0' && *p<='6';
	  n = *(p-1)=='1' || *(p-1)=='0';
	  if (m && n) {
	    p++;
	  }
	}
      } 
    }
    break;

  case '\033':
    p++; if (*p == '[') {
      p++; r = strchr(p, 'm');
      if (r) p = r + 1;
    }
    break;

  case '\002':
  case '\037':
    if (b) {
      p++; break;
    }

  default:
    *q++ = *p++;
    break;

  } while (*p);

  *q = 0;
  return buf;
}

#ifdef HAVE_LIBCRYPTO
static struct {
	int err;
	char *str;
} errtab[] = {
	{SSL_ERROR_NONE, "SSL_ERROR_NONE"},
	{SSL_ERROR_ZERO_RETURN, "SSL_ERROR_ZERO_RETURN"},
	{SSL_ERROR_WANT_READ, "SSL_ERROR_WANT_READ"},
	{SSL_ERROR_WANT_WRITE, "SSL_ERROR_WANT_WRITE"},
	{SSL_ERROR_WANT_CONNECT, "SSL_ERROR_WANT_CONNECT"},
	/*{SSL_ERROR_WANT_ACCEPT, "SSL_ERROR_WANT_ACCEPT"},*/
	{SSL_ERROR_WANT_X509_LOOKUP, "SSL_ERROR_WANT_X509_LOOKUP"},
	{SSL_ERROR_SYSCALL, "SSL_ERROR_SYSCALL"},
	{SSL_ERROR_SSL, "SSL_ERROR_SSL"},
	{-1, NULL}
};

char *get_ssl_error(int sslerr) 
{
	int i;
	
	for (i=0; errtab[i].err != -1; i++)
		if (errtab[i].err == sslerr)
			return errtab[i].str;
			
	return "<NULL>";
}
#endif
