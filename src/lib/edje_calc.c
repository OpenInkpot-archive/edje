#include "Edje.h"
#include "edje_private.h"

static void _edje_part_recalc_single(Edje *ed, Edje_Real_Part *ep, Edje_Part_Description *desc, Edje_Part_Description *chosen_desc, Edje_Real_Part *rel1_to, Edje_Real_Part *rel2_to, Edje_Real_Part *confine_to, Edje_Calc_Params *params);
static void _edje_part_recalc(Edje *ed, Edje_Real_Part *ep);

void
_edje_part_pos_set(Edje *ed, Edje_Real_Part *ep, int mode, double pos)
{
   double npos;
   
   if (pos > 1.0) pos = 1.0;
   else if (pos < 0.0) pos = 0.0;
   npos = 0.0;
   /* take linear pos along timescale and use interpolation method */
   switch (mode)
     {
      case EDJE_TWEEN_MODE_SINUSOIDAL:
	npos = (1.0 - cos(pos * PI)) / 2.0;
	break;
      case EDJE_TWEEN_MODE_ACCELERATE:
	npos = 1.0 - sin((PI / 2.0) + (pos * PI / 2.0));
	break;
      case EDJE_TWEEN_MODE_DECELERATE:
	npos = sin(pos * PI / 2.0);
	break;
      case EDJE_TWEEN_MODE_LINEAR:
	npos = pos;
	break;
      default:
	break;
     }
   if (npos == ep->description_pos) return;

   ep->description_pos = npos;
   
   ed->dirty = 1;
   ep->dirty = 1;
}

void
_edje_part_description_apply(Edje *ed, Edje_Real_Part *ep, char  *d1, double v1, char *d2, double v2)
{
   char *cd1 = "default", *cd2 = "default";
   double cv1 = 0.0, cv2 = 0.0;
   
   if (!d1) d1 = "default";
   if (!d2) d2 = "default";

   if (!strcmp(d1, "default") && (v1 == 0.0))
     ep->param1.description = ep->part->default_desc;
   else
     {
	Evas_List *l;
	double min_dst;
	Edje_Part_Description *desc_found;

	desc_found = NULL;
	min_dst = 999.0;
	if (!strcmp("default", d1))
	  {
	     desc_found = ep->part->default_desc;
	     min_dst = ep->part->default_desc->state.value - v1;
	     if (min_dst < 0) min_dst = -min_dst;
	  }
	for (l = ep->part->other_desc; l; l = l->next)
	  {
	     Edje_Part_Description *desc;
	     
	     desc = l->data;
	     if (!strcmp(desc->state.name, d1))
	       {
		  double dst;
		  
		  dst = desc->state.value - v1;
		  if (dst == 0.0)
		    {
		       desc_found = desc;
		       break;
		    }
		  if (dst < 0.0) dst = -dst;
		  if (dst < min_dst)
		    {
		       desc_found = desc;
		       min_dst = dst;
		    }
	       }
	  }
	ep->param1.description = desc_found;
     }
   ep->param1.rel1_to = NULL;
   ep->param1.rel2_to = NULL;
   if (ep->param1.description)
     {
	if (ep->param1.description->rel1.id >= 0)
	  ep->param1.rel1_to = evas_list_nth(ed->parts, ep->param1.description->rel1.id);
	if (ep->param1.description->rel2.id >= 0)
	  ep->param1.rel2_to = evas_list_nth(ed->parts, ep->param1.description->rel2.id);
     }
   
   if (!strcmp(d2, "default") && (v2 == 0.0))
     ep->param2.description = ep->part->default_desc;
   else
     {
	Evas_List *l;
	double min_dst;
	Edje_Part_Description *desc_found;
	
	desc_found = NULL;
	min_dst = 999.0;
	if (!strcmp("default", d2))
	  {
	     desc_found = ep->part->default_desc;
	     min_dst = ep->part->default_desc->state.value - v2;
	     if (min_dst < 0) min_dst = -min_dst;
	  }
	for (l = ep->part->other_desc; l; l = l->next)
	  {
	     Edje_Part_Description *desc;
	     
	     desc = l->data;
	     if (!strcmp(desc->state.name, d2))
	       {
		  double dst;
		  
		  dst = desc->state.value - v2;
		  if (dst == 0.0)
		    {
		       desc_found = desc;
		       break;
		    }
		  if (dst < 0.0) dst = -dst;
		  if (dst < min_dst)
		    {
		       desc_found = desc;
		       min_dst = dst;
		    }
	       }
	  }
	ep->param2.description = desc_found;
     }
   if (!ep->param1.description)
     ep->param1.description = ep->part->default_desc;
   ep->param1.rel1_to = NULL;
   ep->param1.rel2_to = NULL;
   if (ep->param1.description)
     {
	if (ep->param1.description->rel1.id >= 0)
	  ep->param1.rel1_to = evas_list_nth(ed->parts, ep->param1.description->rel1.id);
	if (ep->param1.description->rel2.id >= 0)
	  ep->param1.rel2_to = evas_list_nth(ed->parts, ep->param1.description->rel2.id);
     }
   ep->param2.rel1_to = NULL;
   ep->param2.rel2_to = NULL;
   if (ep->param2.description)
     {
	if (ep->param2.description->rel1.id >= 0)
	  ep->param2.rel1_to = evas_list_nth(ed->parts, ep->param2.description->rel1.id);
	if (ep->param2.description->rel2.id >= 0)
	  ep->param2.rel2_to = evas_list_nth(ed->parts, ep->param2.description->rel2.id);
     }
   
   ed->dirty = 1;
   ep->dirty = 1;
}

