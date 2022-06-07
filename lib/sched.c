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
	static struct Env *e;
    static int count = 0; // remaining time slices of current env
    static int point = 0; // current env_sched_list index
    // printf("Sched_yield. envid = %d\n", curenv ? curenv->env_id : 0);
    e = curenv; // 需要加上这一句，因为如果有程序自己退出了，这里的e应该还是上次的e，会出现错误

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
	/* Step1: count == 0 indicates that time is up, change executing env.*/
	/* Actually Not only count == 0 we need to change Env, but current process become NULL or NOT_RUNNABLE, we also need to change. */
    if (count == 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			// Current env's time is over, change to another.
        	LIST_REMOVE(e, env_sched_link); //Step1: delete e from current sched list.
        	LIST_INSERT_TAIL(&env_sched_list[1 - point], e, env_sched_link); // Step2:insert e into another sched list.
		}
		/* Step2: searching for a runnable thread. if not exists, change to another list. */
        LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
            if (e->env_status == ENV_RUNNABLE) {
                count = e->env_pri;
				// printf("count = %d(from priority)\n", count);
                count -= 1;
                env_run(e);
				break;
				/* It will never return. */
            }
        }
        /* Step3: ENV_RUNNABLE NOT HAVE, change list. */
        point = 1 - point;

        /* Step4: Search another thread. */
        LIST_FOREACH(e, &env_sched_list[point], env_sched_link) {
            if (e->env_status == ENV_RUNNABLE) {
                count = e->env_pri;
				// printf("count = %d(from priority)\n", count);
                count -= 1;
                env_run(e);
				break;
            }
        }
		while(1);
    }
    else {
		// printf("Other dir.\n");
        count -= 1;
		env_run(e);
    }
}
