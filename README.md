# Character Device Driver (`kdriver`)
## Supported features
- Support primitive `struct file_operations` such as `open, close, mmap, ioctl`
- Support `struct device` based file interface `/dev/kdriver` for user-mode driver
- Support read-only access to simulate kernel device resource (e.g. counters)
- Support read-write access to simulate kernel device resource (e.g. monitor)
- Support user-mode driver driven example app for the aforementioned features 
