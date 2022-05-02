#include <env.h>
#include <pmap.h>
#include <printf.h>

/* Overview:
 *  Implement simple round-robin scheduling.
 *
 *
 * Hints:
 *  1. The variable which is for counting should be defined as 'static'.
 *  2. Use variable 'env_sched_list', which is a pointer array.
 *  3. CANNOT use `return` statement!
 */
/*** exercise 3.15 ***/
void sched_yield(void)
{
	// env_run(LIST_FIRST(env_sched_list));
	static struct Env *e;
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    
    /*  hint:
     *  1. if (count==0), insert `e` into `env_sched_list[1-point]`
     *     using LIST_REMOVE and LIST_INSERT_TAIL.
     *  2. if (env_sched_list[point] is empty), point = 1 - point;
     *     then search through `env_sched_list[point]` for a runnable env `e`, 
     *     and set count = e->env_pri
     *  3. count--
     *  4. env_run()
     *
     *  functions or macros below may be used (not all):
     *  LIST_INSERT_TAIL, LIST_REMOVE, LIST_FIRST, LIST_EMPTY
     */
	// printf("begin change env... count = %d\n", count);
    if (count == 0) {
		// printf("Step1\n");
		if (e != NULL) {
			// Current env's time is over, change to another.
        	LIST_REMOVE(e, env_sched_link); //Step1: delete e from current sched list.
        	LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link); // Step2:insert e into another sched list.
		}
		// printf("Step2\n");
        if (LIST_EMPTY(&env_sched_list[point])) {
            point = 1 - point;
        }
		// printf("Step3\n");
        LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
            if (e->env_status == ENV_RUNNABLE) {
                count = e->env_pri;
				// printf("count = %d(from priority)\n", count);
                count -= 1;
                env_run(e);
				break;
            }
        }
    }
    else {
		// printf("Other dir.\n");
        count -= 1;
		env_run(e);
    }
}
