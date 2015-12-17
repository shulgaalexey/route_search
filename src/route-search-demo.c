#include "route-search-demo.h"
#include <maps_service.h>

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *label;
	maps_service_h maps; /* Handle of Maps Service */
} appdata_s;

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	   elm_conformant is mandatory for base gui to have proper size
	   when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Label */
	/* Create an actual view of the base gui.
	   Modify this part to change the view. */
	ad->label = elm_label_add(ad->conform);
	elm_object_text_set(ad->label, "<align=center>Hello Tizen</align>");
	evas_object_size_hint_weight_set(ad->label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_content_set(ad->conform, ad->label);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}

static bool
route_segment_maneuver_cb(int index, int total, maps_route_maneuver_h maneuver, void *user_data)
{
	char *route_info = (char *)user_data;
	char *instruction_text = NULL;
	const int max_line_len = 64;
	const int half_line_len = max_line_len / 2;
	char num[0x10] = {0};

	maps_route_maneuver_get_instruction_text(maneuver, &instruction_text);
	if(instruction_text && strlen(instruction_text)) {

		/* Add Instruction number */
		snprintf(num, 0x10, "</br> %d: ", index + 1);
		strcat(route_info, num);

		/* Add instruction text */
		const int l = strlen(instruction_text);
		if(l > max_line_len) { /* If the instruction is too long, lets extract the middle part */

			/* Add the first part of the instruction */
			char instruction_head[half_line_len + 1] = {0};
			snprintf(instruction_head, half_line_len - 3, "%s", instruction_text);
			strcat(route_info, instruction_head);

			/* Add first and second parts separator */
			strcat(route_info, "...");

			/* Add the second part of the instruction */
			char instruction_tail[half_line_len + 1] = {0};
			snprintf(instruction_tail, half_line_len, "%s", instruction_text + l - half_line_len);
			strcat(route_info, instruction_tail);
		} else
			/* Add whole instruction */
			strcat(route_info, instruction_text);

	}
	free(instruction_text);
	maps_route_maneuver_destroy(maneuver);
	return true;
}

static bool
route_segment_cb(int index, int total, maps_route_segment_h segment, void *user_data)
{
	maps_route_segment_foreach_maneuver	(segment, route_segment_maneuver_cb, user_data);
	maps_route_segment_destroy(segment);
	return true;
}


static bool
search_route_cb(maps_error_e error, int request_id, int index, int total,
		maps_route_h route, void* user_data)
{
	double distance = .0;
	long duration = 0;
	char route_info[0x1000] = {0};

	maps_route_get_total_distance(route, &distance);
	maps_route_get_total_duration(route, &duration);

	snprintf(route_info, 0x1000, "Route duration %.0f min, length %.3f km",
			ceil(1. * duration / 60), distance / 1000);

	/* Bonus: printing list of maneuvers to achieve the destination */
	maps_route_foreach_segment(route, route_segment_cb, route_info);

	appdata_s *ad = user_data;
	elm_object_text_set(ad->label, route_info);

	/* Don't forget to release the route handle */
   maps_route_destroy(route);

   /* If return true, we will receive other routes,
    * corresponding to our searching query parameters.
    * In this example, first route is enough for us. */
   return true;
}

static bool
app_create(void *data)
{
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	/* Specify Maps Provider name. */
	if(maps_service_create("HERE", &ad->maps) != MAPS_ERROR_NONE)
		return false;

	/* Set security key, issued by Maps Provider */
	maps_service_set_provider_key(ad->maps, "your-security-key");

	/* Set distance units */
	maps_preference_h preference = NULL;
	maps_preference_create(&preference);
	maps_preference_set_distance_unit(preference, MAPS_DISTANCE_UNIT_M);

	int request_id = 0;
	maps_coordinates_h origin = NULL, destination = NULL;
	maps_coordinates_create(50.0734902, 14.4279653, &origin);
	maps_coordinates_create(50.0860608,14.4145915, &destination);

	/* Use Route API */
	maps_service_search_route(ad->maps, origin, destination, preference, search_route_cb, ad, &request_id);

	maps_coordinates_destroy(origin);
	maps_coordinates_destroy(destination);
	maps_preference_destroy(preference);

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
	appdata_s *ad = data;
	maps_service_destroy(ad->maps);
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
