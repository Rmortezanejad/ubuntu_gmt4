#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Scrollbar.h>

#define Min(a,b) ((a)<(b)?(a):(b))
#define Max(a,b) ((a)>(b)?(a):(b))

#include <stddef.h>

#include "xgrid_utility.h"

#include "xgrid_PannerP.h"

static XtResource resources[] = {
#define offset(field) XtOffset(PannerWidget, panner.field)
  { XtNcanvas, XtCCanvas, XtRWidget, sizeof(Widget),
  	offset(canvas), XtRWidget, NULL },

  { XtNxRange, XtCXRange, XtRInt, sizeof(int),
  	offset(horizontal.range), XtRInt, (XtPointer)0 },
  { XtNxPosition, XtCXPosition, XtRInt, sizeof(int),
  	offset(horizontal.position), XtRPosition, (XtPointer)0 },
  { XtNxCallback, XtCXCallback, XtRCallback, sizeof(XtCallbackList),
  	offset(horizontal.callback), XtRPointer, NULL },
  { XtNxScrollStep, XtCXScrollStep, XtRDimension, sizeof(Dimension),
  	offset(horizontal.scroll_step), XtRDimension, 0 },
	
  { XtNyRange, XtCYRange, XtRInt, sizeof(int),
  	offset(vertical.range), XtRInt, (XtPointer)0 },
  { XtNyPosition, XtCYPosition, XtRInt, sizeof(int),
  	offset(vertical.position), XtRInt, (XtPointer)0 },
  { XtNyCallback, XtCYCallback, XtRCallback, sizeof(XtCallbackList),
  	offset(vertical.callback), XtRPointer, NULL },
  { XtNyScrollStep, XtCYScrollStep, XtRDimension, sizeof(Dimension),
  	offset(vertical.scroll_step), XtRDimension, 0 }
#undef offset
};

/****	Local procedures	****/

static void doLayout (); /* PannerWidget self */
static void recalculateBar (); /* PanScrollRec * bar */

static void jumpProc (); /* Widget bar, PanScrollRec * data, float * fraction */
static void scrollProc (); /* Widget bar, PanScrollRec * data, int position */

/****	Methods for Panner.	****/

/* At initialisation, create our two scrollbar widgets */
static void PannerInitialize (Widget request __attribute__((unused)),
			      Widget result, ArgList args __attribute__((unused)),
			      Cardinal *nargs __attribute__((unused)))
{
  Widget bar;
  PanScrollRec *horizontal = &((PannerWidget)result)->panner.horizontal;
  PanScrollRec *vertical = &((PannerWidget)result)->panner.vertical;
 
  /* Horizontal bar */
  bar = XtVaCreateManagedWidget("horizontal", scrollbarWidgetClass,
  		result,
		XtNorientation, (XtArgVal)XtorientHorizontal,
		XtNminimumThumb, (XtArgVal)16,
  		/* Adjust for (IMHO) backwards Athena logic */
		XtNscrollDCursor,
		XCreateFontCursor(XtDisplay(result), XC_sb_up_arrow),
		XtNscrollUCursor,
		XCreateFontCursor(XtDisplay(result), XC_sb_down_arrow),
  		NULL);
  XawScrollbarSetThumb(bar, 0.0, 1.0);
  XtAddCallback(bar, XtNscrollProc, scrollProc, horizontal);
  XtAddCallback(bar, XtNjumpProc, jumpProc, horizontal);
  horizontal->scrollbar   = bar;
  horizontal->orientation = XtorientHorizontal;

  /* Vertical bar. We don't swap the arrows because the
     actions are reversed instead.  */
  bar = XtVaCreateManagedWidget("vertical", scrollbarWidgetClass,
  		result,
		XtNorientation, (XtArgVal)XtorientVertical,
		XtNminimumThumb, (XtArgVal)16,
		NULL);
  XawScrollbarSetThumb(bar, 0.0, 1.0);
  XtAddCallback(bar, XtNscrollProc, scrollProc, vertical);
  XtAddCallback(bar, XtNjumpProc, jumpProc, vertical);
  vertical->scrollbar   = bar;
  vertical->orientation = XtorientVertical;
}

