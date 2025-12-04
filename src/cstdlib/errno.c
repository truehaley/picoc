/*  */
#include <errno.h>

#include "interpreter.h"


#ifdef EACCES
static int EACCESValue = EACCES;
#endif

#ifdef EADDRINUSE
static int EADDRINUSEValue = EADDRINUSE;
#endif

#ifdef EADDRNOTAVAIL
static int EADDRNOTAVAILValue = EADDRNOTAVAIL;
#endif

#ifdef EAFNOSUPPORT
static int EAFNOSUPPORTValue = EAFNOSUPPORT;
#endif

#ifdef EAGAIN
static int EAGAINValue = EAGAIN;
#endif

#ifdef EALREADY
static int EALREADYValue = EALREADY;
#endif

#ifdef EBADF
static int EBADFValue = EBADF;
#endif

#ifdef EBADMSG
static int EBADMSGValue = EBADMSG;
#endif

#ifdef EBUSY
static int EBUSYValue = EBUSY;
#endif

#ifdef ECANCELED
static int ECANCELEDValue = ECANCELED;
#endif

#ifdef ECHILD
static int ECHILDValue = ECHILD;
#endif

#ifdef ECONNABORTED
static int ECONNABORTEDValue = ECONNABORTED;
#endif

#ifdef ECONNREFUSED
static int ECONNREFUSEDValue = ECONNREFUSED;
#endif

#ifdef ECONNRESET
static int ECONNRESETValue = ECONNRESET;
#endif

#ifdef EDEADLK
static int EDEADLKValue = EDEADLK;
#endif

#ifdef EDESTADDRREQ
static int EDESTADDRREQValue = EDESTADDRREQ;
#endif

#ifdef EDOM
static int EDOMValue = EDOM;
#endif

#ifdef EDQUOT
static int EDQUOTValue = EDQUOT;
#endif

#ifdef EEXIST
static int EEXISTValue = EEXIST;
#endif

#ifdef EFAULT
static int EFAULTValue = EFAULT;
#endif

#ifdef EFBIG
static int EFBIGValue = EFBIG;
#endif

#ifdef EHOSTUNREACH
static int EHOSTUNREACHValue = EHOSTUNREACH;
#endif

#ifdef EIDRM
static int EIDRMValue = EIDRM;
#endif

#ifdef EILSEQ
static int EILSEQValue = EILSEQ;
#endif

#ifdef EINPROGRESS
static int EINPROGRESSValue = EINPROGRESS;
#endif

#ifdef EINTR
static int EINTRValue = EINTR;
#endif

#ifdef EINVAL
static int EINVALValue = EINVAL;
#endif

#ifdef EIO
static int EIOValue = EIO;
#endif

#ifdef EISCONN
static int EISCONNValue = EISCONN;
#endif

#ifdef EISDIR
static int EISDIRValue = EISDIR;
#endif

#ifdef ELOOP
static int ELOOPValue = ELOOP;
#endif

#ifdef EMFILE
static int EMFILEValue = EMFILE;
#endif

#ifdef EMLINK
static int EMLINKValue = EMLINK;
#endif

#ifdef EMSGSIZE
static int EMSGSIZEValue = EMSGSIZE;
#endif

#ifdef EMULTIHOP
static int EMULTIHOPValue = EMULTIHOP;
#endif

#ifdef ENAMETOOLONG
static int ENAMETOOLONGValue = ENAMETOOLONG;
#endif

#ifdef ENETDOWN
static int ENETDOWNValue = ENETDOWN;
#endif

#ifdef ENETRESET
static int ENETRESETValue = ENETRESET;
#endif

#ifdef ENETUNREACH
static int ENETUNREACHValue = ENETUNREACH;
#endif

#ifdef ENFILE
static int ENFILEValue = ENFILE;
#endif

#ifdef ENOBUFS
static int ENOBUFSValue = ENOBUFS;
#endif

#ifdef ENODATA
static int ENODATAValue = ENODATA;
#endif

