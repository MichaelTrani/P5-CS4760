P4 Operating System Simulator

This project has run into some issues.
It uses queues from the queue library to store processes by their PID.
The simulator assigns a fake PID for each process to run.
It iteraties by priority and sends messages, including quantum /time slice by means of shmem struct. etc.

I ran into some issues. The main one being that my shared memory stops working mid-run.
I thought it was because the processes were ending too early but after running through GDB I've found that's not the case.
This may be related, but my memory keys are appearing as 0x00000000. Research indicated that this was due to using private keys.
I changed my makefile to not use the private key library, pthread, but the problem persists.
The odd thing was that this was not always the case. It worked fine at first and even after reverting to previous builds it won't delete shared memory.
I have left output statements to show where each program binds up.
It was one of those problems where I always felt like I was at the edge of finding the answer but never quite got to it.
Previously it would run several children processes and fail during one of their runs. 
Now it will only run one child.

-Michael Trani