/* If range or position changed, recalculate appearance
   of scrollbars. Don't bother checking for width, height
   resources because the resize method will catch those.   */
static Boolean PannerSetValues (Widget old, Widget request __attribute__((unused)),
				Widget new, ArgList args __attribute__((unused)),
				Cardinal *nargs __attribute__((unused)))
{
  PannerWidget oldp, newp;
  
  oldp = (PannerWidget)old;
  newp = (PannerWidget)new;
  
  if ((oldp->panner.horizontal.range != newp->panner.horizontal.range)
  	|| (oldp->panner.horizontal.position != newp->panner.horizontal.position))
    recalculateBar(&(newp->panner.horizontal));

  if ((oldp->panner.vertical.range != newp->panner.vertical.range)
  	|| (oldp->panner.vertical.position != newp->panner.vertical.position))
    recalculateBar(&(newp->panner.vertical));

  if (oldp->panner.canvas != newp->panner.canvas)
    doLayout(newp);
  /* All the changes have been made to child widgets, not ourselves,
     so no redisplay is needed */
  return False;
}

static void PannerResize (Widget self)
{
  doLayout((PannerWidget)self);
}

static void doLayout (PannerWidget self)
{
  Dimension width, height, hBar, vBar;
  
  XtVaGetValues((Widget) self,
  	XtNwidth, &width,
	XtNheight, &height,
	NULL);
  XtVaGetValues(self->panner.horizontal.scrollbar,
  		XtNheight, &hBar, NULL);
  XtVaGetValues(self->panner.vertical.scrollbar,
  		XtNwidth, &vBar, NULL);

  XtConfigureWidget(self->panner.canvas,
  	0, 0, width - vBar, height - hBar, 0);
  XtConfigureWidget(self->panner.horizontal.scrollbar,
  	0, height - hBar, width - vBar, hBar, 0);
  XtConfigureWidget(self->panner.vertical.scrollbar,
  	width - vBar, 0, vBar, height - hBar, 0);

  recalculateBar(&(self->panner.horizontal));
  recalculateBar(&(self->panner.vertical));
}

static void recalculateBar (PanScrollRec * bar)
{
  float top, size;
  
  /* How much of range is visible? */
  if (bar->orientation == XtorientHorizontal)
    bar->shown = bar->scrollbar->core.width;
  else
    bar->shown = bar->scrollbar->core.height;
  /* Consistency check  */
  bar->range = Max(bar->range, 0);
  bar->position = Min(bar->position, (bar->range - bar->shown));
  bar->position = Max(bar->position, 0);
  /* Adjust scrollbar */
  if (bar->range == 0 || bar->shown >= bar->range)
    XawScrollbarSetThumb(bar->scrollbar, 0.0, 1.0);
  else {
    top  = (float)(bar->position) / (float)(bar->range);
    size = (float)(bar->shown) / (float)(bar->range);
    XawScrollbarSetThumb(bar->scrollbar, top, size);
  }
}

/****	Class record	****/

static CompositeClassExtensionRec extension = {
    /* next_extension		*/ NULL,
    /* record_type		*/ NULLQUARK,
    /* version			*/ XtCompositeExtensionVersion,
    /* record_size		*/ sizeof(CompositeClassExtensionRec),
    /* accepts_objects		*/ True
};

PannerClassRec pannerClassRec = {
  { /* core fields */
    /* superclass		*/	(WidgetClass) &compositeClassRec,
    /* class_name		*/	"Panner",
    /* widget_size		*/	sizeof(PannerRec),
    /* class_initialize		*/	NULL,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	PannerInitialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	XtInheritRealize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* num_resources		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	NULL,
    /* resize			*/	PannerResize,
    /* expose			*/	NULL,
    /* set_values		*/	PannerSetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	XtInheritTranslations,
    /* query_geometry		*/	NULL,
    /* display_accelerator	*/	XtInheritDisplayAccelerator,
    /* extension		*/	NULL
  },
  { /* Composite part */
    /* geometry_manager		*/	XtInheritGeometryManager,
    /* change_managed		*/	XtInheritChangeManaged,
    /* insert_child		*/	XtInheritInsertChild,
    /* delete_child		*/	XtInheritDeleteChild,
    /* extension		*/	(XtPointer)&extension
  },
  { /* panner fields */
    /* empty			*/	0
  }
};

