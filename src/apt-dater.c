/* $Id$
 *
 * Makes our customer Linux updates faster.
 *
 */

#include "apt-dater.h"
#include "keyfiles.h"
#include "ui.h"
#include "stats.h"
#include "sighandler.h"
#include "lock.h"

CfgFile *cfg = NULL;
GMainLoop *loop = NULL;
gboolean rebuilddl = FALSE;
time_t oldest_st_mtime;

int main(int argc, char **argv)
{
 char opts;
 char *cfgfilename = NULL;
 GList *hosts = NULL;

 cfgfilename = g_strdup_printf("%s/%s/%s", g_get_user_config_dir(), PROG_NAME, "apt-dater.conf");
 g_set_prgname(PROG_NAME);

 while ((opts = getopt(argc, argv, "c:")) != EOF) {
  switch(opts) {
  case 'c':
   if(cfgfilename) free(cfgfilename);
   cfgfilename = (char *) strdup(optarg);
   break;
  default:
   g_printerr("Usage: %s [-c config]\n", g_get_prgname());
   exit(EXIT_FAILURE);
  }
 }

 if(!cfgfilename) {
  g_printerr("Out of memory\n");
  exit(EXIT_FAILURE);
 }

 if(!(cfg = (CfgFile *) loadConfig(cfgfilename))) {
  g_printerr("Error on loading config file %s\n", cfgfilename);
  exit(EXIT_FAILURE);
 }

 if(!(hosts = (GList *) loadHosts(cfg->hostsfile))) {
  g_printerr("Error on loading config file %s\n", cfg->hostsfile);
  exit(EXIT_FAILURE);
 }

 getOldestMtime(hosts);

 doUI(hosts);
 setSigHandler();

 loop = g_main_loop_new (NULL, FALSE);

 g_timeout_add(1000, (GSourceFunc) refreshStats, hosts);
 g_idle_add ((GSourceFunc) ctrlUI, hosts);

 /* Startup the main loop */
 g_main_loop_run (loop);

 g_main_loop_unref (loop);
 cleanUI();
 cleanupLocks();

 freeConfig(cfg);
 free(cfgfilename);

 exit(EXIT_SUCCESS);
}