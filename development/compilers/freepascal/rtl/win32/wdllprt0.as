//DLL Startup code for WIN32 port of FPK-Pascal 0.9.98
//Written by P.Ozerski
//16.10.1998
     .text
     .globl _mainCRTStartup
_mainCRTStartup:
     movb $1,U_SYSWIN32_ISCONSOLE
     jmp .LDLL_Entry
     .globl _WinMainCRTStartup
_WinMainCRTStartup:
     movb $0,U_SYSWIN32_ISCONSOLE
.LDLL_Entry:
     pushl    %ebp
     movl     %esp,%ebp
     pushl    %ebx
     pushl    %esi
     pushl    %edi
     movl     8(%ebp),%edi
     movl     %edi,U_SYSWIN32_HINSTANCE
     movl     12(%ebp),%edi
     movl     %edi,U_SYSWIN32_DLLREASON
     movl     16(%ebp),%edi
     movl     %edi,U_SYSWIN32_DLLPARAM
     call     _FPC_DLL_Entry
     popl     %edi
     popl     %esi
     popl     %ebx
     popl     %ebp
     ret      $12


