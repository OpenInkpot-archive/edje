/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#include "edje_private.h"

static int _edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos);
static int _edje_entry_imf_event_commit_cb(void *data, int type, void *event);
static int _edje_entry_imf_event_changed_cb(void *data, int type, void *event);
static int _edje_entry_imf_event_delete_surrounding_cb(void *data, int type, void *event);

typedef struct _Entry Entry;
typedef struct _Sel Sel;
typedef struct _Anchor Anchor;

struct _Entry
{
   Edje_Real_Part *rp;
   Evas_Coord cx, cy;
   Evas_Object *cursor_bg;
   Evas_Object *cursor_fg;
   Evas_Textblock_Cursor *cursor;
   Evas_Textblock_Cursor *sel_start, *sel_end;
   Eina_List *sel;
   Eina_List *anchors;
   Eina_List *anchorlist;
   char *selection;
   Evas_Bool selecting : 1;
   Evas_Bool have_selection : 1;
   Evas_Bool select_allow : 1;
   Evas_Bool select_mod_start : 1;
   Evas_Bool select_mod_end : 1;
   Evas_Bool had_sel : 1;

#ifdef ECORE_IMF   
   int	comp_len;

   Ecore_IMF_Context *imf_context;

   Ecore_Event_Handler *imf_ee_handler_commit;
   Ecore_Event_Handler *imf_ee_handler_delete;
   Ecore_Event_Handler *imf_ee_handler_changed;
#endif   
};

struct _Sel
{
   Evas_Textblock_Rectangle rect;
   Evas_Object *obj_fg, *obj_bg, *obj, *sobj;
};

struct _Anchor
{
   Entry *en;
   char *name;
   Evas_Textblock_Cursor *start, *end;
   Eina_List *sel;
};

#ifdef ECORE_IMF   
static void 
_edje_entry_focus_in_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   Edje_Real_Part *rp = data;
   if (!rp) return;
   Entry *en = rp->entry_data;
   
   if (!en) return;

   if (!rp->edje || !rp->edje->obj) return;
   
   if (!en->imf_context) return;

   if (evas_object_focus_get(rp->edje->obj))
     {
	ecore_imf_context_reset(en->imf_context);
	ecore_imf_context_focus_in(en->imf_context);
     }
}

static void
_edje_entry_focus_out_cb(void *data, Evas_Object *o, const char *emission, const char *source)
{
   Edje_Real_Part *rp = data;
   if (!rp) return;
   Entry *en = rp->entry_data;
   if (!en) return;

   if (!en->imf_context) return;

   ecore_imf_context_reset(en->imf_context);
   ecore_imf_context_cursor_position_set(en->imf_context, evas_textblock_cursor_pos_get(en->cursor));
   ecore_imf_context_focus_out(en->imf_context);
}
#endif

static void
_edje_focus_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje *ed = data;
#ifdef ECORE_IMF   
   Edje_Real_Part *rp;
   Entry *en;
#endif
   
   _edje_emit(ed, "focus,in", "");
#ifdef ECORE_IMF
   rp = ed->focused_part;
   if (rp == NULL) return;
   
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;
 
   if (en->imf_context)
     {
	ecore_imf_context_reset(en->imf_context);
	ecore_imf_context_focus_in(en->imf_context);
     }
#endif
}
    
static void
_edje_focus_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje *ed = data;
#ifdef ECORE_IMF
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
#endif
   
   _edje_emit(ed, "focus,out", "");

#ifdef ECORE_IMF
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;

   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
        ecore_imf_context_focus_out(en->imf_context);
     }
#endif
}

static void
_curs_update_from_curs(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Evas_Coord cx, cy, cw, ch;
   evas_textblock_cursor_char_geometry_get(c, &cx, &cy, &cw, &ch);
   en->cx = cx + (cw / 2);
   en->cy = cy + (ch / 2);
}

static void
_curs_back(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!evas_textblock_cursor_char_prev(c))
     {
	if (evas_textblock_cursor_node_prev(c))
	  {
	     while (evas_textblock_cursor_node_format_get(c))
	       {
		  if (evas_textblock_cursor_node_format_is_visible_get(c)) break;
		  if (!evas_textblock_cursor_node_prev(c)) break;
	       }
	  }
     }
   _curs_update_from_curs(c, o, en);
}

static void
_curs_next(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!evas_textblock_cursor_char_next(c))
     {
	if (evas_textblock_cursor_node_next(c))
	  {
	     while (evas_textblock_cursor_node_format_get(c))
	       {
		  if (evas_textblock_cursor_node_format_is_visible_get(c))
		    break;
		  if (!evas_textblock_cursor_node_next(c))
		    break;
	       }
	  }
     }
   else
     {
	int len, pos;
	
	len = evas_textblock_cursor_node_text_length_get(c);
	pos = evas_textblock_cursor_pos_get(c);
	if (pos == len)
	  evas_textblock_cursor_node_next(c);
     }
   _curs_update_from_curs(c, o, en);
}

static int
_curs_line_last_get(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Evas_Textblock_Cursor *cc;
   int ln;
   
   cc = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_node_last(cc);
   ln = evas_textblock_cursor_line_geometry_get(cc, NULL, NULL, NULL, NULL);
   evas_textblock_cursor_free(cc);
   return ln;
}

static void
_curs_jump_line(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en, int ln)
{
   Evas_Coord lx, ly, lw, lh;

   if (ln < 0) ln = 0;
   else
     {
	int last = _curs_line_last_get(c, o, en);
	if (ln > last) ln = last;
     }
   if (!evas_object_textblock_line_number_geometry_get(o, ln, &lx, &ly, &lw, &lh))
     return;
   if (evas_textblock_cursor_char_coord_set(c, en->cx, ly + (lh / 2)))
     return;
   evas_textblock_cursor_line_coord_set(c, ly + (lh / 2));
   if (en->cx < (lx + (lw / 2))) evas_textblock_cursor_line_first(c);
   else evas_textblock_cursor_line_last(c);
}

