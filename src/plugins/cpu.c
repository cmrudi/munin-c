/*
 * Copyright (C) 2008-2013 Helmut Grohne <helmut@subdivi.de> - All rights reserved.
 * Copyright (C) 2013 Steve Schnepp <steve.schnepp@pwkf.org> - All rights reserved.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2 or v.3.
 */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include "common.h"
#include "plugins.h"

#define SYSWARNING 30
#define SYSCRITICAL 50
#define USRWARNING 80

/* TODO: port support for env.foo_warning and env.foo_critical from mainline plugin */

static int print_stat_value(const char* field_name, const char* stat_value, int hz_) {
	return printf("%s.value %llu\n", field_name, strtoull(stat_value, NULL, 0) * 100 / hz_);
}

int cpu(int argc, char **argv) {
	FILE *f;
	char buff[256], *s;
	int ncpu=0, extinfo=0, hz_;
	bool scaleto100 = false;
	if(argc > 1) {
		if(!strcmp(argv[1], "config")) {
			s = getenv("scaleto100");
			if(s && !strcmp(s, "yes"))
				scaleto100 = true;

			if(!(f=fopen(PROC_STAT, "r")))
				return fail("cannot open " PROC_STAT);
			while(fgets(buff, 256, f)) {
				if(!strncmp(buff, "cpu", 3)) {
					if(xisdigit(buff[3]))
						ncpu++;
					if(buff[3] == ' ' && 0 == extinfo) {
						strtok(buff+4, " \t");
						for(extinfo=1;strtok(NULL, " \t");extinfo++)
							;
					}
				}
			}
			fclose(f);

			if(ncpu < 1 || extinfo < 4)
				return fail("cannot parse " PROC_STAT);

			puts("graph_title CPU usage");
			if(extinfo >= 7)
				puts("graph_order system user nice idle iowait irq softirq");
			else
				puts("graph_order system user nice idle");
			if(scaleto100)
				puts("graph_args --base 1000 -r --lower-limit 0 --upper-limit 100");
			else
				printf("graph_args --base 1000 -r --lower-limit 0 --upper-limit %d\n", 100 * ncpu);
			puts("graph_vlabel %\n"
				"graph_scale no\n"
				"graph_info This graph shows how CPU time is spent.\n"
				"graph_category system\n"
				"graph_period second\n"
				"system.label system\n"
				"system.draw AREA");
			printf("system.max %d\n", 100 * ncpu);
			puts("system.min 0\n"
				"system.type DERIVE");
			printf("system.warning %d\n", SYSWARNING * ncpu);
			printf("system.critical %d\n", SYSCRITICAL * ncpu);
			puts("system.info CPU time spent by the kernel in system activities\n"
				"user.label user\n"
				"user.draw STACK\n"
				"user.min 0");
			printf("user.max %d\n", 100 * ncpu);
			printf("user.warning %d\n", USRWARNING * ncpu);
			puts("user.type DERIVE\n"
				"user.info CPU time spent by normal programs and daemons\n"
				"nice.label nice\n"
				"nice.draw STACK\n"
				"nice.min 0");
			printf("nice.max %d\n", 100 * ncpu);
			puts("nice.type DERIVE\n"
				"nice.info CPU time spent by nice(1)d programs\n"
				"idle.label idle\n"
				"idle.draw STACK\n"
				"idle.min 0");
			printf("idle.max %d\n", 100 * ncpu);
			puts("idle.type DERIVE\n"
				"idle.info Idle CPU time");
			if(scaleto100)
				printf("system.cdef system,%d,/\n"
					"user.cdef user,%d,/\n"
					"nice.cdef nice,%d,/\n"
					"idle.cdef idle,%d,/\n", ncpu, ncpu, ncpu, ncpu);
			if(extinfo >= 7) {
				puts("iowait.label iowait\n"
					"iowait.draw STACK\n"
					"iowait.min 0");
				printf("iowait.max %d\n", 100 * ncpu);
				puts("iowait.type DERIVE\n"
					"iowait.info CPU time spent waiting for I/O operations to finish\n"
					"irq.label irq\n"
					"irq.draw STACK\n"
					"irq.min 0");
				printf("irq.max %d\n", 100 * ncpu);
				puts("irq.type DERIVE\n"
					"irq.info CPU time spent handling interrupts\n"
					"softirq.label softirq\n"
					"softirq.draw STACK\n"
					"softirq.min 0");
				printf("softirq.max %d\n", 100 * ncpu);
				puts("softirq.type DERIVE\n"
					"softirq.info CPU time spent handling \"batched\" interrupts");
				if(scaleto100)
					printf("iowait.cdef iowait,%d,/\n"
						"irq.cdef irq,%d,/\n"
						"softirq.cdef softirq,%d,/\n", ncpu, ncpu, ncpu);
			}
			if(extinfo >= 8) {
				puts("steal.label steal\n"
					"steal.draw STACK\n"
					"steal.min 0");
				printf("steal.max %d\n", 100 * ncpu);
				puts("steal.type DERIVE\n"
					"steal.info The time that a virtual CPU had runnable tasks, but the virtual CPU itself was not running");
				if(scaleto100)
					printf("steal.cdef steal,%d,/\n", ncpu);
			}
			if(extinfo >= 9) {
				puts("guest.label guest\n"
					"guest.draw STACK\n"
					"guest.min 0");
				printf("guest.max %d\n", 100 * ncpu);
				puts("guest.type DERIVE\n"
					"guest.info The time spent running a virtual CPU for guest operating systems under the control of the Linux kernel.");
				if(scaleto100)
					printf("guest.cdef guest,%d,/\n", ncpu);
			}
			return 0;
		}
		if(!strcmp(argv[1], "autoconf"))
			return autoconf_check_readable(PROC_STAT);
	}
	if(!(f=fopen(PROC_STAT, "r")))
		return fail("cannot open " PROC_STAT);
	hz_ = getenvint("HZ", 100);
	while(fgets(buff, 256, f)) {
		if(!strncmp(buff, "cpu ", 4)) {
			fclose(f);
			if(!(s = strtok(buff+4, " \t")))
				break;
			print_stat_value("user", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				break;
			print_stat_value("nice", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				break;
			print_stat_value("system", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				break;
			print_stat_value("idle", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				return 0;
			print_stat_value("iowait", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				return 0;
			print_stat_value("irq", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				return 0;
			print_stat_value("softirq", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				return 0;
			print_stat_value("steal", s, hz_);
			if(!(s = strtok(NULL, " \t")))
				return 0;
			print_stat_value("guest", s, hz_);
			return 0;
		}
	}
	fclose(f);
	return fail("no cpu line found in " PROC_STAT);
}
