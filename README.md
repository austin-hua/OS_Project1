# OSProject1
Project One - [Group 11] 2019 台大作業系統

b06902101	b06902098	b05902099	b06902051	b05902132	b06902086

Problem description: http://rswiki.csie.org/dokuwiki/courses:107_2:project_1

Kernel files can be compiled along with kernel v5.0.3 - v5.0.5.  
For other versions, use kernel/proj1.c directly, but other files may need to be merged manually.

It's easy to compile the kernel, but the installation process may differ by your distribution.  
A good instruction for Archlinux is at: https://wiki.archlinux.org/index.php/Kernel/Traditional_compilation 
For Ubuntu 16, see the link at details section here: http://rswiki.csie.org/dokuwiki/courses:104_2:homework_1


TODO
  Add extremely accurate system call for child process start and finish (nanosecond accuracy)
    ->This can be done with <linux/ktime.h> header
    ->https://www.fsl.cs.sunysb.edu/kernel-api/re28.html
  
  Add input reading to main function... pass input to functions...
  Shally & Annjee: Add RR & FIFO
  Naveno & Gearlad: Add STF and PSTF
