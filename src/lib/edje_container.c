#include "Edje.h"
#include "edje_private.h"

/* All items are virtual constructs that provide Evas_Objects at some point.
 * Edje may move, resize, show, hide, clip, unclip, raise, lower etc. this
 * item AFTER it calls the item's add() method and before it calls the del()
 * method. Edje may call add() and del() at any time as often items may not
 * be visible and so may not need to exist at all - they are merely information
 * used for layout, and nothing more. this helps save cpu and memory keeping
 * things responsive for BIG lists of items. you create an item from an item
 * class then ask that item to be appended/prepended etc. to the container.
 */
typedef struct _Edje_Item Edje_Item;
typedef struct _Edje_Item_Class Edje_Item_Class;

struct _Edje_Item_Class
{
   Evas_Object *(*add)      (Edje_Item *ei);
   void         (*del)      (Edje_Item *ei);
   void         (*select)   (Edje_Item *ei);
   void         (*deselect) (Edje_Item *ei);
   void         (*focus)    (Edje_Item *ei);
   void         (*unfocus)  (Edje_Item *ei);
};

/* private */
struct _Edje_Item
{
   Edje_Item_Class *class;
   void            *class_data;
   
   unsigned char    accessible : 1;
   Evas_Object     *object;
   Evas_Object     *underlay_object;
};

/* here is an item for a vertical list - with 1 or more columns. this has 3 */
/* just rotate for a horizontal list */

/*
 *             COL 0                 COL 1          COL 2
 * 
 * +-----------------------------+ +-------+ +----------------+
 * |          pad_top            | |       | |                |
 * | pad_left  OBJECT  pad_right | |  OBJ  | |     OBJECT     | ROW 0
 * |         pad_bottom          | |       | |                |
 * +-----------------------------+ +-------+ +----------------+
 *               /\              /|\
 *     space_row ||               +-- space_col
 *               \/
 * +-----------------------------+ +-------+ +----------------+
 * |                             | |       | |                |
 * |                             | |       | |                | ROW 1
 * |                             | |       | |                |
 * +-----------------------------+ +-------+ +----------------+
 * 
 * spacer object:
 * 1 edje object goes inbetween each row as a spacer object (opt)
 * 1 edje object goes inbetween each column as a spacer object (opt)
 * 
 * surround object:
 * 1 edje object goes around each item - item swallowed into "item" part (opt)
 *   if no "item" part then just underlay it
 *   on select send "select" "" signal
 *   on unselect send "unselect" "" signal
 *   on focus send "focus" "" signal
 *   on unfocus send "unfocus" signal
 * 
 *   if any list item/cell is an edje object emit this to them too.
 * 
 *   also call callbacks.
 *   if a surround object emits such a signal itself then call callbacks too
 * 
 * only 1 or 0 items can be focused
 * disabled items cannot be focused or selected/deselected
 * 
 * container accepts:
 * { theme sets these effectively }
 * set edje it is part of
 * set collection id for col spacer
 * set collection id for row spacer
 * set row space
 * set col space
 * set item padding
 * set collection id for surround object
 * 
 * { theme and app can both do this. theme has to do via embryo }
 * clear list
 * append item
 * prepend item
 * insert item before item
 * insert item after item
 * get item count
 * get first item
 * get last item
 * get item N
 * get item before item
 * get item after item
 * select item
 * unselect item
 * unselect all items
 * select all items
 * get selected item list
 * focus item
 * unfocus item
 * focus next item
 * focus prev item
 * get focused item
 * enable item
 * disable item
 * get item pos (along list) (0.0 - 1.0 1.0 = end of list)
 * get item span (0.0 - 1.0 1.0 == whole list height)
 * jump to pos
 * get list min width
 * get list min height
 * get view percentage
 * 
 * notes:
 * 
 * dnd of list items within lthe list and outside of it ???
 * 
 */

/* create and destroy virtual items */

Edje_Item *
edje_item_add(Edje_Item_Class *cl, void *data)
{
}

void
edje_item_del(Edje_Item *ei)
{
}

/* an arbitary data pointer to use to track other data */

void
edje_item_data_set(Edje_Item *ei, void *data)
{
}

void *
edje_item_data_get(Edje_Item *ei)
{
}

/* this object covers the entire item */
void
edje_item_overlay_object_set(Edje_Item *ei, Evas_Object *obj)
{
}

Evas_Object *
edje_item_overlay_object_get(Edje_Item *ei)
{
}

/* this object goes under entire item */
void
edje_item_object_set(Edje_Item *ei, Evas_Object *obj)
{
}

