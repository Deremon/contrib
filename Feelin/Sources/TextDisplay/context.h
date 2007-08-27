/*

    TextDisplay/context.h

*/

#ifndef FEELIN_TEXTDISPLAY_CONTEXT_H
#define FEELIN_TEXTDISPLAY_CONTEXT_H

#include "parse.h"

///FTDContextContainer

struct in_FeelinTD_Context_Container
{
    struct in_FeelinTD_Context_Container  *Next;
    struct in_FeelinTD_Context_Container  *Prev;

    UWORD                           Type;
    BYTE                            Stolen;
    BYTE                            _pad0;
    APTR                            Data;
};

typedef struct in_FeelinTD_Context_Container       FTDContextContainer;

/*  (FTDContextContainer.Types)

    All contexts are linked together in (FTDContext.ContextList),  avoiding
    list  management  for  each  kind of context (also saving some memory).
    This member  is  used  by  td_context_pop()  to  restaure  the  correct
    context.

*/

enum    {

        FV_TD_CONTEXT_FONT = 1,
        FV_TD_CONTEXT_PENS

        };

//+
///FTDContext
typedef struct in_FeelinTD_Context
{
    FTDParseResult                  result;                 // used by parse function to get attributes

    FTDLine                        *line;                   // current line
    UBYTE                           flags;
    UBYTE                           align;
    UBYTE                           spacing;
    UBYTE                           style;
    UBYTE                           mode;
    UBYTE                           shortcut;
    BYTE                            stop_shortcut;          // if TRUE, stop search for shortcuts
    BYTE                            stop_engine;            // if true engine is stoped, text is then considered as plain until </stop>

    /* contexts */

    FTDContextContainer            *container_font;
    FTDContextContainer            *container_pens;

    FList                           ContainerList;

    /* The ColorsList *must* be stolen  before  the  context  is  disposed,
    otherwise all FTDColor will be disposed with it. */

    FList                           ColorsList;
}
FTDContext;

 
#define FF_TD_CONTEXT_EXTRA_TOPLEFT             (1 << 0)
#define FF_TD_CONTEXT_EXTRA_BOTTOMRIGHT         (1 << 1)
//+


/*

    The following functions are used to create, delete and  manipulate  the
    FTDContext structure

*/

FTDContext          * td_context_new(struct FeelinClass *Class,FObject Obj);
void                  td_context_dispose(FTDContext *Context);
APTR                  td_context_push(FTDContext *Context,uint32 Type);
FTDContextContainer * td_context_pop(FTDContext *Context,uint32 Type);
FTDColor            * td_context_add_color(FTDContext *Context,uint32 Spec,uint32 Type);

/*

   Les  structures  FTDContextXxx  sont   utilis�es   pour   contenir   les
   informations  d'un  contexte,  c'est � dire les modifications entre deux
   commandes e.g. <font...> & </font>. Ces structures sont aussi  utilis�es
   dans  les chunks. Elle sont cr�es � la vol�e pour �conomiser de l'espace
   et r�duire la taille de la structure FTDTextchunk. Ainsi, si  le  membre
   'font'  de  la  structure  FTDTextChunk est NULL les valeurs par d�fault
   doivent �tres utilis�es (e.g. couleur de text FV_Pen_Text, fonte  donn�e
   par  l'attribut  FA_TextDisplay_Font  avec  la  taille par d�fault de la
   fonte)

   ************

   Un contexte est trouv� (e.g. <font>)

   la fonction td_context_push() est appel�e pour mettre en  file  l'ancien
   contexte.

   un  nouveau  contexte  est  cr�e  (vide  pour  le  moment).  Le  bool�en
   'FTDContextContainer.Stolen' devient FALSE.

   1] fin des balises, texte brut

      Un chunk texte est cr�e.

      1.1] 'FTDContextContainer.Stolen' est FALSE

         Le chunk prend possession du contexte (en l'occurence la structure
         FTDContextFont associ�e � la structure FTDContextContainer) et met
         le  bool�en  'FTDContextContainer.Stolen'   TRUE.   La   structure
         FTDContextFont sera d�truite lorsque le chunk texte sera d�truit.

      1.2] 'FTDContextContainer.saved' est TRUE

         Le contexte � d�j� �tait sauv� par un pr�c�dent chunk. Le drapeaux
         FF_TD_TEXTCHUNK_INHERIT_FONT       est       positionn�       dans
         'FTDTextChunk.inherit'. Le contexte (en l'occurence  la  structure
         FTDContextFont  associ�e  �  la structure FTDContextContainer) est
         utilis� comme r�f�rence.

   2] balise de fin </font>

      la fonction td_context_pop()  est  appel�e  pour  restaurer  l'ancien
      contexte.  Le  contexte  pr�sent  doit donc �tre d�truit. La fonction
      td_context_pop() v�rifie toujours si un  contexte  est  bien  pr�sent
      dans la file, l'abus de 'pop' n'est pas dangereux.

      2.1] 'FTDContextContainer.Stolen' est FALSE

         Le contexte n'a pas �t� sauv� car aucun chunk texte n'a �t�  cr�e.
         Tant pis pour luis, le contexte est d�truit ainsi que la structure
         FTDContextContainer associ�e.

      2.2] 'FTDContextContainer.Stolen' est TRUE

         Le context a �t� sauv� par un  chunk  texte.  Seule  la  structure
         FTDContextContainer est d�truite.

*/

#endif
