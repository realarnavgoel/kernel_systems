# Requirements for kdriver
## Write a character device driver to support the following device file operations
### Must-have features
- `open` a file per user-defined context
  - Start a kthread that writes every 1usec RNG counter into user-defined context
  - Start a ktrhead that monitors every 1usec well-defined offset(s) for a value of 1
    - Invokes a callback that printk to kern.log, when the control value changes
- `close` a previously opened file per user-defined context
  - Stop a kthread that previously simulates writes of counter
  - Stop a kthread that previously simulates reads of control
- `mmap` per user-defined context's physical page into virtual memory
- `ioctl` to change the offset for monitoring to user-defined value and return the old offset
- `ioctl` to change the counter value for writing to user-defined value and return the old counter 

### Nice-to-have features 
- `read` or `write` a file a user-defined (offset, size) and return data into a user-defined buffer
