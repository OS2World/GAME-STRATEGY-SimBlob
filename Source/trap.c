/* Copyright 1994 MetaWare Incorporated.  All rights reserved.
 *
 * $Id: trap.c,v 1.3 1994/08/26 23:16:47 marka Exp $
 *
 * $Log: trap.c,v $
 * Revision 1.3  1994/08/26  23:16:47  marka
 * Change copyright notice.
 *
 * Revision 1.2  1994/08/26  01:08:29  marka
 * Print the registers only if the HC_DUMPREG environment variable
 * is defined.
 *
 * Revision 1.1  1994/07/16  05:50:43  marka
 * libinstall on file owned by: marka
 *
 */

#define INCL_NOPMAPI
#define INCL_DOSEXCEPTIONS
#define INCL_DOSPROCESS
#define INCL_DOSFILEMGR
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#include <os2.h>

#include <stdio.h>
#include <string.h>

void myprintf(const char *format, ...) {
    va_list ap;
    static char buf[128];
    ULONG written;

    /* Write the stuff into a buffer.
     */
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    /* Write the buffer to stdout.
     */
    DosWrite(1, buf, strlen(buf), &written);
    }


void print_context(PCONTEXTRECORD cont) {
    PTIB   ptib;
    PPIB   ppib;
    APIRET rc;

    if (cont->ContextFlags & CONTEXT_CONTROL) {
	myprintf("EIP : %8.8lX ",cont->ctx_RegEip  );
	myprintf("EBP : %8.8lX ",cont->ctx_RegEbp  );
	myprintf("ESP : %8.8lX ",cont->ctx_RegEsp  );
	myprintf("EFLG: %8.8lX\r\n",cont->ctx_EFlags  );
	}

    if (cont->ContextFlags & CONTEXT_INTEGER) {
	myprintf("EAX : %8.8lX ",cont->ctx_RegEax  );
	myprintf("EBX : %8.8lX ",cont->ctx_RegEbx  );
	myprintf("ECX : %8.8lX ",cont->ctx_RegEcx  );
	myprintf("EDX : %8.8lX\r\n",cont->ctx_RegEdx  );
	myprintf("EDI : %8.8lX ",cont->ctx_RegEdi  );
	myprintf("ESI : %8.8lX\r\n",cont->ctx_RegEsi  );
	}

    if (cont->ContextFlags & CONTEXT_CONTROL) {
	myprintf("CS  : %4.4lX     ",cont->ctx_SegCs   );
	myprintf("SS  : %4.4lX\r\n",cont->ctx_SegSs   );
	}

    if (cont->ContextFlags & CONTEXT_SEGMENTS) {
	myprintf("GS  : %4.4lX     ",cont->ctx_SegGs);
	myprintf("FS  : %4.4lX     ",cont->ctx_SegFs);
	myprintf("ES  : %4.4lX     ",cont->ctx_SegEs);
	myprintf("DS  : %4.4lX     \r\n",cont->ctx_SegDs);
	}

    if (cont->ContextFlags & CONTEXT_CONTROL) {
	if (DosGetInfoBlocks(&ptib, &ppib) == 0) {
	    static CHAR modname[CCHMAXPATH];
	    myprintf("Process id: %lu, ", ppib->pib_ulpid);
	    rc = DosQueryModuleName(ppib->pib_hmte,CCHMAXPATH, modname);
	    myprintf(".EXE name: %s\r\n", rc == 0 ? modname : "?????");
	    myprintf("Thread id: %lu,  ordinal : %lu, priority : %p\r\n",
			ptib->tib_ptib2->tib2_ultid ,
			ptib->tib_ordinal,
			ptib->tib_ptib2->tib2_ulpri );
	    myprintf("Stack bottom: %8.8lX, ", ptib->tib_pstack);
	    myprintf("top: %8.8lX \r\n",ptib->tib_pstacklimit);

	    }
	}
    }