static void
_curs_jump_line_by(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en, int by)
{
   int ln;
   
   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL) + by;
   _curs_jump_line(c, o, en, ln);
}

static void
_curs_up(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   _curs_jump_line_by(c, o, en, -1);
}

static void
_curs_down(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   _curs_jump_line_by(c, o, en, 1);
}

static void
_curs_lin_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_first(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_lin_end(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_last(c);
   if (!evas_textblock_cursor_node_format_get(c))
     _curs_next(c, o, en);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_set(c, 0);
   evas_textblock_cursor_line_first(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_end(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_set(c, _curs_line_last_get(c, o, en));
   _curs_lin_end(c, o, en);
   _curs_update_from_curs(c, o, en);
}



static void
_sel_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (en->sel_start) return;
   en->sel_start = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_copy(c, en->sel_start);
   en->sel_end = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_copy(c, en->sel_end);
   en->have_selection = 0;
   if (en->selection)
     {
	free(en->selection);
	en->selection = NULL;
     }
}

static void
_sel_enable(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (en->have_selection) return;
   en->have_selection = 1;
   if (en->selection)
     {
	free(en->selection);
	en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,start", en->rp->part->name);
}

static void
_sel_extend(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!en->sel_end) return;
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_end)) return;
   evas_textblock_cursor_copy(c, en->sel_end);
   if (en->selection)
     {
	free(en->selection);
	en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
}

static void
_sel_preextend(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!en->sel_end) return;
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_start)) return;
   evas_textblock_cursor_copy(c, en->sel_start);
   if (en->selection)
     {
	free(en->selection);
	en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
}

static void
_sel_clear(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   en->had_sel = 0;
   if (en->sel_start)
     {
	evas_textblock_cursor_free(en->sel_start);
	evas_textblock_cursor_free(en->sel_end);
	en->sel_start = NULL;
	en->sel_end = NULL;
     }
   if (en->selection)
     {
	free(en->selection);
	en->selection = NULL;
     }
   while (en->sel)
     {
	Sel *sel;
	
	sel = en->sel->data;
        en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
        en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
	if (sel->obj_bg) evas_object_del(sel->obj_bg);
	if (sel->obj_fg) evas_object_del(sel->obj_fg);
	free(sel);
	en->sel = eina_list_remove_list(en->sel, en->sel);
     }
   if (en->have_selection)
     {
        en->have_selection = 0;
	_edje_emit(en->rp->edje, "selection,cleared", en->rp->part->name);
     }
}

static void
_sel_update(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Eina_List *range = NULL, *l;
   Sel *sel;
   Evas_Coord x, y, w, h;
   Evas_Object *smart, *clip;
   
   smart = evas_object_smart_parent_get(o);
   clip = evas_object_clip_get(o);
   if (en->sel_start)
     range = evas_textblock_cursor_range_geometry_get(en->sel_start, en->sel_end);
   else
     return;
   if (eina_list_count(range) != eina_list_count(en->sel))
     {
	while (en->sel)
	  {
	     sel = en->sel->data;
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
	     if (sel->obj_bg) evas_object_del(sel->obj_bg);
	     if (sel->obj_fg) evas_object_del(sel->obj_fg);
	     free(sel);
	     en->sel = eina_list_remove_list(en->sel, en->sel);
	  }
	if (en->have_selection)
	  {
	     for (l = range; l; l = eina_list_next(l))
	       {
		  Evas_Object *ob;
		  
		  sel = calloc(1, sizeof(Sel));
		  en->sel = eina_list_append(en->sel, sel);
		  ob = edje_object_add(en->rp->edje->evas);
		  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source);
		  evas_object_smart_member_add(ob, smart);
		  evas_object_stack_below(ob, o);
		  evas_object_clip_set(ob, clip);
		  evas_object_pass_events_set(ob, 1);
		  evas_object_show(ob);
		  sel->obj_bg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_bg);
                  
		  ob = edje_object_add(en->rp->edje->evas);
		  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source2);
		  evas_object_smart_member_add(ob, smart);
		  evas_object_stack_above(ob, o);
		  evas_object_clip_set(ob, clip);
		  evas_object_pass_events_set(ob, 1);
		  evas_object_show(ob);
		  sel->obj_fg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_fg);
	       }
	  }
     }
   x = y = w = h = -1;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   if (en->have_selection)
     {
	EINA_LIST_FOREACH(en->sel, l, sel)
	  {
	     Evas_Textblock_Rectangle *r;
	     
	     r = range->data;
	     if (sel->obj_bg)
	       {
		  evas_object_move(sel->obj_bg, x + r->x, y + r->y);
		  evas_object_resize(sel->obj_bg, r->w, r->h);
	       }
	     if (sel->obj_fg)
	       {
		  evas_object_move(sel->obj_fg, x + r->x, y + r->y);
		  evas_object_resize(sel->obj_fg, r->w, r->h);
	       }
	     *(&(sel->rect)) = *r;
	     range = eina_list_remove_list(range, range);
	     free(r);
	  }
     }
   else
     {
	while (range)
	  {
	     free(range->data);
	     range = eina_list_remove_list(range, range);
	  }
     }
}

