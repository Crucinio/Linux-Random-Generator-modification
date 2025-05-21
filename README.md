# Linux-Random-Generator-modification
This repository contains source code of drivers and test-applications for modifying behaviour of system random generator (random) of OS Linux 

## Experiments 3-4
To build experiments 3-4 you need linux headers of the target kernel versions installed in /src/<your version> and built into modules
/lib/modules/ -> where the built kernel modules are stored
rune make inside the experiment directory
After compiling the drivers you can insert them by running
sudo insmod <driver_name>.ko
and remove it from kernel with:
sudo rmmod <driver_name>

## Experiment 5
Experiment 5 is CLI.
It already contains the modules precompiled for use on a Debian 12 kernel
To substitute modules and add your own ones, you can alter the contents of modules dir.
