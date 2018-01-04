# esseo
A metaphore of life.

## What this project is

This project explores the functionalities of the System V library via a simulation of a group of individuals. Since we are experimenting with IPC, signals and task scheduling, each individual is represented by a separate process.

## Quick start

In order to test the project, open the file called *sim_config* under */source* and tweak the parameters of the simulation. 
The following example contains the parameters you can tweak:

```
init_people=20
genes=500
sim_time=10
birth_death=1
```
 
Once this is done, you can open a terminal under */source* and use the following commands:

* *make run*: compile the project and run it.
* *make*: compile the project. Once this is done, you can use *./a.out* to run the simulation.
* *make test_session*: this is a handy command to use when you want to perform multiple tests automatically. This will performs 10 tests one after another. During a simulation, you can even tweak the parameters in *sim_config*. The updated parameters will be used in the next test.

## Deep testing of the project

Inside the file *life_sim_lib.h* you can find some important globals of the projects :

* **Compile modes**: these definitions control the activation of many points in the code that achieve a behaviour.

i.e.
```
#define CM_SAY_ALWAYS_YES false //if true, individuals will always contact/accept any other one. 
```

* **Log**: these definitions control the activation of logs basing on log types.
On this project, we use the macro *LOG()* to have a log classified by types.

Below there is an example of the behaviour:
```
#define LT_INDIVIDUALS_ACTIONS LOG_ENABLED(true)
#define LT_MANAGER_ACTIONS LOG_ENABLED(false)

...

//Somewhere in the project...
LOG(LT_INDIVIDUALS_ACTIONS,"This will be shown in the output!\n");
LOG(LT_MANAGER_ACTIONS,"This will not =(\n");

```

## Structure of the project

The main parts of the projects are:

* **Manager**: This is the process that manages the simulation: it reads the config file, sets up the simulation, creates new individuals, kills and creates another individuals every *birth-death* seconds and print stats at the end. All this code is in *life_simulator.c*.
* **Individuals**: Each individual tries to find a partner that could give him good descendants. It holds a structure called *ind_data* containing name, type (A or B), genome and pid. This structure is shared accross individuals whenever it needs to. All this code is in *individual.c*.
* **The simulation library**: We have a shared library between the manager and individuals. This library contains useful globals (as described in the section above) as well as some functions of general need. The code is in *life_sim_lib.c* and *life_sim_lib.h*.
* **The stats library**: The stats functions are only used by the manager, but we decided to put this in another file to avoid the bloating of the manager file, which is already long enough. The code is in *life_sim_stats.c* and *life_sim_stats.h*.

## How IPC and semaphores are used

Given the nature of this project, it heavily relies on IPC.

Type A individuals use **shared memory** to publish their data. The shared array of their ind_data (see the section above under *'individuals'* is called *agenda* throughout the project). Shared memory is also used to keep count of the current population and to keep track of every individual's pid, this will be useful later to choose which processes should be killed and to kill every process at simulation end.

**Message queues** are heavily used by individuals: Each time a type B individuals identifies a good match via the *agenda*, it sends a message containing its *ind_data*. The A type will answer with a 'Y' if it accepts, 'N' otherwise. When a couple is formed, both the individuals will send a message to the manager specifying their data. This will let the manager create descendants. During the project we noticed that a single message queue would get quickly flooded and couldn't always handle every message properly, this is why there are two separate message queues: one is used for the communication between individuals (*msgid_proposals*) and the other for the manager (*msgid_common*). 

**Signals** are used by the manager to take action every *birth_death* seconds and subsequently to kill the individual with the lowest pid (which is basically the oldest individual). When this happens, the manager removes that pid from the *alive_individuals* array in shared memory, then SIGUSR1 signal is sent to and handled by the individual, which then removes its own shared data and exits.

In order to cope with the critical sections created by the shared memory, we used a mutex **semaphore**. Another semaphore is used at the beginning of the simulation, since individuals can start their life only when the simulation is ready to start (all the initial individuals created).

## Some implementation details

###### Useful macros
There is a heavy use of macros to make the code more readable (once you know the macros!):
* LOG macros are used to properly log something, given a log type (this is useful to filter the data when testing)
* MUTEX_P and MUTEX_V work just like they look to. They perform the corresponding semaphore operation
* TEST_ERROR is used after some system call which may fail, setting errno to the corresponding value. If this happens, the error is logged.
* You may notice the *bool* type used with also *true* and *false* (we are in c!). This was done to have this handy type and make the code readable. It corresponds to a *char*. But you can forget it and use it like a boolean.

###### Limits of the project
The project has been tested and works well for any number of individuals. If the *genes* parameter is low, the individuals will be more prolific and names will become longer. There is a maximum length of name, defined inside the lib. Since in Linux there is a limit to the maximum number of child processes, we set to 1000 the maximum *init_people*.
During the project, an important limit came out: the msgqueue could be filled of messages and this could cause some deadlocks in some situations. Since this is a problem with high populations with low genomes, we added a check that avoids the sending of messages basing on the current messages in the queue.

###### How individuals evaluate and adapt
Each individual tries to find a partner that maximizes the greatest common divisor of the genomes. However, since an individual could be refused by anyone, it has to have an adaptation mechanism to increase the odds to reproduce. This is achieved by a value called *acceptance_threshold*. This value is initially set to the maximum and then decreased towards 0. An individual with threshold 0 will accept any individual. The value is decreased each time the individual gets refused by a number of times equal to the current population of opposite type.
The evaluation is performed by checking if the gcd is at least *acceptance_threshold* percentage of the value *genome* / 2 (since this is the maximum possible gcd excluding the number itself - given that we always accept a multiple). This means that an acceptance threshold of 75% will lead to acceptation only if the gcd is at least 75% of the current genome/2.
