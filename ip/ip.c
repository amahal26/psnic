/*
 * ip.c		"ip" utility frontend.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <limits.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <regex.h>
#include <getopt.h>
#include <stdbool.h>
#include <time.h>

#include "utils.h"
#include "ip_common.h"

int preferred_family = AF_UNSPEC;
int human_readable;
int use_iec;
int show_stats;
int show_details;
int oneline;
int brief;
int json;
int timestamp;
int force;
int max_flush_loops = 10;
int batch_mode;
bool do_all;
char pid_list[1024][100];

struct rtnl_handle rth = { .fd = -1 };

static void usage(void) __attribute__((noreturn));

static void usage(void)
{
	fprintf(stderr,
		"Usage: ip [ OPTIONS ] OBJECT { COMMAND | help }\n"
		"       ip [ -force ] -batch filename\n"
		"where  OBJECT := { link | address | addrlabel | route | rule | neigh | ntable |\n"
		"                   tunnel | tuntap | maddress | mroute | mrule | monitor | xfrm |\n"
		"                   netns | l2tp | fou | macsec | tcp_metrics | token | netconf | ila |\n"
		"                   vrf | sr | nexthop | mptcp }\n"
		"       OPTIONS := { -V[ersion] | -s[tatistics] | -d[etails] | -r[esolve] |\n"
		"                    -h[uman-readable] | -iec | -j[son] | -p[retty] |\n"
		"                    -f[amily] { inet | inet6 | mpls | bridge | link } |\n"
		"                    -4 | -6 | -I | -D | -M | -B | -0 |\n"
		"                    -l[oops] { maximum-addr-flush-attempts } | -br[ief] |\n"
		"                    -o[neline] | -t[imestamp] | -ts[hort] | -b[atch] [filename] |\n"
		"                    -rc[vbuf] [size] | -n[etns] name | -N[umeric] | -a[ll] |\n"
		"                    -c[olor]}\n");
	exit(-1);
}

static int do_help(int argc, char **argv)
{
	usage();
	return 0;
}

void separate_enter(char *s){
	char *p=s;
	p=strstr(s,"\n");
	if(p!=NULL){
		strcpy(p,p+strlen("\n"));
	}
}

void separate_space(char *s){
	char *p=s;
	p=strstr(s," ");
	if(p!=NULL){
		strcpy(p,p+strlen(" "));
		separate_space(p+1);
	}
}

pid_t Fork(void){
	pid_t	pid;

	pid = fork ();
	if (-1 == pid){
		perror("can not fork");
	}
	return pid;
}

void seach_vnic(int count){
	int	i=0;
	for (i=0; i<count; ++i){
		pid_t pid=Fork();
		if(pid==-1) break;
		else if(pid==0){
			get_vnic(pid_list[i]);
			exit (EXIT_SUCCESS);
		}
		else{
			int	status=0;
			pid=wait(&status);
		}
	}
}

int main(int argc, char **argv)
{
	char *basename;
	int color = 0;

	drop_cap();

	basename = strrchr(argv[0], '/');
	if (basename == NULL)
		basename = argv[0];
	else
		basename++;

	_SL_ = oneline ? "\\" : "\n";

	check_enable_color(color, json);

	if (rtnl_open(&rth, 0) < 0)
		exit(1);

	rtnl_set_strict_dump(&rth);
	//printf("do exec\n");
	if(argc==1){
		FILE *fp;
		char *cmdline="pgrep envoy";
		if((fp=popen(cmdline,"r"))==NULL){
			perror("Searching pid command fail");
			exit(EXIT_FAILURE);
		}
		int i=0;
		while(!feof(fp)){
			fgets(pid_list[i], sizeof(pid_list[i]), fp);
			separate_enter(pid_list[i]);
			separate_space(pid_list[i]);
			i++;
		}
		(void) pclose(fp);
		i--;

		seach_vnic(i);

		exit (EXIT_SUCCESS);
	}
	else if(argc!=2&&strcmp(argv[1],ANOTHER_KEY)==0) coll_name(argv);
	else printf("No command\n"); 

	return 0;
	rtnl_close(&rth);
	usage();
}
