/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 IBM Corp.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "pty.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>

int openpty(int *amaster, int *aslave, char *name, const struct termios *termp, const struct winsize *winp)
{
    const char* tty_path;
    int master = -1;
    int slave = -1;
    int save_errno;
    
    if(!aslave || !amaster)
    {
        errno = ENOENT;
        return -1;
    }

    if ((master = open("/dev/ptc", O_RDWR | O_NOCTTY)) == -1) goto error;
    *amaster = master;
    
    if ((tty_path = ttyname(master)) == NULL) goto error;
    
    if ((slave = open(tty_path, O_RDWR | O_NOCTTY)) == -1) goto error;
    *aslave = slave;
    
    if ((tty_path = ttyname(slave)) == NULL) goto error;
    
    /* Not supported in PASE */
    /* if(revoke(tty_path) == -1) goto error; */
    
    if(name != NULL)
    {
        strcpy(name, tty_path);
    }
    
    if(termp != NULL && tcsetattr(master, TCSANOW, termp) == -1) goto error;
    
    if(winp != NULL && ioctl(master, TIOCSWINSZ, winp) == -1) goto error;
    
    return 0;
    
error:
    save_errno = errno;
    
    if(master >= 0) close(master);
    if(slave >= 0) close(slave);
    
    errno = save_errno;
    return -1;
}

pid_t forkpty(int *amaster, char *name, const struct termios *termp, const struct winsize *winp)
{
    pid_t pid;
    int master = -1;
    int slave = -1;
    int save_errno;
    
    if(!amaster)
    {
        errno = ENOENT;
        return -1;
    }
    
    if(openpty(&master, &slave, name, termp, winp) < 0)
    {
      return (pid_t) -1;
    }
    
    pid = fork();
    if(pid < 0)
    {
        save_errno = errno;
        
        close(master);
        close(slave);
        
        errno = save_errno;
    }
    else if(pid == 0)
    {
        close(master);
        
        // login_tty returns 0 or -1, which is
        // conveniently the same return codes we need
        pid = (pid_t) login_tty(slave);
    }
    else
    {
        close(slave);
        *amaster = master;
    }
    
    return pid;
}

int login_tty(int fd)
{
    const char* tty_path;
    int slave;
    
    // make sure fd is connected to a TTY
    if(isatty(fd) == -1) return -1;
    
    // check fd was opened R/W
    if(fcntl(fd, F_GETFL) & O_RDWR != O_RDWR) return -1;

    // allocate a new session
    if(setsid() == -1) return -1;
    
    if ((tty_path = ttyname(fd)) == NULL) return -1;
    
    // NOTE: AIX does not have an IOCTL to set the controlling terminal.
    // When you open a TTY without a controlling terminal *and* you're
    // the session leader, the opened fd will automatically be set as
    // your controlling terminal, unless you pass O_NOCTTY to open.
    //
    // Here, we re-open the TTY using the path returned by ttyname and
    // make sure that O_NOCTTY is not set so that it becomes our
    // controlling terminal.
    if ((slave = open(tty_path, O_RDWR)) == -1) return -1;
    
    // Now that we've opened a new FD to the TTY, close the original
    close(fd);

    // point stdin, stdout, stderr at the terminal fd
    if(dup2(slave, 0) == -1) return -1;
    if(dup2(slave, 1) == -1) return -1;
    if(dup2(slave, 2) == -1) return -1;
    
    // close the terminal fd
    close(slave);
    
    return 0;
}
