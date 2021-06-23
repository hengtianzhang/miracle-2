#ifndef __LINUX_CPU_H_
#define __LINUX_CPU_H_

enum cpuhp_state {
	CPUHP_INVALID = -1,
	CPUHP_OFFLINE = 0,
};

int __cpu_setup_state(enum cpuhp_state state, const char *name, bool invoke,
			int (*startup)(unsigned int cpu),
			int (*teardown)(unsigned int cpu), bool multi_instance);

static inline int cpu_setup_state(enum cpuhp_state state,
				    const char *name,
				    int (*startup)(unsigned int cpu),
				    int (*teardown)(unsigned int cpu))
{
	return __cpu_setup_state(state, name, true, startup, teardown, false);
}

static inline int cpu_setup_state_nocalls(enum cpuhp_state state,
					    const char *name,
					    int (*startup)(unsigned int cpu),
					    int (*teardown)(unsigned int cpu))
{
	return __cpu_setup_state(state, name, false, startup, teardown,
				   false);
}

static inline int cpu_setup_state_multi(enum cpuhp_state state,
					  const char *name,
					  int (*startup)(unsigned int cpu,
							 struct hlist_node *node),
					  int (*teardown)(unsigned int cpu,
							  struct hlist_node *node))
{
	return __cpu_setup_state(state, name, false,
				   (void *) startup,
				   (void *) teardown, true);
}

extern void boot_cpu_init(void);

#endif /* !__LINUX_CPU_H_ */