static void
_edje_anchor_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   int len;
   int ignored;
   Entry *en;
   
   en = rp->entry_data;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
       (en->select_allow))
     return;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
	n = an->name;
	if (!n) n = "";
	len = 200 + strlen(n);
	buf = alloca(len);
        if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK)
          snprintf(buf, len, "anchor,mouse,down,%i,%s,triple", ev->button, an->name);
        else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
	  snprintf(buf, len, "anchor,mouse,down,%i,%s,double", ev->button, an->name);
	else
	  snprintf(buf, len, "anchor,mouse,down,%i,%s", ev->button, an->name);
	_edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   int len;
   int ignored;
   Entry *en;
   
   en = rp->entry_data;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
       (en->select_allow))
     return;
   if ((!ev->event_flags) || (!ignored))
     {
	n = an->name;
	if (!n) n = "";
	len = 200 + strlen(n);
	buf = alloca(len);
	snprintf(buf, len, "anchor,mouse,up,%i,%s", ev->button, an->name);
	_edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   int len;
   int ignored;
   Entry *en;
   
   en = rp->entry_data;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
       (en->select_allow))
     return;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
	n = an->name;
	if (!n) n = "";
	len = 200 + strlen(n);
	buf = alloca(len);
	snprintf(buf, len, "anchor,mouse,move,%s", an->name);
	_edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_in_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_In *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   int len;
   int ignored;

   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
	n = an->name;
	if (!n) n = "";
	len = 200 + strlen(n);
	buf = alloca(len);
	snprintf(buf, len, "anchor,mouse,in,%s", an->name);
	_edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_out_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Out *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   int len;
   int ignored;

   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
	n = an->name;
	if (!n) n = "";
	len = 200 + strlen(n);
	buf = alloca(len);
	snprintf(buf, len, "anchor,mouse,out,%s", an->name);
	_edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_anchors_update(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Eina_List *l, *ll, *range;
   Evas_Coord x, y, w, h;
   Evas_Object *smart, *clip;
   Sel *sel;
   Anchor *an;

   smart = evas_object_smart_parent_get(o);
   clip = evas_object_clip_get(o);
   x = y = w = h = -1;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   EINA_LIST_FOREACH(en->anchors, l, an)
     {
	range = evas_textblock_cursor_range_geometry_get(an->start, an->end);
	if (eina_list_count(range) != eina_list_count(an->sel))
	  {
	     while (an->sel)
	       {
		  sel = an->sel->data;
                  en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
                  en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
		  if (sel->obj_bg) evas_object_del(sel->obj_bg);
		  if (sel->obj_fg) evas_object_del(sel->obj_fg);
		  if (sel->obj) evas_object_del(sel->obj);
		  free(sel);
		  an->sel = eina_list_remove_list(an->sel, an->sel);
	       }
	     for (ll = range; ll; ll = eina_list_next(ll))
	       {
		  Evas_Object *ob;

		  sel = calloc(1, sizeof(Sel));
		  an->sel = eina_list_append(an->sel, sel);
		  ob = edje_object_add(en->rp->edje->evas);
		  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source5);
		  evas_object_smart_member_add(ob, smart);
		  evas_object_stack_below(ob, o);
		  evas_object_clip_set(ob, clip);
		  evas_object_pass_events_set(ob, 1);
		  evas_object_show(ob);
		  sel->obj_bg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_bg);
                  
		  ob = edje_object_add(en->rp->edje->evas);
		  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source6);
		  evas_object_smart_member_add(ob, smart);
		  evas_object_stack_above(ob, o);
		  evas_object_clip_set(ob, clip);
		  evas_object_pass_events_set(ob, 1);
		  evas_object_show(ob);
		  sel->obj_fg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_fg);
                  
		  ob = evas_object_rectangle_add(en->rp->edje->evas);
		  evas_object_color_set(ob, 0, 0, 0, 0);
		  evas_object_smart_member_add(ob, smart);
		  evas_object_stack_above(ob, o);
		  evas_object_clip_set(ob, clip);
		  evas_object_repeat_events_set(ob, 1);
		  evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_DOWN, _edje_anchor_mouse_down_cb, an);
		  evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_UP, _edje_anchor_mouse_up_cb, an);
		  evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_MOVE, _edje_anchor_mouse_move_cb, an);
		  evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_IN, _edje_anchor_mouse_in_cb, an);
		  evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_OUT, _edje_anchor_mouse_out_cb, an);
		  evas_object_show(ob);
		  sel->obj = ob;
	       }
	  }
	EINA_LIST_FOREACH(an->sel, ll, sel)
          {
	     Evas_Textblock_Rectangle *r;

	     r = range->data;
	     *(&(sel->rect)) = *r;
	     if (sel->obj_bg)
	       {
		  evas_object_move(sel->obj_bg, x + r->x, y + r->y);
		  evas_object_resize(sel->obj_bg, r->w, r->h);
	       }
	     if (sel->obj_fg)
	       {
		  evas_object_move(sel->obj_fg, x + r->x, y + r->y);
		  evas_object_resize(sel->obj_fg, r->w, r->h);
	       }
	     if (sel->obj)
	       {
		  evas_object_move(sel->obj, x + r->x, y + r->y);
		  evas_object_resize(sel->obj, r->w, r->h);
	       }
	     range = eina_list_remove_list(range, range);
	     free(r);
	  }
     }
}

static void
_anchors_clear(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   while (en->anchorlist)
     {
	free(en->anchorlist->data);
	en->anchorlist = eina_list_remove_list(en->anchorlist, en->anchorlist);
     }
   while (en->anchors)
     {
	Anchor *an = en->anchors->data;
	
	evas_textblock_cursor_free(an->start);
	evas_textblock_cursor_free(an->end);
	while (an->sel)
	  {
	     Sel *sel = an->sel->data;
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
	     if (sel->obj_bg) evas_object_del(sel->obj_bg);
	     if (sel->obj_fg) evas_object_del(sel->obj_fg);
	     if (sel->obj) evas_object_del(sel->obj);
	     free(sel);
	     an->sel = eina_list_remove_list(an->sel, an->sel);
	  }
	free(an);
	en->anchors = eina_list_remove_list(en->anchors, en->anchors);
     }
}