void
_edje_recalc(Edje *ed)
{
   Evas_List *l;
   
   if (!ed->dirty) return;
   if (ed->freeze)
     {
	ed->recalc = 1;
	return;
     }
   for (l = ed->parts; l; l = l->next)
     {
	Edje_Real_Part *ep;
	
	ep = l->data;
	ep->calculated = 0;
     }
   for (l = ed->parts; l; l = l->next)
     {
	Edje_Real_Part *ep;
	
	ep = l->data;
	if (!ep->calculated) _edje_part_recalc(ed, ep);
     }
   ed->dirty = 0;
   ed->recalc = 0;
}

static void
_edje_part_recalc_single(Edje *ed,
			 Edje_Real_Part *ep, 
			 Edje_Part_Description *desc, 
			 Edje_Part_Description *chosen_desc,
			 Edje_Real_Part *rel1_to, 
			 Edje_Real_Part *rel2_to, 
			 Edje_Real_Part *confine_to,
			 Edje_Calc_Params *params)
{
   int minw, minh;

   /* relative coords of top left & bottom right */
   if (rel1_to)
     {
	params->x = desc->rel1.offset_x +
	  rel1_to->x + (desc->rel1.relative_x * rel1_to->w);
	params->y = desc->rel1.offset_y +
	  rel1_to->y + (desc->rel1.relative_y * rel1_to->h);
     }
   else
     {
	params->x = desc->rel1.offset_x +
	  (desc->rel1.relative_x * ed->w);
	params->y = desc->rel1.offset_y +
	  (desc->rel1.relative_y * ed->h);
     }
   if (rel2_to)
     {
	params->w = desc->rel2.offset_x +
	  rel2_to->x + (desc->rel2.relative_x * rel2_to->w) -
	  params->x;
	params->h = desc->rel2.offset_y +
	  rel2_to->y + (desc->rel2.relative_y * rel2_to->h) -
	  params->y;
     }
   else
     {
	params->w = (double)desc->rel2.offset_x +
	  (desc->rel2.relative_x * (double)ed->w) -
	  params->x + 1;
	params->h = (double)desc->rel2.offset_y +
	  (desc->rel2.relative_y * (double)ed->h) -
	  params->y + 1;
     }   

   /* aspect */
   if (params->h > 0)
     {
	double aspect;
	double new_w, new_h;
   
	new_h = params->h;
	new_w = params->w;
	aspect = (double)params->w / (double)params->h;
	/* adjust for max aspect (width / height) */
	if ((desc->aspect.max > 0.0) && (aspect > desc->aspect.max))
	  {
	     new_h = (params->w / desc->aspect.max);
	     new_w = (params->h * desc->aspect.max);
	  }
	/* adjust for min aspect (width / height) */
	if ((desc->aspect.min > 0.0) && (aspect < desc->aspect.min))
	  {
	     new_h = (params->w / desc->aspect.min);
	     new_w = (params->w * desc->aspect.min);
	  }
	/* do real adjustment */
	if ((params->h - new_h) > (params->w - new_w))
	  {
	     if (params->h < new_h)
	       {
		  params->y = params->y +
		    ((params->h - new_h) * (1.0 - desc->align.y));
		  params->h = new_h;
	       }
	     else if (params->h > new_h)
	       {
		  params->y = params->y +
		    ((params->h - new_h) * desc->align.y);
		  params->h = new_h;
	       }
	  }
	else
	  {
	     if (params->w < new_w)
	       {
		  params->x = params->x +
		    ((params->w - new_w) * (1.0 - desc->align.x));
		  params->w = new_w;
	       }
	     else if (params->w > new_w)
	       {
		  params->x = params->x +
		    ((params->w - new_w) * desc->align.x);
		  params->w = new_w;
	       }
	  }
     }

   /* size step */
   if (desc->step.x > 0)
     {
	int steps;
	int new_w;
	
	steps = params->w / desc->step.x;
	new_w = desc->step.x * steps;
	if (params->w > new_w)
	  {
	     params->x = params->x +
	       ((params->w - new_w) * desc->align.x);
	     params->w = new_w;
	  }	
     }
   if (desc->step.y > 0)
     {
	int steps;
	int new_h;
	
	steps = params->h / desc->step.y;
	new_h = desc->step.y * steps;
	if (params->h > new_h)
	  {
	     params->y = params->y +
	       ((params->h - new_h) * desc->align.y);
	     params->h = new_h;
	  }	
     }
   minw = desc->min.w;
   minh = desc->min.h;
   /* if we have text that wants to make the min size the text size... */
   if ((chosen_desc) && (ep->part->type == EDJE_PART_TYPE_TEXT))
     {
	char   *text;
	char   *font;
	int     size;
	double  tw, th;
	
	text = chosen_desc->text.text;
	font = chosen_desc->text.font;
	size = chosen_desc->text.size;
	if (ep->text.text) text = ep->text.text;
	if (ep->text.font) font = ep->text.font;
	if (ep->text.size) size = ep->text.size;
	evas_object_text_font_set(ep->object, font, size);
	if ((chosen_desc->text.min_x) || (chosen_desc->text.min_y))
	  {
	     evas_object_text_text_set(ep->object, text);
	     evas_object_geometry_get(ep->object, NULL, NULL, &tw, &th);
	     if (chosen_desc->text.min_x)
	       {
		  minw = tw;
		  /* FIXME: account for effect */
		  /* for now just add 2 */
		  minw += 2;
	       }
	     if (chosen_desc->text.min_y)
	       {
		  minh = th;
		  /* FIXME: account for effect */
		  /* for now just add 2 */
		  minw += 2;
	       }
	  }
     }
   /* adjust for min size */
   if (minw >= 0)
     {
	if (params->w < minw)
	  {
	     params->x = params->x + 
	       ((params->w - minw) * (1.0 - desc->align.x));
	     params->w = minw;
	  }
     }
   if (minh >= 0)
     {
	if (params->h < minh)
	  {
	     params->y = params->y + 
	       ((params->h - minh) * (1.0 - desc->align.y));
	     params->h = minh;
	  }
     }
   /* adjust for max size */
   if (desc->max.w >= 0)
     {
	if (params->w > desc->max.w)
	  {
	     params->x = params->x + 
	       ((params->w - desc->max.w) * desc->align.x);
	     params->w = desc->max.w;
	  }
     }
   if (desc->max.h >= 0)
     {
	if (params->h > desc->max.h)
	  {
	     params->y = params->y + 
	       ((params->h - desc->max.h) * desc->align.y);
	     params->h = desc->max.h;
	  }
     }
   /* confine */
   if (confine_to)
     {
	int offset;
	int step;
	
	/* complex dragable params */
	offset = params->x + ep->drag.x - confine_to->x;
	if (desc->dragable.step_x > 0)
	  {
	     params->x = confine_to->x + 
	       ((offset / desc->dragable.step_x) * desc->dragable.step_x);
	  }
	else if (desc->dragable.count_x > 0)
	  {
	     step = (confine_to->w - params->w) / desc->dragable.count_x;
	     params->x = confine_to->x +
	       ((offset / step) * step);	       
	  }
	offset = params->y + ep->drag.y - confine_to->y;
	if (desc->dragable.step_y > 0)
	  {
	     params->y = confine_to->y + 
	       ((offset / desc->dragable.step_y) * desc->dragable.step_y);
	  }
	else if (desc->dragable.count_y > 0)
	  {
	     step = (confine_to->h - params->h) / desc->dragable.count_y;
	     params->y = confine_to->y +
	       ((offset / step) * step);	       
	  }
	/* limit to confine */
	if (params->x < confine_to->x)
	  {
	     params->x = confine_to->x;
	  }
	if ((params->x + params->w) > (confine_to->x + confine_to->w))
	  {
	     params->x = confine_to->w - params->w;
	  }
	if (params->y < confine_to->y)
	  {
	     params->y = confine_to->y;
	  }
	if ((params->y + params->h) > (confine_to->y + confine_to->h))
	  {
	     params->y = confine_to->h - params->y;
	  }
     }
   else
     {
	/* simple dragable params */
	params->x += ep->drag.x;
	params->y += ep->drag.y;
     }
   /* fill */
   params->fill.x = desc->fill.pos_abs_x + (params->w * desc->fill.pos_rel_x);
   params->fill.w = desc->fill.abs_x + (params->w * desc->fill.rel_x);
   params->fill.y = desc->fill.pos_abs_y + (params->h * desc->fill.pos_rel_y);
   params->fill.h = desc->fill.abs_y + (params->h * desc->fill.rel_y);
   /* colors */
   params->color.r = desc->color.r;
   params->color.g = desc->color.g;
   params->color.b = desc->color.b;
   params->color.a = desc->color.a;
   params->color2.r = desc->color2.r;
   params->color2.g = desc->color2.g;
   params->color2.b = desc->color2.b;
   params->color2.a = desc->color2.a;
   params->color3.r = desc->color3.r;
   params->color3.g = desc->color3.g;
   params->color3.b = desc->color3.b;
   params->color3.a = desc->color3.a;
   /* visible */
   params->visible = desc->visible;
   /* border */
   params->border.l = desc->border.l;
   params->border.r = desc->border.r;
   params->border.t = desc->border.t;
   params->border.b = desc->border.b;
}

