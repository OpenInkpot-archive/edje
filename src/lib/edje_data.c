#include "Edje.h"
#include "edje_private.h"

Eet_Data_Descriptor *_edje_edd_edje_file = NULL;
Eet_Data_Descriptor *_edje_edd_edje_image_directory = NULL;
Eet_Data_Descriptor *_edje_edd_edje_image_directory_entry = NULL;
Eet_Data_Descriptor *_edje_edd_edje_program = NULL;
Eet_Data_Descriptor *_edje_edd_edje_program_target = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part_collection_directory = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part_collection_directory_entry = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part_collection = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part_description = NULL;
Eet_Data_Descriptor *_edje_edd_edje_part_image_id = NULL;

void
_edje_edd_setup(void)
{
   /* image directory */
   _edje_edd_edje_image_directory_entry = 
     eet_data_descriptor_new("Edje_Image_Directory_Entry",
			     sizeof(Edje_Image_Directory_Entry),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_type", source_type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_param", source_param, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "id", id, EET_T_INT);
   
   _edje_edd_edje_image_directory = 
     eet_data_descriptor_new("Edje_Image_Directory", 
			     sizeof(Edje_Image_Directory),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_image_directory, Edje_Image_Directory, "entries", entries, _edje_edd_edje_image_directory_entry);

   /* collection directory */
   _edje_edd_edje_part_collection_directory_entry = 
     eet_data_descriptor_new("Edje_Part_Collection_Directory_Entry",
			     sizeof(Edje_Part_Collection_Directory_Entry),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "id", id, EET_T_INT);
   
   _edje_edd_edje_part_collection_directory = 
     eet_data_descriptor_new("Edje_Part_Collection_Directory",
			     sizeof(Edje_Part_Collection_Directory),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_collection_directory, Edje_Part_Collection_Directory, "entries", entries, _edje_edd_edje_part_collection_directory_entry);
   
   /* the main file directory */
   _edje_edd_edje_file = 
     eet_data_descriptor_new("Edje_File", 
			     sizeof(Edje_File),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "image_dir", image_dir, _edje_edd_edje_image_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "collection_dir", collection_dir, _edje_edd_edje_part_collection_directory);   

   /* parts & programs - loaded induvidually */
   _edje_edd_edje_program_target = 
     eet_data_descriptor_new("Edje_Program_Target",
			     sizeof(Edje_Program_Target),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program_target, Edje_Program_Target, "id", id, EET_T_INT);
   
   _edje_edd_edje_program = 
     eet_data_descriptor_new("Edje_Program",
			     sizeof(Edje_Program),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "signal", signal, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "action", action, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state", state, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state2", state2, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "value", value, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.mode", tween.mode, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.time", tween.time, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_program, Edje_Program, "targets", targets, _edje_edd_edje_program_target);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "after", after, EET_T_INT);
   
   _edje_edd_edje_part_image_id = 
     eet_data_descriptor_new("Edje_Part_Image_Id",
			     sizeof(Edje_Part_Image_Id),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_image_id, Edje_Part_Image_Id, "id", id, EET_T_INT);
   
   _edje_edd_edje_part_description = 
     eet_data_descriptor_new("Edje_Part_Description",
			     sizeof(Edje_Part_Description),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "state.name", state.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "state.value", state.value, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "visible", visible, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.x", dragable.x, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.step_x", dragable.step_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.count_x", dragable.count_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.y", dragable.y, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.step_y", dragable.step_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.count_y", dragable.count_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "dragable.counfine_id", dragable.confine_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "align.x", align.x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "align.y", align.y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "min.w", min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "min.h", min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "max.w", max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "max.h", max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "step.x", step.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "step.y", step.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "aspect.min", aspect.min, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "aspect.max", aspect.max, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel1.relative_x", rel1.relative_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel1.relative_y", rel1.relative_y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel1.offset_x", rel1.offset_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel1.offset_y", rel1.offset_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel1.id", rel1.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel2.relative_x", rel2.relative_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel2.relative_y", rel2.relative_y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel2.offset_x", rel2.offset_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel2.offset_y", rel2.offset_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "rel2.id", rel2.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "image.id", image.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_description, Edje_Part_Description, "image.tween_list", image.tween_list, _edje_edd_edje_part_image_id);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "border.l", border.l, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "border.r", border.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "border.t", border.t, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "border.b", border.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.pos_rel_x", fill.pos_rel_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.pos_abs_x", fill.pos_abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.rel_x", fill.rel_x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.abs_x", fill.abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.pos_rel_y", fill.pos_rel_y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.pos_abs_y", fill.pos_abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.rel_y", fill.rel_y, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "fill.abs_y", fill.abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color.r", color.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color.g", color.g, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color.b", color.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color.a", color.a, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color2.r", color2.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color2.g", color2.g, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color2.b", color2.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color2.a", color2.a, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color3.r", color3.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color3.g", color3.g, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color3.b", color3.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "color3.a", color3.a, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.text", text.text, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.font", text.font, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.size", text.size, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.effect", text.effect, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.fit_x", text.fit_x, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.fit_y", text.fit_y, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.min_x", text.min_x, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.min_y", text.min_y, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.align.x", text.align.x, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description, Edje_Part_Description, "text.align.y", text.align.y, EET_T_DOUBLE);
   
   _edje_edd_edje_part = 
     eet_data_descriptor_new("Edje_Part",
			     sizeof(Edje_Part),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "type", type, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "mouse_events", mouse_events, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "clip_to_id", clip_to_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "color_class", color_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "text_class", text_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_part, Edje_Part, "default_desc", default_desc, _edje_edd_edje_part_description);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part, Edje_Part, "other_desc", other_desc, _edje_edd_edje_part_description);
   
   _edje_edd_edje_part_collection  = 
     eet_data_descriptor_new("Edje_Part_Collection", 
			     sizeof(Edje_Part_Collection),
			     evas_list_next,
			     evas_list_append,
			     evas_list_data,
			     evas_hash_foreach,
			     evas_hash_add);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs", programs, _edje_edd_edje_program);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_collection, Edje_Part_Collection, "parts", parts, _edje_edd_edje_part);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "id", id, EET_T_INT);
}