static void
_anchors_get(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Evas_Textblock_Cursor *c1;
   Anchor *an = NULL;
   int firsttext = 0;

   _anchors_clear(c, o, en);
   c1 = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_node_first(c1);
   do 
     {
	const char *s;
	
	s = evas_textblock_cursor_node_format_get(c1);
	if (s)
	  {
	     if ((!strncmp(s, "+ a ", 4)) || (!strncmp(s, "+a ", 3)))
	       {
		  an = calloc(1, sizeof(Anchor));
		  if (an)
		    {
		       char *p;
		       
		       an->en = en;
		       p = strstr(s, "href=");
		       if (p)
			 {
			    an->name = strdup(p + 5);
			 }
		       en->anchors = eina_list_append(en->anchors, an);
		       an->start = evas_object_textblock_cursor_new(o);
		       an->end = evas_object_textblock_cursor_new(o);
		       evas_textblock_cursor_copy(c1, an->start);
		       evas_textblock_cursor_copy(c1, an->end);
		    }
	       }
	     else if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
	       {
		  if (an)
		    {
		       if (!firsttext)
			 {
			    if (an->name) free(an->name);
			    evas_textblock_cursor_free(an->start);
			    evas_textblock_cursor_free(an->end);
			    en->anchors = eina_list_remove(en->anchors, an);
			    free(an);
			 }
		       firsttext = 0;
		       an = NULL;
		    }
	       }
	  }
	else
	  {
	     s = evas_textblock_cursor_node_text_get(c1);
	     if (an)
	       {
		  if (!firsttext)
		    {
		       evas_textblock_cursor_copy(c1, an->start);
		       firsttext = 1;
		    }
		  evas_textblock_cursor_char_last(c1);
		  evas_textblock_cursor_copy(c1, an->end);
	       }
	  }
     } 
   while (evas_textblock_cursor_node_next(c1));
   evas_textblock_cursor_free(c1);
}


static void
_range_del(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Evas_Textblock_Cursor *c1;
   
   c1 = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_node_last(c1);
   if (!evas_textblock_cursor_compare(en->sel_end, c1))
     evas_textblock_cursor_node_prev(en->sel_end);
   if (!evas_textblock_cursor_compare(en->sel_start, c1))
     evas_textblock_cursor_node_prev(en->sel_start);
   evas_textblock_cursor_free(c1);
   evas_textblock_cursor_range_delete(en->sel_start, en->sel_end);
}

static void
_backspace(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   Evas_Textblock_Cursor *c1, *c2;
   int nodel = 0;
   
   c1 = evas_object_textblock_cursor_new(o);
   if (!evas_textblock_cursor_char_prev(c))
     {
	if (!evas_textblock_cursor_node_prev(c))
	  nodel = 1;
	else
	  {
	     evas_textblock_cursor_copy(c, c1);
	     if (evas_textblock_cursor_node_format_get(c) &&
		 (!evas_textblock_cursor_node_format_is_visible_get(c)))
	       _curs_back(c, o, en);
	  }
     }
   else
     evas_textblock_cursor_copy(c, c1);
   c2 = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_copy(c, c2);
   if (!nodel) evas_textblock_cursor_range_delete(c1, c2);
   evas_textblock_cursor_copy(c, c1);
   _curs_back(c, o, en);
   if (evas_textblock_cursor_compare(c, c1))
     _curs_next(c, o, en);
   
   evas_textblock_cursor_free(c1);
   evas_textblock_cursor_free(c2);
}

static void
_edje_key_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje *ed = data;
   Evas_Event_Key_Down *ev = event_info;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Evas_Bool control, alt, shift;
   Evas_Bool multiline;
   Evas_Bool cursor_changed;
   Evas_Textblock_Cursor *tc;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;
   if (!ev->key) return;

#ifdef ECORE_IMF
#if 0 // FIXME -- keyboard activated IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Key_Down ecore_ev;
        ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_KEY_DOWN,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#endif
