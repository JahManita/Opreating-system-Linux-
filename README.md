# Opreating-system-Linux
Project#1

A producer-consumer problem requiring a single producer (of donuts) process, and an arbitrary number of consumer processes. The producer must: create a shared memory segment with 4 ring buffers and their required control variables, one for each of four kinds of donuts.

Created semaphores for each ring buffer to provide mutually exclusive access to each buffer after creating and mapping the segment.

- Result 
  - Provided the probability of deadlock VS queue depth graph which is a linear graph.
  
  - Observe that the higher queue depth, the lower chance of getting deadlock.
  
  - My 50% deadlock queue size is 94 which is ranged from the 90-95 queue size data. I used the 94th queue size depth to generate the probability of deadlock VS number   of consumers graph
  
  - Obeserved the higher the number of consumers, the higher chance of deadlock because there is only one producer.


Project#2

A producer-consumer problem requiring a single process solution using threads within a process ( the pthread interfaces are suggested and available on all of our Linux systems). Unlike the project#1, multiple producers (of donuts) must be created as threads along with some number of consumer threads.

- Result
  - Provided the probability of deadlock vs queue depth graph which is close to linear. 
  
  - Observed that the higher the queue depth, the lower the chance of getting a deadlock. This experiment was similar to project#1.
  
  - My 50% deadlock queue size is 5100. I used the 5100th queue size depth when collecting data from each dozen donuts. I ran with 5 configurations which are 5000, 
  10000, 20000, 25000 and 30000 dozens. 
  
  - Observed the higher the number of donut dozens, the higher chance of getting deadlock.  
