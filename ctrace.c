#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <err.h>
#include <machine/reg.h>

#ifdef __i386__
# define RIP r_eip
# define RAX r_eax
#else
# define RIP r_rip
# define RAX r_rax
# define RBP r_rbp
# define RSP r_rsp
#endif

int main ()
{
  FILE *beamf = popen ("pgrep beam", "r");
  pid_t beampid;
  if (fscanf (beamf, "%d", &beampid) != 1)
    errx (1, "no beam process");
  pclose (beamf);

  //struct sigaction term_sa = { .sa_flags = SA_SIGINFO };

  if (ptrace (PT_ATTACH, beampid, 0, 0) == -1)
    err (1, "pt_attach");
  
  waitpid (beampid, 0, 0);

  lwpid_t lwplist[64];
  int n_lwp = ptrace (PT_GETLWPLIST, beampid,
		      (caddr_t) lwplist, sizeof lwplist / sizeof lwplist[0]);


  lwpid_t tid = lwplist[n_lwp - 1];

  for (int i = 0; i < n_lwp; i++)
    {
      struct reg regs;
      ptrace (PT_GETREGS, lwplist[i], (caddr_t) &regs, 0);
      printf ("+ %d ip=%lx\n", lwplist[i], regs.RIP);
    }
  printf ("\n");

  if (ptrace (PT_CONTINUE, tid, 0, 0) == -1)
    warn ("pt_cont");

  waitpid (beampid, 0, 0);

  for (int step = 0; step < 10; step++)
  //for (;;)
    {
      struct ptrace_lwpinfo lwp;
      ptrace (PT_LWPINFO, beampid, (caddr_t) &lwp, sizeof lwp);

      struct reg regs;
      if (ptrace (PT_GETREGS, beampid, (caddr_t) &regs, 0) == -1)
	warn ("pt_getregs");

      long rip = regs.RIP;
      //if (rip >= 0x00000000005f54d0 && rip < 0x0000000000606930)
      if (true)
	{
	  printf ("tid=%d bp/sp=%p/%p rip=%p rax=%p\n",
		  lwp.pl_lwpid,
		  regs.RBP, regs.RSP,
		  rip, regs.RAX);
	}

	usleep (1000000);

      /* if (ptrace (PT_STEP, tid, (caddr_t) 1, 0) == -1) */
      /* 	warn ("stepping"); */

      /* waitpid (beampid, 0, 0); */
    }

  fprintf (stderr, "done.\n");


  if (ptrace (PT_DETACH, beampid, 0, 0) == -1)
    warn ("pt_detach");

  fprintf (stderr, "detached.\n");
}