#endif
   
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   
   control = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
   multiline = rp->part->multiline;
   cursor_changed = 0;
   if (!strcmp(ev->key, "Escape"))
     {
	// dead keys here. Escape for now (should emit these)
        _edje_emit(ed, "entry,key,escape", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Up"))
     {
	if (multiline)
	  {
	     if (shift) _sel_start(en->cursor, rp->object, en);
	     else _sel_clear(en->cursor, rp->object, en);
	     _curs_up(en->cursor, rp->object, en);
	     if (shift) _sel_extend(en->cursor, rp->object, en);
	     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
	  }
        _edje_emit(ed, "entry,key,up", rp->part->name);
     }
   else if (!strcmp(ev->key, "Down"))
     {
	if (multiline)
	  {
	     if (shift) _sel_start(en->cursor, rp->object, en);
	     else _sel_clear(en->cursor, rp->object, en);
	     _curs_down(en->cursor, rp->object, en);
	     if (shift) _sel_extend(en->cursor, rp->object, en);
	     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
	  }
        _edje_emit(ed, "entry,key,down", rp->part->name);
     }
   else if (!strcmp(ev->key, "Left"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	_curs_back(en->cursor, rp->object, en);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,left", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Right"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	_curs_next(en->cursor, rp->object, en);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,right", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "BackSpace"))
     {
	if (control)
	  {
	     // del to start of previous word
	  }
	else if ((alt) && (shift))
	  {
	     // undo last action
	  }
	else
	  {
	     if (en->have_selection)
	       _range_del(en->cursor, rp->object, en);
	     else
	       _backspace(en->cursor, rp->object, en);
	  }
	_sel_clear(en->cursor, rp->object, en);
	_curs_update_from_curs(en->cursor, rp->object, en);
	_edje_emit(ed, "entry,changed", rp->part->name);
        _edje_emit(ed, "entry,key,backspace", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Delete"))
     {
	if (control)
	  {
	     // del to end of next word
	  }
	else if (shift)
	  {
	     // cut
	  }
	else
	  {	
	     if (en->have_selection)
	       _range_del(en->cursor, rp->object, en);
	     else
	       {
		  Evas_Textblock_Cursor *c1;
		  
		  c1 = evas_object_textblock_cursor_new(rp->object);
		  evas_textblock_cursor_copy(en->cursor, c1);
		  _curs_next(en->cursor, rp->object, en);
		  if (evas_textblock_cursor_compare(en->cursor, c1))
		    _backspace(en->cursor, rp->object, en);
		  evas_textblock_cursor_free(c1);
	       }
	  }
	_sel_clear(en->cursor, rp->object, en);
	_curs_update_from_curs(en->cursor, rp->object, en);
	_anchors_get(en->cursor, rp->object, en);
	_edje_emit(ed, "entry,changed", rp->part->name);
        _edje_emit(ed, "entry,key,delete", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Home"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	if ((control) && (multiline))
	  _curs_start(en->cursor, rp->object, en);
	else
	  _curs_lin_start(en->cursor, rp->object, en);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,home", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "End"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	if ((control) && (multiline))
	  _curs_end(en->cursor, rp->object, en);
	else
	  _curs_lin_end(en->cursor, rp->object, en);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,end", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "v")))
     {
	_edje_emit(ed, "entry,paste,request", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && ((!strcmp(ev->key, "c") || (!strcmp(ev->key, "Insert")))))
     {
	// FIXME: copy - save selection
	_edje_emit(ed, "entry,copy,notify", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && ((!strcmp(ev->key, "x") || (!strcmp(ev->key, "m")))))
     {
	// FIXME: cut - save selection, delete seletion
	_curs_update_from_curs(en->cursor, rp->object, en);
	_edje_emit(ed, "entry,cut,notify", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "z")))
     {
	if (shift)
	  {
	     // redo
	  }
	else
	  {
	     // undo
	  }
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "y")))
     {
	// redo
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "w")))
     {
	_sel_clear(en->cursor, rp->object, en);
	// select current word
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Tab"))
     {
	if (multiline)
	  {
	     if (shift)
	       {
		  // remove a tab
	       }
	     else
	       {
		  evas_textblock_cursor_format_prepend(en->cursor, "\t");
		  _curs_update_from_curs(en->cursor, rp->object, en);
		  _anchors_get(en->cursor, rp->object, en);
		  _edje_emit(ed, "entry,changed", rp->part->name);
	       }
	     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
	  }
        _edje_emit(ed, "entry,key,tab", rp->part->name);
     }
   else if ((!strcmp(ev->key, "ISO_Left_Tab")) && (multiline))
     { 
	// remove a tab
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Prior"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	_curs_jump_line_by(en->cursor, rp->object, en, -10);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,pgup", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Next"))
     {
	if (shift) _sel_start(en->cursor, rp->object, en);
	else _sel_clear(en->cursor, rp->object, en);
	_curs_jump_line_by(en->cursor, rp->object, en, 10);
	if (shift) _sel_extend(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,pgdn", rp->part->name);
	ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((!strcmp(ev->key, "Return")) || (!strcmp(ev->key, "KP_Enter")))
     {
	if (multiline)
	  {
	     evas_textblock_cursor_format_prepend(en->cursor, "\n");
	     _curs_update_from_curs(en->cursor, rp->object, en);
	     _anchors_get(en->cursor, rp->object, en);
	     _edje_emit(ed, "entry,changed", rp->part->name);
	     _edje_emit(ed, "cursor,changed", rp->part->name);
	     cursor_changed = 1;
	     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
	  }
        _edje_emit(ed, "entry,key,enter", rp->part->name);
     }
   else if ((!strcmp(ev->key, "Multi_key")))
     {
	// FIXME: compose next 2 keystrokes (Examples):
	// a " -> ä
	// o / -> ø
	// a e -> æ
	// e ' -> é
	// s s -> ß
	// etc.
     }
   // FIXME: other input methods? (xim, scim, uim etc.)...
   else
     {
	// FIXME: if composing.. store 2 keys
	if (ev->string)
	  {
            if (en->have_selection)
	       _range_del(en->cursor, rp->object, en);
	     evas_textblock_cursor_text_prepend(en->cursor, ev->string);
	     _sel_clear(en->cursor, rp->object, en);
	     _curs_update_from_curs(en->cursor, rp->object, en);
	     _anchors_get(en->cursor, rp->object, en);
	     _edje_emit(ed, "entry,changed", rp->part->name);
	     _edje_emit(ed, "cursor,changed", rp->part->name);
	     cursor_changed = 1;
	     ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
	  }
     }
   if ((evas_textblock_cursor_compare(tc, en->cursor)) && (!cursor_changed))
     _edje_emit(ed, "cursor,changed", rp->part->name);

#ifdef ECORE_IMF
   if (en->imf_context)
     {
	ecore_imf_context_reset(en->imf_context);
	ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif
   
   evas_textblock_cursor_free(tc);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_key_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;

#ifdef ECORE_IMF
#if 0 // FIXME key activation imf
   if (en->imf_context)
     {
        Ecore_IMF_Event_Key_Up ecore_ev;
        ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_KEY_UP,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#endif
#endif
   
}

static void
_edje_part_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Bool multiline;
   Evas_Textblock_Cursor *tc;
   Evas_Bool dosel = 0;
   if (!rp) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;
   if (ev->button == 2)
     {
	_edje_emit(rp->edje, "entry,paste,request", rp->part->name);
        return;
     }
   if (ev->button != 1) return;
   en->select_mod_start = 0;
   en->select_mod_end = 0;
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
     dosel = 1;
   else if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
     {
        if (en->select_allow) dosel = 1;
     }
   if (dosel)
     {
        // double click -> select word
        // triple click -> select line
     }
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   multiline = rp->part->multiline;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   en->cx = ev->canvas.x - x;
   en->cy = ev->canvas.y - y;
   if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
     {
        Evas_Coord lx, ly, lw, lh;
        int line;

        line = evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
        if (line == -1)
          _curs_end(en->cursor, rp->object, en);
        else
          {
             evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
             if (en->cx <= lx)
               _curs_lin_start(en->cursor, rp->object, en);
             else
               _curs_lin_end(en->cursor, rp->object, en);
          }
        line = evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
     }
   else
     {
        Evas_Coord lx, ly, lw, lh;
        int line;
        
        line = evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
     }
   if (dosel)
     {
        if ((en->have_selection) && 
            (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT))
          {
             Eina_List *first, *last;
             double sc;
             
             first = en->sel;
             last = eina_list_last(en->sel);
             if (first && last)
               {
                  Evas_Textblock_Rectangle *r1, *r2;
                  Evas_Coord d, d1, d2;
                  
                  r1 = first->data;
                  r2 = last->data;
                  d = r1->x - en->cx;
                  d1 = d * d;
                  d = (r1->y + (r1->h / 2)) - en->cy;
                  d1 += d * d;
                  d = r2->x + r2->w - 1 - en->cx;
                  d2 = d * d;
                  d = (r2->y + (r2->h / 2)) - en->cy;
                  d2 += d * d;
                  sc = rp->edje->scale;
                  if (sc == 0.0) sc = _edje_scale;
                  d = (Evas_Coord)(20.0 * sc); // FIXME: maxing number!
                  d = d * d;
                  if (d1 < d2)
                    {
                       if (d1 <= d)
                         {
                            en->select_mod_start = 1;
                            en->selecting = 1;
                         }
                    }
                  else
                    {
                       if (d2 <= d)
                         {
                            en->select_mod_end = 1;
                            en->selecting = 1;
                         }
                    }
               }
          }
        else
          {
             en->selecting = 1;
             _sel_clear(en->cursor, rp->object, en);
             _sel_start(en->cursor, rp->object, en);
          }
     }
   if (evas_textblock_cursor_compare(tc, en->cursor))
     _edje_emit(rp->edje, "cursor,changed", rp->part->name);

#ifdef ECORE_IMF
   if (en->imf_context)
     {
	ecore_imf_context_reset(en->imf_context);
	ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif
   
   evas_textblock_cursor_free(tc);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_part_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Bool multiline;
   Evas_Textblock_Cursor *tc;
   Evas_Bool dosel = 0;
   if (ev->button != 1) return;
   if (!rp) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   multiline = rp->part->multiline;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   en->cx = ev->canvas.x - x;
   en->cy = ev->canvas.y - y;
   if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
     {
        Evas_Coord lx, ly, lw, lh;
        
        evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
        evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
        if (en->cx <= lx)
          _curs_lin_start(en->cursor, rp->object, en);
        else
          _curs_lin_end(en->cursor, rp->object, en);
     }
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
     {  
        if (en->select_allow)
          {
             if (en->had_sel)
               {
                  if (en->select_mod_end)
                    _sel_extend(en->cursor, rp->object, en);
                  else if (en->select_mod_start)
                    _sel_preextend(en->cursor, rp->object, en);
               }
             else
               _sel_extend(en->cursor, rp->object, en);
//             evas_textblock_cursor_copy(en->cursor, en->sel_end);
          }
     }
   else
     evas_textblock_cursor_copy(en->cursor, en->sel_end);
   if (en->selecting)
     {
        if (en->have_selection)
          en->had_sel = 1;
        en->selecting = 0;
     }
   if (evas_textblock_cursor_compare(tc, en->cursor))
     _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   evas_textblock_cursor_free(tc);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_part_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Textblock_Cursor *tc;
   Evas_Bool multiline;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;
   if (en->selecting)
     {
        tc = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->cursor, tc);
        multiline = rp->part->multiline;
        evas_object_geometry_get(rp->object, &x, &y, &w, &h);
        en->cx = ev->cur.canvas.x - x;
        en->cy = ev->cur.canvas.y - y;
        if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
          {
             Evas_Coord lx, ly, lw, lh;
             
             evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
             evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
             if (en->cx <= lx)
               _curs_lin_start(en->cursor, rp->object, en);
             else
               _curs_lin_end(en->cursor, rp->object, en);
          }
        if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
          {
             if (en->select_allow)
               {
                  if (en->had_sel)
                    {
                       if (en->select_mod_end)
                         _sel_extend(en->cursor, rp->object, en);
                       else if (en->select_mod_start)
                         _sel_preextend(en->cursor, rp->object, en);
                    }
                  else
                    _sel_extend(en->cursor, rp->object, en);
               }
          }
        else
          {
             _sel_extend(en->cursor, rp->object, en);
          }
        if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) != 0)
          _sel_enable(en->cursor, rp->object, en);
        if (en->have_selection)
          _sel_update(en->cursor, rp->object, en);
        if (evas_textblock_cursor_compare(tc, en->cursor))
          _edje_emit(rp->edje, "cursor,changed", rp->part->name);
        evas_textblock_cursor_free(tc);
        _edje_entry_real_part_configure(rp);
     }
}

/***************************************************************/
void
_edje_entry_init(Edje *ed)
{
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_FOCUS_IN, _edje_focus_in_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_FOCUS_OUT, _edje_focus_out_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_KEY_DOWN, _edje_key_down_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_KEY_UP, _edje_key_up_cb, ed);
}

void
_edje_entry_shutdown(Edje *ed)
{
}

void
_edje_entry_real_part_init(Edje_Real_Part *rp)
{
   Entry *en;
#ifdef ECORE_IMF
   const char *ctx_id;
   const Ecore_IMF_Context_Info *ctx_info;
   Evas *evas;
#endif
   
   en = calloc(1, sizeof(Entry));
   if (!en) return;
   rp->entry_data = en;
   en->rp = rp;

   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_DOWN, _edje_part_mouse_down_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_UP, _edje_part_mouse_up_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_MOVE, _edje_part_mouse_move_cb, rp);
   
   if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
     {
	if ((rp->chosen_description) &&
	    (rp->chosen_description->text.repch))
	  evas_object_textblock_replace_char_set(rp->object, rp->chosen_description->text.repch);
	else
	  evas_object_textblock_replace_char_set(rp->object, "*");
     }

   en->cursor_bg = edje_object_add(rp->edje->evas);
   edje_object_file_set(en->cursor_bg, rp->edje->path, rp->part->source3);
   evas_object_smart_member_add(en->cursor_bg, rp->edje->obj);
   evas_object_stack_below(en->cursor_bg, rp->object);
   evas_object_clip_set(en->cursor_bg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_bg, 1);
   rp->edje->subobjs = eina_list_append(rp->edje->subobjs, en->cursor_bg);
   
   en->cursor_fg = edje_object_add(rp->edje->evas);
   edje_object_file_set(en->cursor_fg, rp->edje->path, rp->part->source4);
   evas_object_smart_member_add(en->cursor_fg, rp->edje->obj);
   evas_object_stack_above(en->cursor_fg, rp->object);
   evas_object_clip_set(en->cursor_fg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_fg, 1);
   rp->edje->subobjs = eina_list_append(rp->edje->subobjs, en->cursor_fg);
   
   if (rp->part->entry_mode >= EDJE_ENTRY_EDIT_MODE_EDITABLE)
     {
	evas_object_show(en->cursor_bg);
	evas_object_show(en->cursor_fg);
     }
   en->cursor = evas_object_textblock_cursor_get(rp->object);

#ifdef ECORE_IMF
   if (!ecore_imf_module_available_get())
     {
	en->imf_context = NULL;
	return;
     }

   edje_object_signal_callback_add(rp->edje->obj, "focus,part,in", rp->part->name, _edje_entry_focus_in_cb, rp);
   edje_object_signal_callback_add(rp->edje->obj, "focus,part,out", rp->part->name, _edje_entry_focus_out_cb, rp);
   
   ctx_id = ecore_imf_context_default_id_get();
   if (ctx_id)
     {
        ctx_info = ecore_imf_context_info_by_id_get(ctx_id);
        if (!ctx_info->canvas_type ||
            strcmp(ctx_info->canvas_type, "evas") == 0)
          {
             en->imf_context = ecore_imf_context_add(ctx_id);
          }
        
        else
          {
             ctx_id = ecore_imf_context_default_id_by_canvas_type_get("evas");
             if (ctx_id)
               {
                  en->imf_context = ecore_imf_context_add(ctx_id);
               }
          }
     }
   else
     en->imf_context = NULL;
   
   if (!en->imf_context) return;
   
   ecore_imf_context_client_window_set(en->imf_context, rp->object);
   ecore_imf_context_client_canvas_set(en->imf_context, rp->edje->evas);

   ecore_imf_context_retrieve_surrounding_callback_set(en->imf_context, _edje_entry_imf_retrieve_surrounding_cb, rp);
   en->imf_ee_handler_commit = ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _edje_entry_imf_event_commit_cb, rp->edje);     
   en->imf_ee_handler_delete = ecore_event_handler_add(ECORE_IMF_EVENT_DELETE_SURROUNDING, _edje_entry_imf_event_delete_surrounding_cb, rp);
   en->imf_ee_handler_changed = ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _edje_entry_imf_event_changed_cb, rp->edje);
   ecore_imf_context_input_mode_set(en->imf_context, 
                                    rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD ? 
                                    ECORE_IMF_INPUT_MODE_INVISIBLE : ECORE_IMF_INPUT_MODE_FULL);
#endif
}

void
_edje_entry_real_part_shutdown(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   rp->entry_data = NULL;
   _sel_clear(en->cursor, rp->object, en);
   _anchors_clear(en->cursor, rp->object, en);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_bg);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_fg);
   evas_object_del(en->cursor_bg);
   evas_object_del(en->cursor_fg);

#ifdef ECORE_IMF
   if (en->imf_context)
     {
	if (en->imf_ee_handler_commit)
	  {
	     ecore_event_handler_del(en->imf_ee_handler_commit);
	     en->imf_ee_handler_commit = NULL;
	  }
        
	if (en->imf_ee_handler_delete)
	  {
	     ecore_event_handler_del(en->imf_ee_handler_delete);
	     en->imf_ee_handler_delete = NULL;
	  }
	
	if (en->imf_ee_handler_changed) 
	  {
	     ecore_event_handler_del(en->imf_ee_handler_changed);
	     en->imf_ee_handler_changed = NULL;
	  }
	
	ecore_imf_context_del(en->imf_context);
	en->imf_context = NULL;
     }
#endif
   
   free(en);
}

void
_edje_entry_real_part_configure(Edje_Real_Part *rp)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en = rp->entry_data;
   if (!en) return;

   _sel_update(en->cursor, rp->object, en);
   _anchors_update(en->cursor, rp->object, en);
   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_char_geometry_get(en->cursor, &xx, &yy, &ww, &hh);
   if (ww < 1) ww = 1;
   if (hh < 1) ww = 1;
   if (en->cursor_bg)
     {
	evas_object_move(en->cursor_bg, x + xx, y + yy);
	evas_object_resize(en->cursor_bg, ww, hh);
     }
   if (en->cursor_fg)
     {
	evas_object_move(en->cursor_fg, x + xx, y + yy);
	evas_object_resize(en->cursor_fg, ww, hh);
     }
}