#ifdef ENODEV
static int ENODEVValue = ENODEV;
#endif

#ifdef ENOENT
static int ENOENTValue = ENOENT;
#endif

#ifdef ENOEXEC
static int ENOEXECValue = ENOEXEC;
#endif

#ifdef ENOLCK
static int ENOLCKValue = ENOLCK;
#endif

#ifdef ENOLINK
static int ENOLINKValue = ENOLINK;
#endif

#ifdef ENOMEM
static int ENOMEMValue = ENOMEM;
#endif

#ifdef ENOMSG
static int ENOMSGValue = ENOMSG;
#endif

#ifdef ENOPROTOOPT
static int ENOPROTOOPTValue = ENOPROTOOPT;
#endif

#ifdef ENOSPC
static int ENOSPCValue = ENOSPC;
#endif

#ifdef ENOSR
static int ENOSRValue = ENOSR;
#endif

#ifdef ENOSTR
static int ENOSTRValue = ENOSTR;
#endif

#ifdef ENOSYS
static int ENOSYSValue = ENOSYS;
#endif

#ifdef ENOTCONN
static int ENOTCONNValue = ENOTCONN;
#endif

#ifdef ENOTDIR
static int ENOTDIRValue = ENOTDIR;
#endif

#ifdef ENOTEMPTY
static int ENOTEMPTYValue = ENOTEMPTY;
#endif

#ifdef ENOTRECOVERABLE
static int ENOTRECOVERABLEValue = ENOTRECOVERABLE;
#endif

#ifdef ENOTSOCK
static int ENOTSOCKValue = ENOTSOCK;
#endif

#ifdef ENOTSUP
static int ENOTSUPValue = ENOTSUP;
#endif

#ifdef ENOTTY
static int ENOTTYValue = ENOTTY;
#endif

#ifdef ENXIO
static int ENXIOValue = ENXIO;
#endif

#ifdef EOPNOTSUPP
static int EOPNOTSUPPValue = EOPNOTSUPP;
#endif

#ifdef EOVERFLOW
static int EOVERFLOWValue = EOVERFLOW;
#endif

#ifdef EOWNERDEAD
static int EOWNERDEADValue = EOWNERDEAD;
#endif

#ifdef EPERM
static int EPERMValue = EPERM;
#endif

#ifdef EPIPE
static int EPIPEValue = EPIPE;
#endif

#ifdef EPROTO
static int EPROTOValue = EPROTO;
#endif

#ifdef EPROTONOSUPPORT
static int EPROTONOSUPPORTValue = EPROTONOSUPPORT;
#endif

#ifdef EPROTOTYPE
static int EPROTOTYPEValue = EPROTOTYPE;
#endif

#ifdef ERANGE
static int ERANGEValue = ERANGE;
#endif

#ifdef EROFS
static int EROFSValue = EROFS;
#endif

#ifdef ESPIPE
static int ESPIPEValue = ESPIPE;
#endif

#ifdef ESRCH
static int ESRCHValue = ESRCH;
#endif

#ifdef ESTALE
static int ESTALEValue = ESTALE;
#endif

#ifdef ETIME
static int ETIMEValue = ETIME;
#endif

#ifdef ETIMEDOUT
static int ETIMEDOUTValue = ETIMEDOUT;
#endif

#ifdef ETXTBSY
static int ETXTBSYValue = ETXTBSY;
#endif

#ifdef EWOULDBLOCK
static int EWOULDBLOCKValue = EWOULDBLOCK;
#endif

#ifdef EXDEV
static int EXDEVValue = EXDEV;
#endif


