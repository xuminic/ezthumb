/** \file
 * \brief iupmatrix control
 * draw functions.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUPMAT_DRAW_H 
#define __IUPMAT_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Render the specified cells only, does nothing when using IupDraw */
void iupMatrixDrawCells(Ihandle* ih, int lin1, int col1, int lin2, int col2);
void iupMatrixDrawTitleColumns(Ihandle* ih, int col1, int col2);
void iupMatrixDrawTitleLines(Ihandle* ih, int lin1, int lin2);

/* Render all the visible cells, does nothing when using IupDraw.
   Optionally update the display by calling iupMatrixDrawUpdate. */
void iupMatrixDraw(Ihandle* ih, int update);

/* Update the display only, only calls IupUpdate when using IupDraw */
void iupMatrixDrawUpdate(Ihandle* ih);

#ifndef USE_OLD_DRAW
/* Redraw everything, called only from the ACTION callback */
void iupMatrixDrawCB(Ihandle* ih);
#endif

/* Process the REDRAW attribute, only calls IupUpdate when using IupDraw */
int iupMatrixDrawSetRedrawAttrib(Ihandle* ih, const char* value);

/* Aux, don't actually draw anything */
void iupMatrixDrawSetDropFeedbackArea(int *x1, int *y1, int *x2, int *y2);
void iupMatrixDrawSetToggleFeedbackArea(int toggle_centered, int *x1, int *y1, int *x2, int *y2);


#ifdef __cplusplus
}
#endif

#endif