const char *
_edje_entry_selection_get(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return NULL;
   // get selection - convert to markup
   if ((!en->selection) && (en->have_selection))
     en->selection = evas_textblock_cursor_range_text_get
     (en->sel_start, en->sel_end, EVAS_TEXTBLOCK_TEXT_MARKUP);
   return en->selection;
}

const char *
_edje_entry_text_get(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return NULL;
   // get text - convert to markup
   return evas_object_textblock_text_markup_get(rp->object);
}

void
_edje_entry_text_markup_set(Edje_Real_Part *rp, const char *text)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   // set text as markup
   _sel_clear(en->cursor, rp->object, en);
   evas_object_textblock_text_markup_set(rp->object, text); 
   evas_textblock_cursor_node_last(en->cursor);
   if (!evas_textblock_cursor_node_format_get(en->cursor))
     {
	evas_textblock_cursor_format_append(en->cursor, "\n");
     }
   else if (!((!strcmp(evas_textblock_cursor_node_format_get(en->cursor), "\n")) ||
	      (!strcmp(evas_textblock_cursor_node_format_get(en->cursor), "\\n"))))
     {
	evas_textblock_cursor_format_append(en->cursor, "\n");
     }
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_entry_set_cursor_start(rp);
}

void
_edje_entry_text_markup_insert(Edje_Real_Part *rp, const char *text)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   // prepend markup @ cursor pos
   if (en->have_selection)
     evas_textblock_cursor_range_delete(en->sel_start, en->sel_end);
   evas_object_textblock_text_markup_prepend(en->cursor, text);
   _sel_clear(en->cursor, rp->object, en);
   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
}