/* creates various system-dependent definitions */
void StdErrnoSetupFunc(Picoc *picoc)
{
    /* defines */
#ifdef EACCES
    VariableDefinePlatformVar(picoc, NULL, "EACCES", &picoc->IntType,
        (AnyValue*)&EACCESValue, false);
#endif

#ifdef EADDRINUSE
    VariableDefinePlatformVar(picoc, NULL, "EADDRINUSE", &picoc->IntType,
        (AnyValue*)&EADDRINUSEValue, false);
#endif

#ifdef EADDRNOTAVAIL
    VariableDefinePlatformVar(picoc, NULL, "EADDRNOTAVAIL", &picoc->IntType,
        (AnyValue*)&EADDRNOTAVAILValue, false);
#endif

#ifdef EAFNOSUPPORT
    VariableDefinePlatformVar(picoc, NULL, "EAFNOSUPPORT", &picoc->IntType,
        (AnyValue*)&EAFNOSUPPORTValue, false);
#endif

#ifdef EAGAIN
    VariableDefinePlatformVar(picoc, NULL, "EAGAIN", &picoc->IntType,
        (AnyValue*)&EAGAINValue, false);
#endif

#ifdef EALREADY
    VariableDefinePlatformVar(picoc, NULL, "EALREADY", &picoc->IntType,
        (AnyValue*)&EALREADYValue, false);
#endif

#ifdef EBADF
    VariableDefinePlatformVar(picoc, NULL, "EBADF", &picoc->IntType,
        (AnyValue*)&EBADFValue, false);
#endif

#ifdef EBADMSG
    VariableDefinePlatformVar(picoc, NULL, "EBADMSG", &picoc->IntType,
        (AnyValue*)&EBADMSGValue, false);
#endif

#ifdef EBUSY
    VariableDefinePlatformVar(picoc, NULL, "EBUSY", &picoc->IntType,
        (AnyValue*)&EBUSYValue, false);
#endif

#ifdef ECANCELED
    VariableDefinePlatformVar(picoc, NULL, "ECANCELED", &picoc->IntType,
        (AnyValue*)&ECANCELEDValue, false);
#endif

#ifdef ECHILD
    VariableDefinePlatformVar(picoc, NULL, "ECHILD", &picoc->IntType,
        (AnyValue*)&ECHILDValue, false);
#endif

#ifdef ECONNABORTED
    VariableDefinePlatformVar(picoc, NULL, "ECONNABORTED", &picoc->IntType,
        (AnyValue*)&ECONNABORTEDValue, false);
#endif

#ifdef ECONNREFUSED
    VariableDefinePlatformVar(picoc, NULL, "ECONNREFUSED", &picoc->IntType,
        (AnyValue*)&ECONNREFUSEDValue, false);
#endif

#ifdef ECONNRESET
    VariableDefinePlatformVar(picoc, NULL, "ECONNRESET", &picoc->IntType,
        (AnyValue*)&ECONNRESETValue, false);
#endif

#ifdef EDEADLK
    VariableDefinePlatformVar(picoc, NULL, "EDEADLK", &picoc->IntType,
        (AnyValue*)&EDEADLKValue, false);
#endif

#ifdef EDESTADDRREQ
    VariableDefinePlatformVar(picoc, NULL, "EDESTADDRREQ", &picoc->IntType,
        (AnyValue*)&EDESTADDRREQValue, false);
#endif

#ifdef EDOM
    VariableDefinePlatformVar(picoc, NULL, "EDOM", &picoc->IntType,
        (AnyValue*)&EDOMValue, false);
#endif

#ifdef EDQUOT
    VariableDefinePlatformVar(picoc, NULL, "EDQUOT", &picoc->IntType,
        (AnyValue*)&EDQUOTValue, false);
#endif

#ifdef EEXIST
    VariableDefinePlatformVar(picoc, NULL, "EEXIST", &picoc->IntType,
        (AnyValue*)&EEXISTValue, false);
#endif

#ifdef EFAULT
    VariableDefinePlatformVar(picoc, NULL, "EFAULT", &picoc->IntType,
        (AnyValue*)&EFAULTValue, false);
#endif

#ifdef EFBIG
    VariableDefinePlatformVar(picoc, NULL, "EFBIG", &picoc->IntType,
        (AnyValue*)&EFBIGValue, false);
#endif

#ifdef EHOSTUNREACH
    VariableDefinePlatformVar(picoc, NULL, "EHOSTUNREACH", &picoc->IntType,
        (AnyValue*)&EHOSTUNREACHValue, false);
#endif

#ifdef EIDRM
    VariableDefinePlatformVar(picoc, NULL, "EIDRM", &picoc->IntType,
        (AnyValue*)&EIDRMValue, false);
#endif

#ifdef EILSEQ
    VariableDefinePlatformVar(picoc, NULL, "EILSEQ", &picoc->IntType,
        (AnyValue*)&EILSEQValue, false);
#endif

#ifdef EINPROGRESS
    VariableDefinePlatformVar(picoc, NULL, "EINPROGRESS", &picoc->IntType,
        (AnyValue*)&EINPROGRESSValue, false);
#endif

#ifdef EINTR
    VariableDefinePlatformVar(picoc, NULL, "EINTR", &picoc->IntType,
        (AnyValue*)&EINTRValue, false);
#endif

#ifdef EINVAL
    VariableDefinePlatformVar(picoc, NULL, "EINVAL", &picoc->IntType,
        (AnyValue*)&EINVALValue, false);
#endif

#ifdef EIO
    VariableDefinePlatformVar(picoc, NULL, "EIO", &picoc->IntType,
        (AnyValue*)&EIOValue, false);
#endif

#ifdef EISCONN
    VariableDefinePlatformVar(picoc, NULL, "EISCONN", &picoc->IntType,
        (AnyValue*)&EISCONNValue, false);
#endif

#ifdef EISDIR
    VariableDefinePlatformVar(picoc, NULL, "EISDIR", &picoc->IntType,
        (AnyValue*)&EISDIRValue, false);
#endif

#ifdef ELOOP
    VariableDefinePlatformVar(picoc, NULL, "ELOOP", &picoc->IntType,
        (AnyValue*)&ELOOPValue, false);
#endif

#ifdef EMFILE
    VariableDefinePlatformVar(picoc, NULL, "EMFILE", &picoc->IntType,
        (AnyValue*)&EMFILEValue, false);
#endif

#ifdef EMLINK
    VariableDefinePlatformVar(picoc, NULL, "EMLINK", &picoc->IntType,
        (AnyValue*)&EMLINKValue, false);
#endif

#ifdef EMSGSIZE
    VariableDefinePlatformVar(picoc, NULL, "EMSGSIZE", &picoc->IntType,
        (AnyValue*)&EMSGSIZEValue, false);
#endif

#ifdef EMULTIHOP
    VariableDefinePlatformVar(picoc, NULL, "EMULTIHOP", &picoc->IntType,
        (AnyValue*)&EMULTIHOPValue, false);
#endif

#ifdef ENAMETOOLONG
    VariableDefinePlatformVar(picoc, NULL, "ENAMETOOLONG", &picoc->IntType,
        (AnyValue*)&ENAMETOOLONGValue, false);
#endif

#ifdef ENETDOWN
    VariableDefinePlatformVar(picoc, NULL, "ENETDOWN", &picoc->IntType,
        (AnyValue*)&ENETDOWNValue, false);
#endif

#ifdef ENETRESET
    VariableDefinePlatformVar(picoc, NULL, "ENETRESET", &picoc->IntType,
        (AnyValue*)&ENETRESETValue, false);
#endif

#ifdef ENETUNREACH
    VariableDefinePlatformVar(picoc, NULL, "ENETUNREACH", &picoc->IntType,
        (AnyValue*)&ENETUNREACHValue, false);
#endif

#ifdef ENFILE
    VariableDefinePlatformVar(picoc, NULL, "ENFILE", &picoc->IntType,
        (AnyValue*)&ENFILEValue, false);
#endif

#ifdef ENOBUFS
    VariableDefinePlatformVar(picoc, NULL, "ENOBUFS", &picoc->IntType,
        (AnyValue*)&ENOBUFSValue, false);
#endif

#ifdef ENODATA
    VariableDefinePlatformVar(picoc, NULL, "ENODATA", &picoc->IntType,
        (AnyValue*)&ENODATAValue, false);
#endif

#ifdef ENODEV
    VariableDefinePlatformVar(picoc, NULL, "ENODEV", &picoc->IntType,
        (AnyValue*)&ENODEVValue, false);
#endif

#ifdef ENOENT
    VariableDefinePlatformVar(picoc, NULL, "ENOENT", &picoc->IntType,
        (AnyValue*)&ENOENTValue, false);
#endif

#ifdef ENOEXEC
    VariableDefinePlatformVar(picoc, NULL, "ENOEXEC", &picoc->IntType,
        (AnyValue*)&ENOEXECValue, false);
#endif

#ifdef ENOLCK
    VariableDefinePlatformVar(picoc, NULL, "ENOLCK", &picoc->IntType,
        (AnyValue*)&ENOLCKValue, false);
#endif

#ifdef ENOLINK
    VariableDefinePlatformVar(picoc, NULL, "ENOLINK", &picoc->IntType,
        (AnyValue*)&ENOLINKValue, false);
#endif

#ifdef ENOMEM
    VariableDefinePlatformVar(picoc, NULL, "ENOMEM", &picoc->IntType,
        (AnyValue*)&ENOMEMValue, false);
#endif

#ifdef ENOMSG
    VariableDefinePlatformVar(picoc, NULL, "ENOMSG", &picoc->IntType,
        (AnyValue*)&ENOMSGValue, false);
#endif

#ifdef ENOPROTOOPT
    VariableDefinePlatformVar(picoc, NULL, "ENOPROTOOPT", &picoc->IntType,
        (AnyValue*)&ENOPROTOOPTValue, false);
#endif

#ifdef ENOSPC
    VariableDefinePlatformVar(picoc, NULL, "ENOSPC", &picoc->IntType,
        (AnyValue*)&ENOSPCValue, false);
#endif

#ifdef ENOSR
    VariableDefinePlatformVar(picoc, NULL, "ENOSR", &picoc->IntType,
        (AnyValue*)&ENOSRValue, false);
#endif

#ifdef ENOSTR
    VariableDefinePlatformVar(picoc, NULL, "ENOSTR", &picoc->IntType,
        (AnyValue*)&ENOSTRValue, false);
#endif

#ifdef ENOSYS
    VariableDefinePlatformVar(picoc, NULL, "ENOSYS", &picoc->IntType,
        (AnyValue*)&ENOSYSValue, false);
#endif

#ifdef ENOTCONN
    VariableDefinePlatformVar(picoc, NULL, "ENOTCONN", &picoc->IntType,
        (AnyValue*)&ENOTCONNValue, false);
#endif

#ifdef ENOTDIR
    VariableDefinePlatformVar(picoc, NULL, "ENOTDIR", &picoc->IntType,
        (AnyValue*)&ENOTDIRValue, false);
#endif

#ifdef ENOTEMPTY
    VariableDefinePlatformVar(picoc, NULL, "ENOTEMPTY", &picoc->IntType,
        (AnyValue*)&ENOTEMPTYValue, false);
#endif

#ifdef ENOTRECOVERABLE
    VariableDefinePlatformVar(picoc, NULL, "ENOTRECOVERABLE", &picoc->IntType,
        (AnyValue*)&ENOTRECOVERABLEValue, false);
#endif

#ifdef ENOTSOCK
    VariableDefinePlatformVar(picoc, NULL, "ENOTSOCK", &picoc->IntType,
        (AnyValue*)&ENOTSOCKValue, false);
#endif

#ifdef ENOTSUP
    VariableDefinePlatformVar(picoc, NULL, "ENOTSUP", &picoc->IntType,
        (AnyValue*)&ENOTSUPValue, false);
#endif

#ifdef ENOTTY
    VariableDefinePlatformVar(picoc, NULL, "ENOTTY", &picoc->IntType,
        (AnyValue*)&ENOTTYValue, false);
#endif

#ifdef ENXIO
    VariableDefinePlatformVar(picoc, NULL, "ENXIO", &picoc->IntType,
        (AnyValue*)&ENXIOValue, false);
#endif

#ifdef EOPNOTSUPP
    VariableDefinePlatformVar(picoc, NULL, "EOPNOTSUPP", &picoc->IntType,
        (AnyValue*)&EOPNOTSUPPValue, false);
#endif

#ifdef EOVERFLOW
    VariableDefinePlatformVar(picoc, NULL, "EOVERFLOW", &picoc->IntType,
        (AnyValue*)&EOVERFLOWValue, false);
#endif

#ifdef EOWNERDEAD
    VariableDefinePlatformVar(picoc, NULL, "EOWNERDEAD", &picoc->IntType,
        (AnyValue*)&EOWNERDEADValue, false);
#endif

#ifdef EPERM
    VariableDefinePlatformVar(picoc, NULL, "EPERM", &picoc->IntType,
        (AnyValue*)&EPERMValue, false);
#endif

#ifdef EPIPE
    VariableDefinePlatformVar(picoc, NULL, "EPIPE", &picoc->IntType,
        (AnyValue*)&EPIPEValue, false);
#endif

#ifdef EPROTO
    VariableDefinePlatformVar(picoc, NULL, "EPROTO", &picoc->IntType,
        (AnyValue*)&EPROTOValue, false);
#endif

#ifdef EPROTONOSUPPORT
    VariableDefinePlatformVar(picoc, NULL, "EPROTONOSUPPORT", &picoc->IntType,
        (AnyValue*)&EPROTONOSUPPORTValue, false);
#endif

#ifdef EPROTOTYPE
    VariableDefinePlatformVar(picoc, NULL, "EPROTOTYPE", &picoc->IntType,
        (AnyValue*)&EPROTOTYPEValue, false);
#endif

#ifdef ERANGE
    VariableDefinePlatformVar(picoc, NULL, "ERANGE", &picoc->IntType,
        (AnyValue*)&ERANGEValue, false);
#endif

#ifdef EROFS
    VariableDefinePlatformVar(picoc, NULL, "EROFS", &picoc->IntType,
        (AnyValue*)&EROFSValue, false);
#endif

#ifdef ESPIPE
    VariableDefinePlatformVar(picoc, NULL, "ESPIPE", &picoc->IntType,
        (AnyValue*)&ESPIPEValue, false);
#endif

#ifdef ESRCH
    VariableDefinePlatformVar(picoc, NULL, "ESRCH", &picoc->IntType,
        (AnyValue*)&ESRCHValue, false);
#endif

#ifdef ESTALE
    VariableDefinePlatformVar(picoc, NULL, "ESTALE", &picoc->IntType,
        (AnyValue*)&ESTALEValue, false);
#endif

#ifdef ETIME
    VariableDefinePlatformVar(picoc, NULL, "ETIME", &picoc->IntType,
        (AnyValue*)&ETIMEValue, false);
#endif

#ifdef ETIMEDOUT
    VariableDefinePlatformVar(picoc, NULL, "ETIMEDOUT", &picoc->IntType,
        (AnyValue*)&ETIMEDOUTValue, false);
#endif

#ifdef ETXTBSY
    VariableDefinePlatformVar(picoc, NULL, "ETXTBSY", &picoc->IntType,
        (AnyValue*)&ETXTBSYValue, false);
#endif

#ifdef EWOULDBLOCK
    VariableDefinePlatformVar(picoc, NULL, "EWOULDBLOCK", &picoc->IntType,
        (AnyValue*)&EWOULDBLOCKValue, false);
#endif

#ifdef EXDEV
    VariableDefinePlatformVar(picoc, NULL, "EXDEV", &picoc->IntType,
        (AnyValue*)&EXDEVValue, false);
#endif

    VariableDefinePlatformVar(picoc, NULL, "errno", &picoc->IntType,
        (AnyValue*)&errno, true);
}