WidgetClass pannerWidgetClass = (WidgetClass)&pannerClassRec;

/****	Scrolling by dragging the thumb.

 	1.  The user is dragging the centre of the thumb
	around, which I think gives a better feel at the
	ends of the scrollbar.
	2. You can't scroll past the bottom of the data,
	ie the thumb is always fully visible	****/

static void jumpProc (Widget bar, PanScrollRec *data, float *fraction)
{
  float	   	thumbSize, scaleFrac;
  int	 	previous;

  if (data->range == 0 || data->shown > data->range)
    return;
    
  XtVaGetValues(bar, XtNshown, &thumbSize, NULL);
  /* The centre of a thumb size X can only be in the
     range X/2 .. 1 - X/2, so scale such a value
     back to 0..1. Values below 0.0 or above 1.0 are
     clipped.					*/
  scaleFrac = (*fraction - (thumbSize/2)) / (1.0 - thumbSize);
  scaleFrac = Max(scaleFrac, 0.0);
  scaleFrac = Min(scaleFrac, 1.0);
  previous = data->position;
  data->position = (int)(scaleFrac * (data->range - data->shown));
  recalculateBar(data);
  /* Notify client */
/* With 64-bit we started to get these messages when compiling xgrid:
 * xgrid_Panner.c:258: warning: cast to pointer from integer of different size.
 * Per Lloyd Parkes, fixed by casting the argument via (ptrdiff_t).
 */
  if (data->position != previous)
    XtCallCallbackList(XtParent(bar), data->callback, (XtPointer)(ptrdiff_t)data->position);
}

/****	Scrolling by clicking the up or down arrows.

	1. As before, you can't scroll past the bottom.
	2. This scrolls by a fixed percentage of the
	window rather than proportionally according
	to the position of the cursor in the bar.
						****/

static void scrollProc (Widget bar, PanScrollRec *data, int position)
{
  int	    distance;
  Position  previous;

  previous = data->position;
  /* Adjust for (IMHO) backwards Athena logic */
  if (data->orientation == XtorientHorizontal)
    position = - position;
  /* Just move up or down by fixed proportion of shown */
  if (data->scroll_step != 0)
    distance = data->scroll_step;
  else
    distance = Max(0.9 * data->shown, 4);
  if (position < 0)
    data->position -= distance;
  else
    data->position += distance;
  recalculateBar(data);
  /* Notify client */
/* With 64-bit we started to get these messages when compiling xgrid:
 * xgrid_Panner.c:297: warning: cast to pointer from integer of different size.
 * Per Lloyd Parkes, fixed by casting the argument via (ptrdiff_t).
 */
  if (data->position != previous)
    XtCallCallbackList(XtParent(bar), data->callback, (XtPointer)(ptrdiff_t)data->position);
}

/****	Convenience functions	****/

void XtSetPannerCanvas (Widget self, Widget canvas)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtSetPannerCanvas");
  XtVaSetValues(self, XtNcanvas, canvas, NULL);
}

int XtPannerXPosition (Widget self)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtPannerXPosition");
  return ((PannerWidget)self)->panner.horizontal.position;
}

int XtPannerYPosition (Widget self)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtPannerYPosition");
  return ((PannerWidget)self)->panner.vertical.position;
}

void XtSetPannerXPosition (Widget self, int position)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtSetPannerXPosition");
  XtVaSetValues(self, XtNxPosition, position, NULL);
}

void XtSetPannerYPosition (Widget self, int position)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtSetPannerYPosition");
  XtVaSetValues(self, XtNyPosition, position, NULL);
}

void XtSetPannerXRange (Widget self, int range)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtSetPannerXRange");
  XtVaSetValues(self, XtNxRange, range, NULL);
}

void XtSetPannerYRange (Widget self, int range)
{
  XtCheckSubclass(self, pannerWidgetClass, "XtSetPannerYRange");
  XtVaSetValues(self, XtNyRange, range, NULL);
}