Evas_Object *
edje_item_object_get(Edje_Item *ei)
{
}

/* optionally you can manage each column's object yourself OR let edje do it */
void
edje_item_object_column_set(Edje_Item *ei, int col, Evas_Object *obj)
{
}

Evas_Object *
edje_item_object_column_get(Edje_Item *ei, int col)
{
}

/* query the item for the items preferred co-ords */
void
edje_tiem_geometry_get(Edje_Item *ei, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
}

/* freeze and thaw items if u are about to do a bunch of changes */
int
edje_item_freeze(Edje_Item *ei)
{
}

int
edje_item_thaw(Edje_Item *ei)
{
}

/* column info */
void
edje_item_column_size_set(Edje_Item *ei, int col, Evas_Coord minw, Evas_Coord maxw, Evas_Coord minh, Evas_Coord maxh)
{
}

void
edje_item_column_size_get(Edje_Item *ei, int col, Evas_Coord *minw, Evas_Coord *maxw, Evas_Coord *minh, Evas_Coord *maxh)
{
}

/* selection stuff */
void
edje_item_select(Edje_Item *ei)
{
}

void
edje_item_unselect(Edje_Item *ei)
{
}

/* focus stuff - only 1 can be focuesd */
void
edje_item_focus(Edje_Item *ei)
{
}

void
edje_item_unfocus(Edje_Item *ei)
{
}

/* disable/enable stuff - stops focus and selection working on these items */
void
edje_item_enable(Edje_Item *ei)
{
}

void
edje_item_disable(Edje_Item *ei)
{
}

/* item utils */
Edje_Item *
edje_item_next_get(Edje_Item *ei)
{
}

Edje_Item *
edje_item_prev_get(Edje_Item *ei)
{
}

int
edje_item_selected_get(Edje_Item *ei)
{
}

int
edje_item_focused_get(Edje_Item *ei)
{
}

int
edje_item_disabled_get(Edje_Item *ei)
{
}

void
edje_item_geometry_get(Edje_Item *ei, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
}

double
edje_item_position_get(Edje_Item *ei)
{
}

void
edje_item_offset_set(Edje_Item *ei, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
}

/***** container calls *****/

void
edje_container_columns_set(Evas_Object *obj, int cols)
{
}

int
edje_container_columns_get(Evas_Object *obj)
{
}

void
edje_container_min_size_get(Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh)
{
}

void
edje_container_max_size_get(Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh)
{
}

int
edje_container_count_get(Evas_Object *obj)
{
}

Edje_Item *
edje_container_item_first_get(Evas_Object *obj)
{
}

Edje_Item *
edje_container_item_last_get(Evas_Object *obj)
{
}

Edje_Item *
edje_container_item_n_get(Evas_Object *obj, int n)
{
}

void
edje_container_homogenous_size_set(Evas_Object *obj, int homog)
{
}

int
edje_container_homogenous_size_get(Evas_Object *obj)
{
}

void
edje_container_orientation_set(Evas_Object *obj, int orient)
{
}

int
edje_container_orientation_get(Evas_Object *obj)
{
}

void
edje_container_scroll_set(Evas_Object *obj, double pos, double shift)
{
}

void
edje_container_scroll_get(Evas_Object *obj, double *pos, double *shift)
{
}

#define E_SMART_OBJ_GET(smart, o, type) \
     { \
	char *_e_smart_str; \
	\
	if (!o) return; \
	smart = evas_object_smart_data_get(o); \
	if (!smart) return; \
	_e_smart_str = (char *)evas_object_type_get(o); \
	if (!_e_smart_str) return; \
	if (strcmp(_e_smart_str, type)) return; \
     }

#define E_SMART_OBJ_GET_RETURN(smart, o, type, ret) \
   { \
      char *_e_smart_str; \
      \
      if (!o) return ret; \
      smart = evas_object_smart_data_get(o); \
      if (!smart) return ret; \
      _e_smart_str = (char *)evas_object_type_get(o); \
      if (!_e_smart_str) return ret; \
      if (strcmp(_e_smart_str, type)) return ret; \
   }

#define E_OBJ_NAME "edje_container_object"

typedef struct _Smart_Data   Smart_Data;
typedef struct _Smart_Item   Smart_Item;
typedef struct _Smart_Column Smart_Column;

struct _Smart_Data
{
   int layout;
   Evas_List *children;
   struct {
      int w, h;
   } min, max;
};

struct _Smart_Item
{
   Evas_Object *container;
   Evas_Object *obj;
};

struct _Smart_Column
{
   int min, max;
};

