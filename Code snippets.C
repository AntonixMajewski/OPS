

// Creating robust mutex


pthread_mutexattr_t mutex_attr;
pthread_mutexattr_init(&mutex_attr);
pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
pthread_mutexattr_setrobust(&mutex_attr, PTHREAD_MUTEX_ROBUST);
pthread_mutex_init(mutex, &mutex_attr);



pthread_mutexattr_destroy(&attr);


// Creating shared semaphore


sem_t *shared_semaphore = sem_open("/my_named_semaphore", O_CREAT | O_RDWR, 0666, 1);
if (shared_semaphore == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
}

void *addr = mmap(NULL, sizeof(sem_t *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
if (addr == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
}

*(sem_t **)addr = shared_semaphore;
sem_t *sem = *(sem_t **)addr;
sem_wait(sem);
// Actions
sem_post(sem);
sem_close(shared_semaphore); 
sem_unlink("/my_named_semaphore"); 
munmap(addr, sizeof(sem_t *)); 


// Creating shared memory


shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
if (shm_fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
}

if (ftruncate(shm_fd, SHM_SIZE) == -1) {
    perror("ftruncate");
    exit(EXIT_FAILURE);
}

// Actions

if (close(shm_fd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
}

if (shm_unlink(SHM_NAME) == -1) {
    perror("shm_unlink");
    exit(EXIT_FAILURE);
}


// Mapping shared object into shared memory
// If updating the file, remember to msync


shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
}

// Actions

if (munmap(shm_ptr, SHM_SIZE) == -1) {
    perror("munmap");
    exit(EXIT_FAILURE);
}


// Create childen


void create_children(int n, float* data, char* log)
{
    while (n-- > 0)
    {
        switch (fork())
        {
            case 0:
                child_work(n, data, log);
                exit(EXIT_SUCCESS);
            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);
        }
    }
}


// Random child work


void child_work(int n, float* out, char* log)
{
    int sample = 0;
    srand(getpid());
    int iters = MONTE_CARLO_ITERS;
    while (iters-- > 0)
    {
        double x = ((double)rand()) / RAND_MAX, y = ((double)rand()) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            sample++;
    }
    out[n] = ((float)sample) / MONTE_CARLO_ITERS;
    char buf[LOG_LEN + 1];

    snprintf(buf, LOG_LEN + 1, "%7.5f\n", out[n] * 4.0f);
    memcpy(log + n * LOG_LEN, buf, LOG_LEN);
}


