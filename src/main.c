//////////////////////////////////////////////////////////////////// 
//
// PEBBLE binary circuit clock V0.1             03-2015
//
// based on the PCBbinary Watchface from 8a22a https://github.com/8a22a/PCBbinary
//
// bachground.png IDENTIFIKATOR: IMAGE_BACKGROUND_
//
////////////////////////////////////////////////////////////////////

//Seting
#define MAINSWITCH_DAY_NIGHT_AUTO 1   //0 = OFF, 1 = ON - Swith the day and night automatik on or off
 
#define SETTINGS_KEY 99
  
#include "pebble.h"

Window *window;

static GBitmap *background_image;
static BitmapLayer *background_layer;

static AppSync sync;
static uint8_t sync_buffer[256]; //128

static int valueRead, valueWritten;

typedef struct persist {
  int Mode;
  int Day_start;
  int Day_end;
  bool numbers;
	
	//remenber the old state	
	bool day;
	bool number_old;
} __attribute__((__packed__)) persist;

persist settings = {
  .Mode 			= 0,
  .Day_start 	= 8,
  .Day_end 		= 18,
	.numbers 		= true,
	.day 				= true,
	.number_old	= true
};

enum {
  KEY_MODE      = 0,
  KEY_DAY_START = 1,
  KEY_DAY_END   = 2,
	KEY_NUMBER		= 3
};

//black or white
bool day  = false; //false, true


Layer *led_layer;

//----------------------------------------- config

void change_background() {
  
  bool day = true; //day is default
  if(settings.Mode == 1){day = false;} //Mode night
  if(settings.Mode == 2){              //Mode auto
    
    time_t now = time(NULL);            //load the time
    struct tm *t = localtime(&now);
    int hour = t->tm_hour;
    
    if(hour < settings.Day_start || hour >= settings.Day_end){ // || hour >= settings.Day_end){ //detect the night
      day = false;
    }
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Hour: %d", hour);
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Day_start: %d", settings.Day_start);
  }
  
  if(day != settings.day || settings.numbers != settings.number_old){ //detect a change
    settings.day = day;    //safe the change
		settings.number_old = settings.numbers;
    
    //change the background
    //clear old
    gbitmap_destroy(background_image);

    //set new background
    if(settings.day) {  //day /black
			if(settings.number_old){
      	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_S);
			}else{
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_SB);
			}
    }
    else {			//night / white
			if(settings.number_old){
      	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_W);
			}else{
				background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_WB);
			}
    }  
  
    bitmap_layer_set_bitmap(background_layer, background_image);
    layer_mark_dirty(bitmap_layer_get_layer(background_layer));
  }  
}

//----------------------------------------- save - load settings

static void loadPersistentSettings() {  
  valueRead = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void savePersistentSettings() {
  valueWritten = persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

//------------------------------------------------------- Sync callback

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "KEY: %d", (int)key);
	
	switch (key) {
    case KEY_MODE:
    	if(strcmp(new_tuple->value->cstring, "day") == 0){settings.Mode = 0;} 
    	else if(strcmp(new_tuple->value->cstring, "night")==0){settings.Mode = 1;}
    	else if(strcmp(new_tuple->value->cstring, "auto")== 0){settings.Mode = 2;}
      break;
    case KEY_DAY_START:
      settings.Day_start = (int)new_tuple->value->int32;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Day_start Time: %d", (int)settings.Day_start);
    	break;
    case KEY_DAY_END:
      settings.Day_end = (int)new_tuple->value->int32;
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Day_end Time: %d", settings.Day_end);
    	break;
		case KEY_NUMBER:
			if(strcmp(new_tuple->value->cstring, "show") == 0){settings.numbers = true;}
			if(strcmp(new_tuple->value->cstring, "hide") == 0){settings.numbers = false;}
			//APP_LOG(APP_LOG_LEVEL_DEBUG, "show numbers: %d", settings.numbers);
			break;
  }

  change_background();
	savePersistentSettings();
}

//----------------------------------------- Clock

//draw the LEDs
GRect getRect(int row, int led) {
  if (row == 0)
    return GRect(17 + led * 24, 21, 13, 13);//GRect(4 + led * 40, 21, 18, 18);
  else
    return GRect(14 + led * 20, 135, 10, 10);
}

//Change the LED color
void color_led(GContext* ctx, int row, int led, bool on) {
  graphics_context_set_fill_color(ctx, settings.day == on ? GColorWhite : GColorBlack); //on
  graphics_fill_rect(ctx, getRect(row, led), 1, 0);
}

//binary calc minute
void led_layer_update_callback(Layer *me, GContext* ctx) {
  
  change_background();  //just do it in the hour trigger to save energy

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  unsigned short hour = t->tm_hour; //% 12
  //hour = hour ? hour : 12; // 0 as 12
  unsigned short min = t->tm_min;

  color_led(ctx, 0, 0, hour & 16);
	color_led(ctx, 0, 1, hour & 8);
  color_led(ctx, 0, 2, hour & 4);
  color_led(ctx, 0, 3, hour & 2);
  color_led(ctx, 0, 4, hour & 1);

  color_led(ctx, 1, 0, min & 32);
  color_led(ctx, 1, 1, min & 16);
  color_led(ctx, 1, 2, min & 8);
  color_led(ctx, 1, 3, min & 4);
  color_led(ctx, 1, 4, min & 2);
  color_led(ctx, 1, 5, min & 1);
}

//minute trigger
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(led_layer);
}

//hourtrigger
void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed) {
  change_background();
}

//--------------------------------------------- init

static void do_init(void) {

  /*
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start in Mode: %d", settings.Mode);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start in Day: %d", settings.day);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Start number_old: %d", settings.number_old);
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "end day: %d", settings.Day_end);
  */
  
  const int inbound_size = 256;//128;
  const int outbound_size = 256;//128;  
  app_message_open(inbound_size, outbound_size); 
  
  //load the persist config setting  
  loadPersistentSettings();  
	
	/*
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD Mode: %d", settings.Mode);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD Day: %d", settings.day);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "LOAD number_old: %d", settings.number_old);
  */
	
  window = window_create();
  
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  Layer *window_layer = window_get_root_layer(window);
	
  GRect frame = layer_get_frame(window_layer);
  
  //load the background

  
  //set new background
  if(settings.day) {  //day - black
		if(settings.number_old){
     	background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_S);
		}else{
			background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_SB);
		}
  }
  else {			//night - white
		if(settings.number_old){
      background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_W);			
		}else{
			background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_WB);
		}
  }  
	
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));		

  change_background();
  
  // Init the layer for the display
  led_layer = layer_create(frame);
  layer_set_update_proc(led_layer, &led_layer_update_callback);
  layer_add_child(window_layer, led_layer);

	//eingangsvariablen -> nicht mehr vergessen
  Tuplet initial_values[] = {
    TupletInteger(KEY_MODE, settings.Mode),
    TupletInteger(KEY_DAY_START, settings.Day_start),
    TupletInteger(KEY_DAY_END, settings.Day_end),
		TupletInteger(KEY_NUMBER, settings.numbers)
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values), sync_tuple_changed_callback, NULL, NULL);
  
  tick_timer_service_subscribe(HOUR_UNIT, &handle_hour_tick);
  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  
}

static void do_deinit(void) {
  
  savePersistentSettings();
  app_sync_deinit(&sync);
  
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  layer_destroy(led_layer);
  window_destroy(window);	 
}

int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}