#ifndef SDI_COMPILER_H
#define SDI_COMPILER_H

/* Includeheader

        Name:           SDI_compiler.h
        Versionstring:  $VER: SDI_compiler.h 1.18 (04.07.2004)
        Author:         SDI
        Distribution:   PD
        Description:    defines to hide compiler stuff

 1.1   25.06.98 : created from data made by Gunter Nikl
 1.2   17.11.99 : added VBCC
 1.3   29.02.00 : fixed VBCC REG define
 1.4   30.03.00 : fixed SAVEDS for VBCC
 1.5   29.07.00 : added #undef statements (needed e.g. for AmiTCP together
        with vbcc)
 1.6   19.05.01 : added STACKEXT and Dice stuff
 1.7   16.06.01 : added MorphOS specials and VARARGS68K
 1.8   21.09.02 : added MorphOS register stuff
 1.9   26.09.02 : added OFFSET macro. Thanks Frank Wille for suggestion
 1.10  18.10.02 : reverted to old MorphOS-method for GCC
 1.11  09.11.02 : added REGARGS define to MorphOS section
 1.12  18.01.04 : some adaptions for AmigaOS4 compatibility
 1.13  17.02.04 : changed ASM macros to be a simple define and added
                  INTERRUPT, CHIP and FAR
 1.14  02.03.04 : added UNUSED which can be used to specify a function parameter
                  or variable as "unused" which will not cause any compiler warning.
 1.15  02.03.04 : added special INLINE define for gcc > 3.0 version
 1.17  07.03.04 : changed INLINE definition of gcc <= 2.95.3 to be static aswell.
 1.18  21.06.04 : added USED and USED_VAR attribute to allow placing a
                  __attribute__((used)) to a function and a variable, taking care of
                  different compiler versions.
 1.19  04.07.04 : register specification for variables is not supported on MorphOS,
                  so we modified the REG() macro accordingly.
*/

/*
** This is PD (Public Domain). This means you can do with it whatever you want
** without any restrictions. I only ask you to tell me improvements, so I may
** fix the main line of this files as well.
**
** To keep confusion level low: When changing this file, please note it in
** above history list and indicate that the change was not made by myself
** (e.g.�add your name or nick name).
**
** Dirk St�cker <stoecker@epost.de>
*/

#ifdef ASM
#undef ASM
#endif
#ifdef REG
#undef REG
#endif
#ifdef LREG
#undef LREG
#endif
#ifdef CONST
#undef CONST
#endif
#ifdef SAVEDS
#undef SAVEDS
#endif
#ifdef INLINE
#undef INLINE
#endif
#ifdef REGARGS
#undef REGARGS
#endif
#ifdef STDARGS
#undef STDARGS
#endif
#ifdef OFFSET
#undef OFFSET
#endif
#ifdef INTERRUPT
#undef INTERRUPT
#endif
#ifdef CHIP
#undef CHIP
#endif
#ifdef FAR
#undef FAR
#endif
#ifdef UNUSED
#undef UNUSED
#endif
#ifdef USED
#undef USED
#endif
#ifdef USED_VAR
#undef USED_VAR
#endif


  #define UNUSED __attribute__((unused)) /* for functions, variables and types */
  #define USED   __attribute__((used))   /* for functions only! */
  #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 0)
    #define INLINE static __inline __attribute__((always_inline))
    #define USED_VAR __attribute__((used)) /* for variables only! */
  #else
    #define INLINE static __inline__
    #define USED_VAR                       /* no support for used with variables */
  #endif

    #define REG(reg,arg) arg
    #define LREG(reg,arg) register REG(reg,arg)
    #define STDARGS
    #define STACKEXT
    #define REGARGS
    #define SAVEDS
    #define FAR
    #define CHIP
    #define INTERRUPT

/* then "common" ones */

#if !defined(ASM)
  #define ASM
#endif
#if !defined(REG)
  #define REG(reg,arg) register __##reg arg
#endif
#if !defined(LREG)
  #define LREG(reg,arg) register arg
#endif
#if !defined(CONST)
  #define CONST const
#endif
#if !defined(SAVEDS)
  #define SAVEDS __saveds
#endif
#if !defined(INLINE)
  #define INLINE static __inline
#endif
#if !defined(REGARGS)
  #define REGARGS __regargs
#endif
#if !defined(STDARGS)
  #define STDARGS __stdargs
#endif
#if !defined(STACKEXT)
  #define STACKEXT __stackext
#endif
#if !defined(VARARGS68K)
  #define VARARGS68K
#endif
#if !defined(OFFSET)
  #define OFFSET(structName, structEntry) \
    ((char *)(&(((struct structName *)0)->structEntry))-(char *)0)
#endif
#if !defined(INTERRUPT)
  #define INTERRUPT __interrupt
#endif
#if !defined(CHIP)
  #define CHIP __chip
#endif
#if !defined(FAR)
  #define FAR __far
#endif
#if !defined(UNUSED)
  #define UNUSED
#endif
#if !defined(USED)
  #define USED
#endif
#if !defined(USED_VAR)
  #define USED_VAR
#endif
#endif /* SDI_COMPILER_H */
