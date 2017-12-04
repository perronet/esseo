#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>


/*
 * Many child processes all writing to the same shared memory
 */


#define NUM_PROC  4
#define NUM_ITER  100000
#define TEST_ERROR    if (errno) {dprintf(STDERR_FILENO,		\
					  "%s:%d: PID=%5d: Error %d (%s)\n", \
					  __FILE__,			\
					  __LINE__,			\
					  getpid(),			\
					  errno,			\
					  strerror(errno));}


// data of the single write
struct single_data {
	unsigned long content;
	pid_t writer;
};

// type of data structure  
struct shared_data {
	// index where next write will happen
	unsigned long cur_idx;
	/*
	 * vector that contains all data to be written:
	 * - vec[i].content = i
	 * - vec[i].writer = PID of process who wrote i in vec[i].content
	 */
	struct single_data vec[NUM_PROC*NUM_ITER];  
};

// Print status of the shared memory m_id onto file descriptor fd
static void shm_print_stats(int fd, int m_id);

// Global variables
int m_id, s_id, num_errors;

int main() {
	int cur_i, i, j, status;
	struct shared_data * my_data;
	pid_t my_pid, child_pid;
	struct sembuf sops;
	
	// Create a shared memory area and attach it to the pointer
	m_id = shmget(IPC_PRIVATE, sizeof(*my_data), 0600);
	TEST_ERROR;
	my_data = shmat(m_id, NULL, 0);
	TEST_ERROR;
	my_data->cur_idx = 0;   // init first counter
	shm_print_stats(2, m_id);
	
	// Create a semaphore to synchronize the start of all child processes
	s_id = semget(IPC_PRIVATE, 2, 0600);
	TEST_ERROR;
	semctl(s_id, 0, SETVAL, 0);   // Sem 0 to syncronize the child processes
    semctl(s_id, 1, SETVAL, 1);   // semaforo 1 inizializzato ad 1 (per sincronizzare nella sezione critica)
	TEST_ERROR;
	// Initialize the common fields
	sops.sem_num = 0;     // check the 0-th semaphore
	sops.sem_flg = 0;     // no flag
	
	// Loop to create NUM_PROC child processes
	for (i=0; i<NUM_PROC; i++) {
		switch (child_pid = fork()) {
		case -1:
			/* Handle error */
			TEST_ERROR;
			break;
			
		case 0:
			my_pid = getpid();
			
			// Wait for the green light
			sops.sem_op = -1;
			semop(s_id, &sops, 1);
			//qui i figli partono tutti allo stesso momento(vedi il codice del padre dopo)
			// Child process writes NUM_ITER times
            //SEZIONE CRITICA
            sops.sem_num = 1; 
			for(j=0; j<NUM_ITER; j++) {
				// Child process writes at current position, then increment
                sops.sem_op = -1;
                semop(s_id, &sops, 1);
				cur_i = my_data->cur_idx;
				my_data->vec[cur_i].content = (unsigned long)cur_i;
				my_data->vec[cur_i].writer = my_pid;
				my_data->cur_idx++;
                sops.sem_op = 1;
                semop(s_id, &sops, 1);
			}
            //FINE SEZIONE CRITICA
			// done writing, now child process can exit
			// child processes are also detached from the shared memory
			exit(0);
			break;
			
		default:
			break;
		}
	}
	
	// Printed all data, then the shared memory can be marked for
	// deletion. Remember: it will be deleted only when all processes
	// are detached from it
	while (shmctl(m_id, IPC_RMID, NULL)) {
		TEST_ERROR;
	}
	
	sops.sem_op = NUM_PROC; //questo pezzo di codice da il via ai figli
	semop(s_id, &sops, 1);
	
	
	// Waiting for all child processes to terminate
	while ((child_pid = wait(&status)) != -1) {
		dprintf(2,"PID=%d. Sender (PID=%d) terminated with status 0x%04X\n",
			getpid(),
			child_pid,
			status);
	}
	
	// Now the semaphore can be deallocated
	semctl(s_id, 0, IPC_RMID);
	
	// Now print the content of the shared data structure
	shm_print_stats(2, m_id);
	num_errors = 0;
	for (i=0; i<NUM_PROC*NUM_ITER; i++) {
		printf("vec[%6d]: content=%6ld, pid=%d\n",
		       i,
		       (unsigned long)my_data->vec[i].content,
		       my_data->vec[i].writer);
		if ((unsigned long)i != my_data->vec[i].content) {
			num_errors++;
		}
	}
	printf("Number of total errors = %d\n", num_errors);
	
	exit(0);
}


static void shm_print_stats(int fd, int m_id) {
	struct shmid_ds my_m_data;
	int ret_val;
	
	while (ret_val = shmctl(m_id, IPC_STAT, &my_m_data)) {
		TEST_ERROR;
	}
	dprintf(fd, "--- IPC Shared Memory ID: %8d, START ---\n", m_id);
	dprintf(fd, "---------------------- Memory size: %ld\n",
		my_m_data.shm_segsz);
	dprintf(fd, "---------------------- Time of last attach: %ld\n",
		my_m_data.shm_atime);
	dprintf(fd, "---------------------- Time of last detach: %ld\n",
		my_m_data.shm_dtime); 
	dprintf(fd, "---------------------- Time of last change: %ld\n",
		my_m_data.shm_ctime); 
	dprintf(fd, "---------- Number of attached processes: %ld\n",
		my_m_data.shm_nattch);
	dprintf(fd, "----------------------- PID of creator: %d\n",
		my_m_data.shm_cpid);
	dprintf(fd, "----------------------- PID of last shmat/shmdt: %d\n",
		my_m_data.shm_lpid);
	dprintf(fd, "--- IPC Shared Memory ID: %8d, END -----\n", m_id);
}