#define EDJE_LAYOUT_NONE  0

/* the following are "linear" layout systems */

/* H & V LIST pack all items at their minimum size - no expanding in the lists
 * direction (lets say height), BUT all items are expanded to fill the "width"
 * of the list. if an item is too small to fit the width, the list object will
 * call a "min_size_change" callback indicating to the parent/swallower that
 * the parent should revise its use/view of this object. this is intended for
 * large lists of items - like a list of files, or titles etc.  this allows
 * for each item to have multiple columns. each column may be any size, with
 * the minimu size being the sum of all minimum column widths. as more items
 * are added column widths may be adjusted and all items told of this
 * adjustment
 */
#define EDJE_LAYOUT_VLIST 1
#define EDJE_LAYOUT_HLIST 2
/* H & V BOX pack items and may or may not expand an item in any direction and
 * may align an item smaller than its allocated space in a certain way. they
 * dont know about columns etc. like lists.
 */
#define EDJE_LAYOUT_VBOX  3
#define EDJE_LAYOUT_HBOX  4
/* H & V flow are like "file manager" views you see in explorer etc. wehere
 * items "line wrap" as they go along horizontally (or vertizally) as needed
 */
#define EDJE_LAYOUT_VFLOW 5
#define EDJE_LAYOUT_HFLOW 6

/* the following are "2 dimensional" layout systems */

/* tables are full 2-dimensional layouts which dont really have append and
 * prepend semantics... this will need working on later for 2d layouts. dont
 * worry about these yet - but keep this as a marker for things to do later
 */
#define EDJE_LAYOUT_TABLE 7
/* count
 */
#define EDJE_LAYOUT_COUNT 8

static void _smart_init(void);
static void _smart_add(Evas_Object * obj);
static void _smart_del(Evas_Object * obj);
static void _smart_layer_set(Evas_Object * obj, int layer);
static void _smart_raise(Evas_Object * obj);
static void _smart_lower(Evas_Object * obj);
static void _smart_stack_above(Evas_Object * obj, Evas_Object * above);
static void _smart_stack_below(Evas_Object * obj, Evas_Object * below);
static void _smart_move(Evas_Object * obj, Evas_Coord x, Evas_Coord y);
static void _smart_resize(Evas_Object * obj, Evas_Coord w, Evas_Coord h);
static void _smart_show(Evas_Object * obj);
static void _smart_hide(Evas_Object * obj);
static void _smart_color_set(Evas_Object * obj, int r, int g, int b, int a);
static void _smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _smart_clip_unset(Evas_Object * obj);

static Evas_Smart  *smart = NULL;

Evas_Object *
edje_container_object_add(Evas *evas)
{
   _smart_init();
   return evas_object_smart_add(evas, smart);
}

/*******************************************/
/* Internal smart object required routines */
/*******************************************/
static void
_smart_init(void)
{
   if (smart) return;
   smart = evas_smart_new(E_OBJ_NAME,
			  _smart_add,
			  _smart_del,
			  _smart_layer_set,
			  _smart_raise,
			  _smart_lower,
			  _smart_stack_above,
			  _smart_stack_below,
			  _smart_move,
			  _smart_resize,
			  _smart_show,
			  _smart_hide,
			  _smart_color_set,
			  _smart_clip_set,
			  _smart_clip_unset,
			  NULL);
}

static void
_smart_add(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = calloc(1, sizeof(Smart_Data));
   if (!sd) return;
//   evas_object_smart_member_add(sd->obj, obj);
   evas_object_smart_data_set(obj, sd);
}

static void
_smart_del(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_del(sd->obj);
   free(sd);
}

static void
_smart_layer_set(Evas_Object *obj, int layer)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_layer_set(sd->obj, layer);
}
   
static void
_smart_raise(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_raise(sd->obj);
}

static void
_smart_lower(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_lower(sd->obj);
}
   
static void
_smart_stack_above(Evas_Object *obj, Evas_Object *above)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_stack_above(sd->obj, above);
}

static void
_smart_stack_below(Evas_Object *obj, Evas_Object *below)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_stack_below(sd->obj, below);
}

static void
_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_move(sd->obj, x, y);
}

static void
_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_resize(sd->obj, w, h);
}
   
static void
_smart_show(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_show(sd->obj);
}

static void
_smart_hide(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_hide(sd->obj);
}

static void
_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_color_set(sd->obj, r, g, b, a);
}

static void
_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_clip_set(sd->obj, clip);
}

static void
_smart_clip_unset(Evas_Object *obj)
{
   Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
//   evas_object_clip_unset(sd->obj);
}  