/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: Base class for console units
    Lang: english
*/

#include <proto/boopsi.h>
#include <proto/utility.h>
#include <aros/asmcall.h>
#include <devices/conunit.h>
#include <intuition/classes.h>
#include <intuition/intuition.h>
#include "consoleif.h"
#include "console_gcc.h"

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

VOID normalizecoords(Object *o, WORD *x_ptr, WORD *y_ptr);

/*
   Base class for the classes handling standard, charmap and snipmap
   consoles. This is a whitebox base class (like GADGETCLASS), which means subclasses
   can have direct access to its instance data.
*/

struct consoledata
{
    struct intConUnit intunit;
};


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)cl->cl_UserData)


/*********************
**  Console::New()  **
*********************/
static Object *console_new(Class *cl, Object *o, struct opSet *msg)
{
    struct Window *win;
    EnterFunc(bug("Console::New()\n"));
    
    /* Get console window */
    win = (struct Window *)GetTagData(A_Console_Window, NULL, msg->ops_AttrList);
    if (!win)
    	ReturnPtr ("Console::New", Object *, NULL);
	
    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (o)
    {
    	struct ConUnit *unit;
	struct consoledata *data;

    	struct RastPort *rp = win->RPort;
    	    	
	data = INST_DATA(cl, o);
	
	unit = (struct ConUnit *)data;
    	/* Initialize the unit fields */
    	unit->cu_Window = win;
    	    	

	/* For now one should use only non-proportional fonts */
    	unit->cu_XRSize	= rp->Font->tf_XSize;
    	unit->cu_YRSize	= rp->Font->tf_YSize;
    	    	
    	D(bug("cu_XRSize: %d, cu_YRSize: %d\n",
    	    	unit->cu_XRSize, unit->cu_YRSize));

     	/* Use whole window for console. */
    	unit->cu_XMax 	  = (win->Width  - (win->BorderLeft + win->BorderRight )) / unit->cu_XRSize - 1;
    	unit->cu_YMax 	  = (win->Height - (win->BorderTop  + win->BorderBottom)) / unit->cu_YRSize - 1;
    	    	
    	unit->cu_XROrigin = win->BorderLeft;
    	unit->cu_YROrigin = win->BorderTop;

    	D(bug("cu_XROrigin: %d, cu_YROrigin: %d\n",
    	unit->cu_XROrigin, unit->cu_YROrigin));
    	    	
    	unit->cu_XRExtant = win->BorderLeft + (unit->cu_XRSize * unit->cu_XMax);
    	unit->cu_YRExtant = win->BorderTop  + (unit->cu_YRSize * unit->cu_YMax);
	
	unit->cu_XCP = DEF_CHAR_XMIN;
	unit->cu_YCP = DEF_CHAR_YMIN;

	unit->cu_XCCP = DEF_CHAR_XMIN;
	unit->cu_YCCP = DEF_CHAR_YMIN;
	
	ICU(o)->conFlags = 0UL;
	    
	
    }
    ReturnPtr("Console::New", Object *, o);
    
}
 

#define XCP (CU(o)->cu_XCP) /* Character X pos */
#define YCP (CU(o)->cu_YCP) /* Character Y pos */

#define XCCP (CU(o)->cu_XCCP) /* Cursor X pos */
#define YCCP (CU(o)->cu_YCCP) /* Cusror U pos */

/**********************
**  Console::Left()  **
**********************/
static VOID console_left(Class *cl, Object *o, struct P_Console_Left *msg)
{
    XCP -= msg->Num;
    XCCP -= msg->Num;
    if (XCCP < 0)
    {
    	XCCP = 0;
    }
    
    return;
}

/***********************
**  Console::Right()  **
***********************/
static VOID console_right(Class *cl, Object *o, struct P_Console_Right *msg)
{

    EnterFunc(bug("Console::Right()\n"));

    XCP  += msg->Num;
    XCCP += msg->Num;
    
    D(bug("XCP=%d, XCCP=%d\n", XCP, XCCP));
    
    if (XCCP > CHAR_XMAX(o))
    {
    	XCCP = CHAR_XMIN(o);
    	Console_Down(o, 1);
    }
    
    ReturnVoid("Console::Right");
}

/********************
**  Console::Up()  **
********************/
static VOID console_up(Class *cl, Object *o, struct P_Console_Up *msg)
{

    YCP  -= msg->Num;
    YCCP -= msg->Num;

    if (YCCP < CHAR_YMIN(o))
    {
    	YCCP = CHAR_YMIN(o);
    }

    return;
}



