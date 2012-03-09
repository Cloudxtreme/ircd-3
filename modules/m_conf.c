/************************************************************************
 *   IRC - Internet Relay Chat, modules/m_conf.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
 *
 *   See file AUTHORS in IRC package for additional names of
 *   the programmers. 
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   $Id: m_conf.c,v 1.5 2003/05/24 00:07:44 demond Exp $
 */
#include "stdinc.h"
#include "handlers.h"
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_serv.h"
#include "s_user.h"
#include "s_log.h"
#include "s_misc.h"
#include "send.h"
#include "msg.h"
#include "parse.h"
#include "modules.h"

static void ms_conf(struct Client*, struct Client*, int, char**);

struct Message conf_msgtab = {
  "CONF", 0, 0, 2, 0, 0, 0,
  {m_unregistered, m_ignore, ms_conf, m_ignore}
};

#ifndef STATIC_MODULES
void
_modinit(void)
{
  mod_add_cmd(&conf_msgtab);
}

void
_moddeinit(void)
{
  mod_del_cmd(&conf_msgtab);
}

const char *_version = "$Revision: 1.5 $";
#endif
/*
 * ms_conf - CONF command handler
 *      parv[0] = sender prefix
 *      parv[1] = target server mask
 *      parv[2] = general variable
 *		parv[3] = new value
 */
static void ms_conf(struct Client* client_p, struct Client* source_p,
                      int parc, char* parv[])
{
	char buf[BUFMAX];
	
	if (ServerInfo.gateway && get_bridge_token(client_p))
		return;

	ConfigFileEntry.sframe = (long)buf;
  if (parc < 3)
    {
      sendto_server(client_p, source_p, NULL, CAP_SVS, NOCAPS, LL_ICLIENT, 
      				":%s CONF %s", parv[0], parv[1]);

	  if (match(parv[1], me.name)) {
		if (!find_z_conf(parv[0]))
			return;

		sendto_realops_flags(FLAGS_ALL, L_ADMIN,
			"%s is remotely rehashing server config file", parv[0]);
		ilog(L_NOTICE, "Remote REHASH from %s", parv[0]);

		rehash(0);
	  }

      return;
    }

  if (parc < 4)
    return;

  sendto_server(client_p, source_p, NULL, CAP_SVS, NOCAPS, LL_ICLIENT,
				":%s CONF %s %s %s :%s", parv[0], parv[1], parv[2],
				parc > 4? parv[3] : "", parc > 4? parv[4] : parv[3]);

  if (!match(parv[1], me.name))
	return;
  if (!find_z_conf(parv[0]))
	return;

  if (!irccmp(parv[2], "hide_server")) {

	  if (!irccmp(parv[3], "yes"))
		  ConfigFileEntry.hide_server = 1;
	  else if (!irccmp(parv[3], "no"))
		  ConfigFileEntry.hide_server = 0;

  } else if (!irccmp(parv[2], "netid")) {

	/*strncpy(buf, parv[3], BUFFMAX);*/
	strncpy(ConfigFileEntry.netid, parv[3],
	      MAXNETID)[MAXNETID] = 0;

  } else if (!irccmp(parv[2], "cyrillic")) {

	  if (!irccmp(parv[3], "yes"))
		  ConfigFileEntry.cyrillic = 1;
	  else if (!irccmp(parv[3], "no"))
		  ConfigFileEntry.cyrillic = 0;

  } else if (!irccmp(parv[2], "totaltrans")) {

	  if (!irccmp(parv[3], "yes"))
		  ConfigFileEntry.totaltrans = 1;
	  else if (!irccmp(parv[3], "no"))
		  ConfigFileEntry.totaltrans = 0;

  } else if (!irccmp(parv[2], "grace_time")) {

	ConfigFileEntry.grace_time = atoi(parv[3]);		

  } else if (!irccmp(parv[2], "spam_wait")) {

	ConfigFileEntry.spam_wait = atoi(parv[3]);		

  } else if (!irccmp(parv[2], "spawn")) {

	  do_spawn(atoi(parv[3]), parc > 4? parv[4] : NULL);	

  } else if (!irccmp(parv[2], "plugin")) {

	  do_plugin(parv[3], parc > 4? parv[4] : NULL);	

#ifndef STATIC_MODULES

  } else if (!irccmp(parv[2], "modunload")) {

	  unload_one_module(parv[3], 0);	

  } else if (!irccmp(parv[2], "modload")) {

	  char *m_bn = irc_basename(parv[3]);

	  if (findmodule_byname(m_bn) == -1)
		load_a_module(parv[3], 0, 0);
	  
	  MyFree(m_bn);
	  
#endif	  

  } else if (!irccmp(parv[2], "drone_kline")) {

	  if (!irccmp(parv[3], "yes"))
		  ConfigFileEntry.drone_kline = 1;
	  else if (!irccmp(parv[3], "no"))
		  ConfigFileEntry.drone_kline = 0;

  } else if (!irccmp(parv[2], "drone_batch")) {

	  if (!irccmp(parv[3], "yes"))
		  ConfigFileEntry.drone_batch = 1;
	  else if (!irccmp(parv[3], "no"))
		  ConfigFileEntry.drone_batch = 0;

  }
}