void
_edje_entry_set_cursor_start(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _curs_start(en->cursor, rp->object, en);
}

void
_edje_entry_set_cursor_end(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _curs_end(en->cursor, rp->object, en);
}

void
_edje_entry_select_none(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
}

void
_edje_entry_select_all(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
   _curs_start(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);
   _curs_end(en->cursor, rp->object, en);
   _sel_extend(en->cursor, rp->object, en);
}

const Eina_List *
_edje_entry_anchor_geometry_get(Edje_Real_Part *rp, const char *anchor)
{
   Entry *en = rp->entry_data;
   Eina_List *l;
   Anchor *an;
   
   if (!en) return NULL;
   EINA_LIST_FOREACH(en->anchors, l, an)
     {
	if (!strcmp(anchor, an->name))
	  return an->sel;
     }
   return NULL;
}

const Eina_List *
_edje_entry_anchors_list(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   Eina_List *l, *anchors = NULL;
   Anchor *an;

   if (!en) return NULL;
   if (!en->anchorlist)
     {
        EINA_LIST_FOREACH(en->anchors, l, an)
	  {
	     const char *n = an->name;
	     if (!n) n = "";
	     anchors = eina_list_append(anchors, strdup(n));
	  }
	en->anchorlist = anchors;
     }
   return en->anchorlist;
}