/**********************
**  Console::Down()  **
**********************/
static VOID console_down(Class *cl, Object *o, struct P_Console_Down *msg)
{
    EnterFunc(bug("Console::Down(num=%d)\n", msg->Num));
    YCP += msg->Num;
    YCCP += msg->Num;
    
    if (YCCP > CHAR_YMAX(o))
    {
    	UBYTE scroll_param = 1;

	Console_DoCommand(o, C_SCROLL_UP, &scroll_param);
	YCCP = CHAR_YMAX(o);
    }
    D(bug("New coords: char (%d, %d), gfx (%d, %d)\n",
    	XCCP, YCCP, CP_X(o), CP_Y(o) ));
    ReturnVoid("Console::Down");
}

AROS_UFH3(static IPTR, dispatch_consoleclass,
    AROS_UFHA(Class *,  cl,  A0),
    AROS_UFHA(Object *, o,   A2),
    AROS_UFHA(Msg,      msg, A1)
)
{
    IPTR retval = 0UL;
    
    switch (msg->MethodID)
    {
    case OM_NEW:
    	retval = (IPTR)console_new(cl, o, (struct opSet *)msg);
	break;
	
    case M_Console_Left:
    	console_left(cl, o, (struct P_Console_Left *)msg);
	break;

    case M_Console_Right:
    	console_right(cl, o, (struct P_Console_Right *)msg);
	break;

    case M_Console_Up:
    	console_up(cl, o, (struct P_Console_Up *)msg);
	break;

    case M_Console_Down:
    	console_down(cl, o, (struct P_Console_Down *)msg);
	break;
    default:
    	retval = DoSuperMethodA(cl, o, msg);
	break;
	
    }
    return (retval);
	
}

/************************
**  Support functions  **
************************/

/************************
**  normalizecoords()  **
************************/

/* Normalizes "out of window" coords, so that they
   fit into the window
*/

#define ABS(a) (((a) < 0) ? -(a) : (a))


#undef ConsoleDevice
#define ConsoleDevice ((struct ConsoleBase *)OCLASS(o)->cl_UserData)

#warning Currently dead code
VOID normalizecoords(Object *o, WORD *x_ptr, WORD *y_ptr)
{
    EnterFunc(bug("normalizecoords(o=%p, x=%d, y=%d)\n",
    	o, *x_ptr, *y_ptr));

    if (*x_ptr > CU(o)->cu_XMax) /* charpos too far to the right */
    {
	D(bug("Pos right of window\n"));
    	/* Increase y */
	Console_Down(o, *x_ptr / CHAR_XMAX(o) );
	
	/* Normalize x */
    	*x_ptr = *x_ptr % (CHAR_XMAX(o) - CHAR_XMIN(o));
    }
    else if (*x_ptr < CHAR_XMIN(o))
    {
	D(bug("Pos left of window\n"));
	    	
	/* Decrease y */
	Console_Up(o, ABS(*x_ptr) / CHAR_XMAX(o) - CHAR_XMIN(o) );
	
	/* Normalize Z */
	*x_ptr = *x_ptr % (CHAR_XMAX(o) - CHAR_XMIN(o));
    
    }
    
    if (*y_ptr > CHAR_YMAX(o))	/* pos below window bounds */
    {
	
    	*y_ptr = CHAR_YMAX(o);
    }
    else if (*y_ptr < CHAR_YMIN(o))
    {
    	UBYTE scroll_param = CHAR_YMIN(o) - *y_ptr;	/* pos above window bounds */
	
	D(bug("Pos above window\n"));
	
    	Console_DoCommand(o, C_SCROLL_DOWN, &scroll_param);

    	*y_ptr = CHAR_YMIN(o);
    
    }

    ReturnVoid("normalizecoords");

}


#undef ConsoleDevice

Class *makeConsoleClass(struct ConsoleBase *ConsoleDevice)
{
    
   Class *cl;
   	
   cl = MakeClass(NULL, ROOTCLASS, NULL, sizeof(struct consoledata), 0UL);
   if (cl)
   {
    	cl->cl_Dispatcher.h_Entry = (APTR)dispatch_consoleclass;
    	cl->cl_Dispatcher.h_SubEntry = NULL;

    	cl->cl_UserData = (IPTR)ConsoleDevice;

   	return (cl);
    }
    return (NULL);
}

