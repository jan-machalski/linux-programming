# Linux System Utilities (C Programs)

This repository contains a collection of **independent C programs** designed for various **Linux system operations**. Each program utilizes system calls, process management, inter-process communication, and synchronization mechanisms.

---

## Contents

### **1. Multiplayer Number Game (`multiplayer_number_game.c`)**
- **Description:** A multiplayer game where players **select numbers** in each round, and the highest number earns points.
- **Key Features:**
  - Uses `fork()` to create multiple child processes (players).
  - Parent process manages **rounds, scores, and communication**.
  - Implements **pipes** (`pipe()`) for bidirectional communication between processes.
  - Handles `SIGCHLD` signal for child process cleanup.
- **Compilation & Execution:**
  ```bash
  gcc -o multiplayer_number_game multiplayer_number_game.c -Wall -Wextra -std=c11
  ./multiplayer_number_game <num_players> <num_rounds>
  ```

---

### **2. Bridge Card Game (`bridge_game.c`)**
- **Description:** A simplified version of the **Bridge card game**, using **multithreading** and synchronization.
- **Key Features:**
  - Uses `pthread_create()` for **simulating players**.
  - Synchronizes threads with **pthread barriers**.
  - Implements **randomized card passing** between players.
  - Properly handles **thread cleanup and game termination**.
- **Compilation & Execution:**
  ```bash
  gcc -o bridge_game bridge_game.c -Wall -Wextra -pthread -std=c11
  ./bridge_game <num_players>
  ```

---

### **3. IPC Message Queue Calculator (`ipc_calculator/`)**
- **Description:** A client-server application that performs **integer arithmetic operations** via **POSIX message queues**.
- **Components:**
  - **`server.c`**: Manages multiple calculation requests via message queues.
  - **`client.c`**: Sends arithmetic requests to the server and receives results.
- **Key Features:**
  - Uses **POSIX message queues (`mqueue.h`)** for IPC.
  - Server supports multiple operations (`addition`, `division`, `modulo`).
  - Implements **signal handling** for graceful shutdown (`SIGINT`).
  - Clients communicate with the server dynamically via **unique message queues**.
- **Compilation & Execution:**
  ```bash
  # Compile server and client
  gcc -o ipc_server ipc_calculator/server.c -Wall -Wextra -lrt -std=c11
  gcc -o ipc_client ipc_calculator/client.c -Wall -Wextra -lrt -std=c11

  # Start the server
  ./ipc_server

  # Run the client with the server queue name
  ./ipc_client <server_queue_name>
  ```

---

### **4. Uber-like Ride Simulation (`uber_simulation.c`)**
- **Description:** A **ride-hailing simulation**, where a **dispatcher** assigns ride requests to **drivers** using POSIX message queues.
- **Key Features:**
  - Uses **message queues** (`mq_send()`, `mq_receive()`) for communication.
  - Parent process acts as a **ride dispatcher**, creating ride requests.
  - Child processes act as **drivers**, completing assigned rides.
  - Drivers update the dispatcher with ride completion via **message queues**.
  - Implements **signal handling** (`SIGALRM`, `SIGINT`) for terminating the simulation.
- **Compilation & Execution:**
  ```bash
  gcc -o uber_simulation uber_simulation.c -Wall -Wextra -lrt -std=c11
  ./uber_simulation <num_drivers> <simulation_time>
  ```

---

## Features
- Efficient use of **system calls** (`fork()`, `pipe()`, `waitpid()`, `mq_open()`).
- Demonstrates **process synchronization** (`SIGCHLD`, `SIGPIPE` handling, message queues).
- Implements **multithreading with pthreads** (`pthread_create()`, `pthread_barrier_wait()`).
- Uses **dynamic memory allocation** safely (`malloc()`, `free()`).

---