void
_edje_entry_cursor_geometry_get(Edje_Real_Part *rp, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en = rp->entry_data;
   if (!en) return;

   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_char_geometry_get(en->cursor, &xx, &yy, &ww, &hh);
   if (ww < 1) ww = 1;
   if (hh < 1) ww = 1;
   if (cx) *cx = x + xx;
   if (cy) *cy = y + yy;
   if (cw) *cw = ww;
   if (ch) *ch = hh;
}

void
_edje_entry_select_allow_set(Edje_Real_Part *rp, Evas_Bool allow)
{
   Entry *en = rp->entry_data;
   en->select_allow = allow;
}

void
_edje_entry_select_abort(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (en->selecting)
     {
        en->selecting = 0;
        _edje_entry_real_part_configure(rp);
     }
}

#ifdef ECORE_IMF
static int
_edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos)
{
   Edje_Real_Part *rp = data;
   Entry *en;
   const char *str;
   
   if (!rp) return 0;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return 0;
   
   if (text)
     {
        str = _edje_entry_text_get(rp);
        *text = str ? strdup(str) : strdup("");
     }
   
   if (cursor_pos)
     {
        *cursor_pos = evas_textblock_cursor_pos_get(en->cursor);
     }
   
   return 1;
}

static int 
_edje_entry_imf_event_commit_cb(void *data, int type, void *event)
{
   Edje* ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Ecore_IMF_Event_Commit *ev = event;
   int cursor_pos, composition_pos;
   int start_pos, end_pos;
   Evas_Bool selecting;
   Evas_Bool changed = 0;
   int i;
   
   if (!rp) return 1;
   
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return 1;
   
   if (en->imf_context != ev->ctx) return 1;
   
   if (en->have_selection)
     {
	for (i = 0; i < en->comp_len; i++)
	  _backspace(en->cursor, rp->object, en);
	_sel_clear(en->cursor, rp->object, en);
     }

   evas_textblock_cursor_text_prepend(en->cursor, ev->str);

   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(ed, "cursor,changed", rp->part->name);
   
   return 0;
}

static int 
_edje_entry_imf_event_changed_cb(void *data, int type, void *event)
{
   Edje* ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   int cursor_pos;
   int composition_pos, length;
   int start_pos, end_pos;
   Evas_Bool selecting = 0;
   Evas_Bool changed = 0;
   Ecore_IMF_Event_Commit *ev = event;
   Evas_Textblock_Cursor *start_cur, *end_cur;
   int i;
   char *preedit_string;

   if (!rp) return 1;

   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return 1;
   
   if (!en->imf_context) return 1;
   
   if (en->imf_context != ev->ctx) return 1;

   ecore_imf_context_preedit_string_get(en->imf_context, &preedit_string, &length);

   // FIXME : check the maximum length of evas_textblock
   if ( 0 /* check the maximum length of evas_textblock */ )
     return 1;
   
   cursor_pos = evas_textblock_cursor_pos_get(en->cursor); 

   if (en->have_selection)
     {
	// delete the composing characters
	for (i = 0;i < en->comp_len; i++)
	  _backspace(en->cursor, rp->object, en);
     }

   en->comp_len = length;

   _sel_clear(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);

   evas_textblock_cursor_text_prepend(en->cursor, preedit_string);

   _sel_extend(en->cursor, rp->object, en);

   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(ed, "cursor,changed", rp->part->name);
   
   return 0;
}

static int
_edje_entry_imf_event_delete_surrounding_cb(void *data, int type, void *event)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Ecore_IMF_Event_Delete_Surrounding *ev = event;
   int cursor_pos;
   
   if (!rp) return 1;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return 1;
   
   if (en->imf_context != ev->ctx) return 1;
   
   cursor_pos = evas_textblock_cursor_pos_get(en->cursor);

   return 0;
}
#endif
