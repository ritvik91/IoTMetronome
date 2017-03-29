#include "mbed.h"
#include "utils.hpp"
#include "m2mblockmessage.h"
#include <string>
#include <iostream>

#include "EthernetInterface.h"
#include "frdm_client.hpp"

#include "metronome.hpp"
Serial pc(USBTX, USBRX);

#define IOT_ENABLED

namespace active_low
{
	const bool on = false;
	const bool off = true;
}

DigitalOut g_led_red(LED1);
DigitalOut g_led_green(LED2);
DigitalOut g_led_blue(LED3);

InterruptIn g_button_mode(SW3);
InterruptIn g_button_tap(SW2);

int64_t min_bpm=1000; 
int64_t max_bpm=-1;
int bpm=1;
bool onMode=true;  //false->learn    true->play
int ptr=0;
time_t mbeats[5];
int seconds=1, size;
char b[20];

M2MResource *min_res, *max_res, *reset_res, *led_res, *pattern_res, *temp_res;
M2MObjectList objects;
M2MObjectInstance* led_inst;
M2MObject* object;
metronome met;

void resetMinMax(void* data)
{
   min_bpm = 1000;
   max_bpm = -1;
   min_res->set_value(min_bpm);
   max_res->set_value(max_bpm);
}


void on_tap()
{
   // Receive a tempo tap
   if(!onMode)
   {
		utils::pulse(g_led_red);
	    met.tap();
	}
}

void on_mode()
{
   // Change modes
   onMode = !onMode;
   if(!onMode)
   {
       met.start_timing();
       g_led_green = active_low::off;
	}
   else
   {
   	met.stop_timing();
   	bpm = met.get_bpm();
		
   	if(bpm<min_bpm)
   		min_bpm=bpm;
   	else if(bpm>max_bpm)
   		max_bpm=bpm;
   }
}

int main()
{	
	int rec=-1;
	
	// Seed the RNG for networking purposes
   unsigned seed = utils::entropy_seed();
   srand(seed);

	// LEDs are active LOW - true/1 means off, false/0 means on
	// Use the constants for easier reading
   g_led_red = active_low::off;	//do on
   g_led_green = active_low::off;	//do on
   g_led_blue = active_low::off;	//do on

	// Button falling edge is on push (rising is on release)
   g_button_mode.fall(&on_mode);
   g_button_tap.fall(&on_tap);

#ifdef IOT_ENABLED
	// Turn on the blue LED until connected to the network
   g_led_blue = active_low::off;

	// Need to be connected with Ethernet cable for success
   EthernetInterface ethernet;
   if (ethernet.connect() != 0)
       return 1;

	// Pair with the device connector
   frdm_client client("coap://api.connector.mbed.com:5684", &ethernet);
   if (client.get_state() == frdm_client::state::error)
       return 1;

	// The REST endpoints for this device
	// Add your own M2MObjects to this list with push_back before client.connect()

   object = M2MInterfaceFactory::create_object("3318");
   led_inst = object->create_object_instance();

   led_inst->set_operation(M2MBase::GET_PUT_POST_ALLOWED);
	led_inst->set_register_uri(true);

   //Expose Units
   pattern_res = led_inst->create_dynamic_resource("5701", "Units",
       M2MResourceInstance::STRING, false);
   pattern_res->set_operation(M2MBase::GET_ALLOWED);
   char buffer[4] = "BPM";
   pattern_res->set_value((const uint8_t*)buffer, 4);

	//Expose Set Point Values
   led_res = led_inst->create_dynamic_resource("5900", "Set Point Value",
       M2MResourceInstance::INTEGER, true);
   led_res->set_operation(M2MBase::GET_PUT_ALLOWED);
	led_res->set_value(bpm);

   //Expose Min Measured Value
   min_res = led_inst->create_dynamic_resource("5601", "Min Measured Value",
       M2MResourceInstance::INTEGER, true);
   min_res->set_operation(M2MBase::GET_ALLOWED);
   min_res->set_value(min_bpm);

   //Expose Max Measured Value
   max_res = led_inst->create_dynamic_resource("5602", "Max Measured Value",
       M2MResourceInstance::INTEGER, true);
   max_res->set_operation(M2MBase::GET_ALLOWED);
   max_res->set_value(max_bpm);

   //Expose Reset Measured Values
   reset_res = led_inst->create_dynamic_resource("5605", "Reset",
       M2MResourceInstance::OPAQUE, false);
   // we allow executing a function here...
   reset_res->set_operation(M2MBase::POST_ALLOWED);
   // when a POST comes in, we want to execute the led_execute_callback
   reset_res->set_execute_function(execute_callback(&resetMinMax));
   // Completion of execute function can take a time, that's why delayed response is used

	objects.push_back(object);
   M2MDevice* device = frdm_client::make_device();
   objects.push_back(device);

	// Publish the RESTful endpoints
   client.connect(objects);

	// Connect complete; turn off blue LED forever
   g_led_blue = active_low::off;	//do on
#endif

   while (true)
   {
#ifdef IOT_ENABLED
       if (client.get_state() == frdm_client::state::error)
           break;
#endif
       // Insert any code that must be run continuously here
       if(onMode){
           g_led_green = !g_led_green;
           pc.printf("bpm=%d\n",bpm);

           //M2MObjectInstance* inst = object->object_instance();
	        //M2MResource* res = inst->resource("5900");
	        
	        if(bpm != rec)
	        {
	        	rec = bpm;
	        	led_res->set_value(bpm);
	        }
	    	min_res->set_value(min_bpm);
	    	max_res->set_value(max_bpm);
	
	        // values in mbed Client are all buffers, and we need a vector of int's
	        uint8_t* buffIn = NULL;
	        uint32_t sizeIn;
	        led_res->get_value(buffIn, sizeIn);
			std::string s((char*)buffIn, sizeIn);
	     	free(buffIn);
	     	
	     	
	     	
	     	
	     	   	bpm = 0;
		        for(int i=0; i<s.length(); i++)
		        	bpm = bpm*10 + (s.at(i)-'0');
		        if(bpm<min_bpm)
		        {
		    		min_bpm=bpm;
		    		min_res->set_value(min_bpm);
		    	}
		    	else if(bpm>max_bpm)
		    	{
		    		max_bpm=bpm;
		    		max_res->set_value(max_bpm);
		    	}
		    	led_res->set_value(bpm);
		    
           wait(60/bpm);
       }
       if(!onMode){
       	wait(2);
       }
   }

#ifdef IOT_ENABLED
   client.disconnect();
#endif

   return 1;
}