ULONG APIENTRY _mwtrap_handler(
	PEXCEPTIONREPORTRECORD rep,
        PEXCEPTIONREGISTRATIONRECORD reg,
        PCONTEXTRECORD cont,
        PVOID reserved)
    {
    ULONG  nest;
    PCSZ    env_ptr;

    if ((rep->ExceptionNum & XCPT_SEVERITY_CODE) != XCPT_FATAL_EXCEPTION) {
	// Not a fatal exception; system should continue search
	return XCPT_CONTINUE_SEARCH;
	}

    DosEnterMustComplete(&nest);

    // Acknowledge signal exceptions

    // Print exception information

    if ((rep->ExceptionNum != XCPT_PROCESS_TERMINATE) &&
	     (rep->ExceptionNum != XCPT_UNWIND) &&
	     (rep->ExceptionNum != XCPT_ASYNC_PROCESS_TERMINATE)) {

	myprintf("\007\r\nFatal exception %8.8lX: ",rep->ExceptionNum);

	switch (rep->ExceptionNum) {
	    case XCPT_ACCESS_VIOLATION:
		myprintf("access violation: ");
		switch (rep->ExceptionInfo[0]) {
		    case XCPT_READ_ACCESS:
		    case XCPT_WRITE_ACCESS:
			myprintf("invalid linear address %8.8lX",
				rep->ExceptionInfo[1]);
			break;
		    case XCPT_SPACE_ACCESS:
			myprintf("invalid selector %8.8lX",rep->ExceptionInfo[1]);
			break;
		    case XCPT_LIMIT_ACCESS:
			myprintf("limit access fault");
			break;
		    case XCPT_UNKNOWN_ACCESS:
			myprintf("unknown access fault");
			break;
		    default:
			myprintf("other unknown access fault");
		    }
		break;
	    case XCPT_INTEGER_DIVIDE_BY_ZERO:
		myprintf("integer divide by zero");
		break;
	    case XCPT_INTEGER_OVERFLOW:
		myprintf("integer overflow\r\n");
		break;
	    case XCPT_ILLEGAL_INSTRUCTION:
		myprintf("illegal instruction");
		break;
	    case XCPT_FLOAT_DENORMAL_OPERAND:
		myprintf("float denormal operand");
		break;
	    case XCPT_FLOAT_DIVIDE_BY_ZERO:
		myprintf("float divide by zero");
		break;
	    case XCPT_FLOAT_INEXACT_RESULT:
		myprintf("float inexact result");
		break;
	    case XCPT_FLOAT_INVALID_OPERATION:
		myprintf("float invalid operation");
		break;
	    case XCPT_FLOAT_OVERFLOW:
		myprintf("float overflow");
		break;
	    case XCPT_FLOAT_STACK_CHECK:
		myprintf("float stack check");
		break;
	    case XCPT_FLOAT_UNDERFLOW:
		myprintf("float underflow");
		break;
	    case XCPT_PRIVILEGED_INSTRUCTION:
		myprintf("privileged instruction");
		break;
	    case XCPT_SIGNAL:
		switch (rep->ExceptionInfo[0]) {
		    case XCPT_SIGNAL_BREAK:
		        myprintf("Control-Break");
			DosAcknowledgeSignalException(XCPT_SIGNAL_BREAK);
			break;
		    case XCPT_SIGNAL_INTR:
			myprintf("Control-C");
			DosAcknowledgeSignalException(XCPT_SIGNAL_INTR);
			break;
		    case XCPT_SIGNAL_KILLPROC:
			myprintf("DosKillProcess");
			break;
		    }
		break;
	    }

	myprintf(".\r\n");

	/* If HC_DUMPREG environment variable is defined, dump
	 * the register contents.
	 */
	if (DosScanEnv("HC_DUMPREG", &env_ptr) == 0)
	    print_context(cont);

	myprintf("Aborting.\r\n");
	}

    DosExitMustComplete(&nest);
    DosUnsetExceptionHandler(reg);
    DosExit( EXIT_THREAD, 0 );
    return 0;

    }
