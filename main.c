#include <inttypes.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <argp.h>
#include <sys/param.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

typedef struct {
	uint64_t  ctr;
	char     *file;
	uint64_t  start_ns;
	uint64_t  end_ns;
} lua_task;

void *wrapper(void *arg) {
	lua_task *task = (lua_task*)arg;
	struct timespec start_time;
	struct timespec cur_time;
	uint64_t ctr = 0;

	clock_gettime(CLOCK_MONOTONIC, &start_time);
	uint64_t start_time_ns = start_time.tv_sec * 1e9 +  start_time.tv_nsec;
	uint64_t cur_time_ns = start_time_ns;

	lua_State *L;
	L = luaL_newstate();
	luaL_openlibs(L);

	int status = luaL_loadfile(L, task->file);
    	if (status) {
        	fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
        	exit(1);
    	}

        int cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);


	do {
		clock_gettime(CLOCK_MONOTONIC, &cur_time);

		cur_time_ns = cur_time.tv_sec * 1e9 + cur_time.tv_nsec;
		if (start_time_ns + 10 * 1e9 < cur_time_ns) {
			break;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, cb_ref);
		int result = lua_pcall(L, 0, 0, 0);
    		if (result) {
        		fprintf(stderr, "Failed to run script: %s\n", lua_tostring(L, -1));
	        	exit(1);
    		}

		ctr++;

	} while(1);

	task->ctr = ctr;
	task->start_ns = start_time_ns;
	task->end_ns = cur_time_ns;

	return NULL;
}

typedef struct {
	int c, q, d, b;
	char *file_name;
} cmd_line_options;

const char *argp_program_version =
  "lua comp bench 0.0001-alpha-zeta-teta";
const char *argp_program_bug_address =
  "<vlad@cloudflare.com>";

/* Program documentation. */
static char doc[] =
  "Runs lua on multiple threads";

/* A description of the arguments we accept. */
static char args_doc[] = "file1";

static struct argp_option options[] = {
  {"concurency",  'c', "concurency", 0, "Number of threads" },
  { 0 }
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state) {
  cmd_line_options *arguments = state->input;
  switch (key) {
    case 'c':
      arguments->c = atoi(arg);
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 1) {
        argp_usage (state);
      }
      arguments->file_name = arg;
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 1) {
        argp_usage (state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[]) {
	pthread_t *threads;
	lua_task *tasks;
	int i;
	uint8_t *buf;
    	size_t len;
    	struct stat s;
	uint64_t total = 0;

  	cmd_line_options arguments = (cmd_line_options){.file_name = "", .q = 8, .c = 1};
  	argp_parse (&argp, argc, argv, 0, 0, &arguments);
	
	int nthreads = arguments.c;
	threads = malloc(nthreads * sizeof(pthread_t));
	tasks = malloc(nthreads * sizeof(lua_task));

	for (i = 0; i < nthreads; i++) {
		tasks[i] = (lua_task){.file = arguments.file_name};
		pthread_create(&threads[i], NULL, wrapper, &tasks[i]);
	}

	uint64_t start_time_ns = 0;
	uint64_t end_time_ns = 0;

  	for (i = 0; i < nthreads; i++) {
    		pthread_join(threads[i], NULL);
		total += tasks[i].ctr;
		/* Select the earliest starting time and the latest end time. */
		if (start_time_ns == 0) {
			start_time_ns = tasks[i].start_ns;
			end_time_ns = tasks[i].end_ns;
		}
		else {
			start_time_ns = MIN (start_time_ns, tasks[i].start_ns);
			end_time_ns = MAX (end_time_ns, tasks[i].end_ns);
		}
  	}

	uint64_t duration = end_time_ns - start_time_ns;

	fprintf(stderr, "Total times executed: %"PRId64"\n", total);
	fprintf(stdout, "ops/s %.4f\n", (double)total * 1e9 / duration);
}
