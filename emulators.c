#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "vm.h"
#include "skvm.h"

/* Read sectors of the guest's disk 
 * buffer - HVA of the buffer
 * sector - starting sector (1st sector is 0)
 * count  - how many sectors 
 */
ssize_t disk_read (struct vm *guest, char *buffer, size_t sector,
                   size_t count)
{
    if (lseek (guest->disk_fd, (off_t) sector * 512, SEEK_SET) < 0)
        return -1;

    return read (guest->disk_fd, buffer, count * 512);
}

void video_out (char c)
{
    putchar (c);
}

uint8_t console_in (void)
{
    /* nop */
    return 0;
}
