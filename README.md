# UnixNoReadUpNoWriteDown
This project is a simple Kerrnel Module that adds [Bell–LaPadula security model](https://en.wikipedia.org/wiki/Bell%E2%80%93LaPadula_model) functionality to the linux filesystem. In this model users & files can have a security level of `0-3`, users can write to files with higher or equal security levels than their own security clearance level, and are able to read information off of files with lower or equal security levels.

This kernel module overrides the `open` syscall upon insertion to the kernel space by changing the function specificied in the syscall table. The new function checks if the accessed file or the currecnt user is present in its database and gets their corresponding security levels (defualt security level of all users & files are assumed to be `0`); Then, the original `open` is called accordingly or `-1` is returned to prevent unwanted requests.

To provide the kernel module with the necessary lists of users and files, a simple device driver and its user-level counterpart python script are implemented. Both of these lists are then held in the kernel memory space using custom linked lists; `files` is a linked list of files absolute paths and their corresponding security level, similarly `users` is a linked list containing pairs of user IDs and their security clearance levels.

In addition, whenever a file present in the `files` list is accessed, access information is logged into `/tmp/phase2log`.

## Requirements
* Ubuntu (version <= 16.04)

## Todo
* Not all file opening procedures pass through `open` syscall, both `openat` and `openat2` syscalls should be overrided as well.
* Linux Kernel has a generic implementation of common data structures such as linked lists, `files` and `users` linked lists are to be refactored to use the linux's linked lists instead.
* Filenames passed to `open` syscall may be of a relative path, a uitility function should be implemented to convert all paths to be absolute.
* Synchronization tools should be applied to both `files` and `users` linked lists to avoid possible race conditions.
* It is almost a taboo to access a file directly from kernel modules, access logs should rather be implemented by writing to a [/proc](https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x773.html) file.