static void
_edje_part_recalc(Edje *ed, Edje_Real_Part *ep)
{
   Edje_Calc_Params p1, p2, p3;
   Edje_Part_Description *chosen_desc;
   double pos = 0.0;
   
   if (ep->calculated) return;
   if (ep->param1.rel1_to)    _edje_part_recalc(ed, ep->param1.rel1_to);
   if (ep->param1.rel2_to)    _edje_part_recalc(ed, ep->param1.rel2_to);
   if (ep->param1.confine_to) _edje_part_recalc(ed, ep->param1.confine_to);
   if (ep->param2.rel1_to)    _edje_part_recalc(ed, ep->param2.rel1_to);
   if (ep->param2.rel2_to)    _edje_part_recalc(ed, ep->param2.rel2_to);
   if (ep->param2.confine_to) _edje_part_recalc(ed, ep->param2.confine_to);
   
   /* actually calculate now */
   if (ep->description_pos == 0.0)
     chosen_desc = ep->param1.description;
   else
     chosen_desc = ep->param2.description;
   
   if (ep->param1.description)
     _edje_part_recalc_single(ed, ep, ep->param1.description, chosen_desc, ep->param1.rel1_to, ep->param1.rel2_to, ep->param1.confine_to, &p1);
   if (ep->param2.description)
     {
	_edje_part_recalc_single(ed, ep, ep->param2.description, chosen_desc, ep->param2.rel1_to, ep->param2.rel2_to, ep->param2.confine_to, &p2);

	/* FIXME: pos isnt just linear - depends on tween method */
	pos = ep->description_pos;
	
	/* visible is special */
	if ((p1.visible) && (!p2.visible))
	  {
	     if (pos == 1.0)
	       p3.visible = 0;
	     else
	       p3.visible = 1;
	  }
	else if ((!p1.visible) && (p2.visible))
	  {
	     if (pos == 0.0)
	       p3.visible = 0;
	     else
	       p3.visible = 1;
	  }
	else
	  p3.visible = p1.visible;
	
	p3.x = (p1.x * (1.0 - pos)) + (p2.x * (pos));
	p3.y = (p1.y * (1.0 - pos)) + (p2.y * (pos));
	p3.w = (p1.w * (1.0 - pos)) + (p2.w * (pos));
	p3.h = (p1.h * (1.0 - pos)) + (p2.h * (pos));
	
	p3.fill.x = (p1.fill.x * (1.0 - pos)) + (p2.fill.x * (pos));
	p3.fill.y = (p1.fill.y * (1.0 - pos)) + (p2.fill.y * (pos));
	p3.fill.w = (p1.fill.w * (1.0 - pos)) + (p2.fill.w * (pos));
	p3.fill.h = (p1.fill.h * (1.0 - pos)) + (p2.fill.h * (pos));
	
	p3.color.r = (p1.color.r * (1.0 - pos)) + (p2.color.r * (pos));
	p3.color.g = (p1.color.g * (1.0 - pos)) + (p2.color.g * (pos));
	p3.color.b = (p1.color.b * (1.0 - pos)) + (p2.color.b * (pos));
	p3.color.a = (p1.color.a * (1.0 - pos)) + (p2.color.a * (pos));
	
	p3.color2.r = (p1.color2.r * (1.0 - pos)) + (p2.color2.r * (pos));
	p3.color2.g = (p1.color2.g * (1.0 - pos)) + (p2.color2.g * (pos));
	p3.color2.b = (p1.color2.b * (1.0 - pos)) + (p2.color2.b * (pos));
	p3.color2.a = (p1.color2.a * (1.0 - pos)) + (p2.color2.a * (pos));
	
	p3.color3.r = (p1.color3.r * (1.0 - pos)) + (p2.color3.r * (pos));
	p3.color3.g = (p1.color3.g * (1.0 - pos)) + (p2.color3.g * (pos));
	p3.color3.b = (p1.color3.b * (1.0 - pos)) + (p2.color3.b * (pos));
	p3.color3.a = (p1.color3.a * (1.0 - pos)) + (p2.color3.a * (pos));
	
	p3.border.l = (p1.border.l * (1.0 - pos)) + (p2.border.l * (pos));
	p3.border.r = (p1.border.r * (1.0 - pos)) + (p2.border.r * (pos));
	p3.border.t = (p1.border.t * (1.0 - pos)) + (p2.border.t * (pos));
	p3.border.b = (p1.border.b * (1.0 - pos)) + (p2.border.b * (pos));
     }
   else
     p3 = p1;
   if (ep->part->type == EDJE_PART_TYPE_RECTANGLE)
     {
	evas_object_move(ep->object, ed->x + p3.x, ed->y + p3.y);
	evas_object_resize(ep->object, p3.w, p3.h);
     }
   else if (ep->part->type == EDJE_PART_TYPE_TEXT)
     {
	/* FIXME: if text object calculate text now */
	/* FIXME: set other colors */
     }
   else if (ep->part->type == EDJE_PART_TYPE_IMAGE)
     {
	char buf[4096];
	int image_id;
	int image_count, image_num;

//	printf("loc %3.3f %3.3f %3.3fx%3.3f\n", p3.x, p3.y, p3.w, p3.h);
	evas_object_move(ep->object, ed->x + p3.x, ed->y + p3.y);
	evas_object_resize(ep->object, p3.w, p3.h);
	evas_object_image_fill_set(ep->object, p3.fill.x, p3.fill.y, p3.fill.w, p3.fill.h);
//	printf("fill %3.3f %3.3f %3.3fx%3.3f\n", p3.fill.x, p3.fill.y, p3.fill.w, p3.fill.h);

	evas_object_image_border_set(ep->object, p3.border.l, p3.border.r, p3.border.t, p3.border.b);
	image_id = ep->param1.description->image.id;
	image_count = 2;
	if (ep->param2.description)
	  image_count += evas_list_count(ep->param2.description->image.tween_list);
	image_num = (pos * ((double)image_count - 0.5));
	if (image_num > (image_count - 1))
	  image_num = image_count - 1;
	if (image_num == 0)
	  image_id = ep->param1.description->image.id;
	else if (image_num == (image_count - 1))
	  image_id = ep->param2.description->image.id;
	else
	  {
	     Edje_Part_Image_Id *imid;
	     
	     imid = evas_list_nth(ep->param2.description->image.tween_list, image_num - 1);
	     if (imid) image_id = imid->id;
	  }

	snprintf(buf, sizeof(buf), "images/%i", image_id);
	evas_object_image_file_set(ep->object, ed->file->path, buf);
     }
   if (p3.visible) evas_object_show(ep->object);
   else evas_object_hide(ep->object);
   evas_object_color_set(ep->object, p3.color.r, p3.color.g, p3.color.b, p3.color.a);
   
   ep->x = p3.x;
   ep->y = p3.y;
   ep->w = p3.w;
   ep->h = p3.h;
   
   ep->calculated = 1;
   ep->dirty = 0;
}