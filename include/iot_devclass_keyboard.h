#ifndef IOT_DEVCLASS_KEYBOARD_H
#define IOT_DEVCLASS_KEYBOARD_H
//Contains interface to communicate using IOT_DEVCLASSID_KEYBOARD class

#include<stdint.h>
//#include<time.h>
#include<assert.h>
#include<ecb.h>

#ifdef __linux__
	#include <linux/input-event-codes.h>
	#define IOT_KEYBOARD_MAX_KEYCODE KEY_MAX
#else
	#define IOT_KEYBOARD_MAX_KEYCODE 255
#endif


struct iot_devifaceclassdata_keyboard : public iot_devifaceclassdata_iface {
	struct data_t { //format of interface class data (class attributes)
		uint32_t format; //version of format or magic code
		uint32_t max_keycode:10, //maximum possible key code. is not checked in match() method so is not used in templates (must be 0 in templates)
			is_pckbd:2; //flag that keyboard is normal PC keyboard with shift, ctrl, alt (when 1). value >1 is used in template to mean 'any value' of this prop
	};

	iot_devifaceclassdata_keyboard(void) : iot_devifaceclassdata_iface(IOT_DEVIFACECLASSID_KEYBOARD, "Keyboard") {
	}

	static void init_classdata(iot_devifaceclass_data* devclass, uint16_t max_keycode, uint8_t is_pckbd) {
		if(max_keycode > IOT_KEYBOARD_MAX_KEYCODE) max_keycode=IOT_KEYBOARD_MAX_KEYCODE;
		devclass->classid=IOT_DEVIFACECLASSID_KEYBOARD;
		*((data_t*)devclass->data)={
			.format=1,
			.max_keycode=max_keycode,
			.is_pckbd=is_pckbd
		};
	}
	const data_t* parse_classdata(const char* cls_data) const {
		const data_t* d=(const data_t*)cls_data;
		if(d->format!=1) return NULL;
		return d;
	}
private:
	virtual bool check_data(const char* cls_data) const { //actual check that data is good by format
		data_t* data=(data_t*)cls_data;
		return data->format==1;
	}
	virtual size_t print_data(const char* cls_data, char* buf, size_t bufsize) const { //actual class data printing function. it must return number of written bytes (without NUL)
		data_t* data=(data_t*)cls_data;
		int len=snprintf(buf, bufsize, "%s (maxcode=%u,is_pc=%s)",name,unsigned(data->max_keycode),!data->is_pckbd ? "no" : data->is_pckbd==1 ? "yes" : "any");
		return len>=int(bufsize) ? bufsize-1 : len;
	}
	virtual uint32_t get_d2c_maxmsgsize(const char* cls_data) const;
	virtual uint32_t get_c2d_maxmsgsize(const char* cls_data) const;
	virtual bool compare(const char* cls_data, const char* tmpl_data) const { //actual comparison function
		data_t* data=(data_t*)cls_data;
		data_t* tmpl=(data_t*)tmpl_data;
		return tmpl->is_pckbd>1 || tmpl->is_pckbd==data->is_pckbd;
	}
};

class iot_devifaceclass__keyboard_BASE {
public:
	enum req_t : uint8_t { //commands (requests) which driver can execute
		REQ_GET_STATE, //request to post EVENT_INIT_STATE with status of all keys. no 'data' is used
	};

	enum event_t : uint8_t { //events (or replies) which driver can send to client
		EVENT_SET_STATE, //reply to REQ_GET_STATE request. provides current state of all keys. data.init_state is used for data struct
		EVENT_KEYDOWN, //key was down
		EVENT_KEYUP, //key was up
		EVENT_KEYREPEAT, //key was autorepeated
	};

	struct msg { //use same message format for requests and events (but this is not mandatory. each event type can have own structure)
		union { //field is determined by usage context
			req_t req_code;
			event_t event_code;
		};
		uint8_t statesize; //number of items in state
		uint16_t key:15, //key code of depressed/released/repeated key for EVENT_KEYDOWN, EVENT_KEYUP, EVENT_KEYREPEAT
			was_drop:1; //flag that some messages were dropped before this one
		uint32_t state[]; //map of depressed keys for all consumer events. already includes result of current key action
	};
	const msg* parse_event(const void *data, uint32_t data_size) {
		uint32_t statesize=(attr->max_keycode / 32)+1;
		if(data_size != sizeof(msg)+statesize*sizeof(uint32_t)) return NULL;
		return static_cast<const msg*>(data);
	}
	static uint32_t get_maxmsgsize(uint16_t max_keycode) {
		return sizeof(msg)+((max_keycode / 32)+1)*sizeof(uint32_t);
	}
protected:
	const iot_devifaceclassdata_keyboard::data_t *attr;

	iot_devifaceclass__keyboard_BASE(const iot_devifaceclass_data *devclass) {
		const iot_devifaceclassdata_iface* iface=devclass->find_iface();
		assert(iface!=NULL);
		assert(iface->classid==IOT_DEVIFACECLASSID_KEYBOARD);
		attr=static_cast<const iot_devifaceclassdata_keyboard*>(iface)->parse_classdata(devclass->data);
		assert(attr!=NULL);
	}
};


class iot_devifaceclass__keyboard_DRV : public iot_devifaceclass__DRVBASE, public iot_devifaceclass__keyboard_BASE {

public:
	iot_devifaceclass__keyboard_DRV(const iot_devifaceclass_data *devclass) : iot_devifaceclass__DRVBASE(devclass),
																				iot_devifaceclass__keyboard_BASE(devclass) {
	}

	//outgoing events (from driver to client)
	int send_keydown(const iot_connid_t &connid, iot_device_driver_base *drv_inst, uint16_t keycode, uint32_t *state) {
		return send_event(connid, drv_inst, EVENT_KEYDOWN, keycode, state);
	}
	int send_keyup(const iot_connid_t &connid, iot_device_driver_base *drv_inst, uint16_t keycode, uint32_t *state) {
		return send_event(connid, drv_inst, EVENT_KEYUP, keycode, state);
	}
	int send_keyrepeat(const iot_connid_t &connid, iot_device_driver_base *drv_inst, uint16_t keycode, uint32_t *state) {
		return send_event(connid, drv_inst, EVENT_KEYREPEAT, keycode, state);
	}
	int send_set_state(const iot_connid_t &connid, iot_device_driver_base *drv_inst, uint32_t *state) {
		return send_event(connid, drv_inst, EVENT_SET_STATE, 0, state);
	}

private:
	int send_event(const iot_connid_t &connid, iot_device_driver_base *drv_inst, event_t event_code, uint16_t key_code, uint32_t *state) {
		uint32_t statesize=(attr->max_keycode / 32)+1;
		char buf[sizeof(msg)+statesize*sizeof(uint32_t)];
		msg* msgp=(msg*)buf;
		memset(msgp, 0, sizeof(buf));
		msgp->event_code = event_code;
		msgp->statesize = uint8_t(statesize);
		msgp->key = key_code;
		if(state) memcpy(msgp->state, state, statesize*sizeof(uint32_t));

		return send_client_msg(connid, drv_inst, buf, sizeof(buf));
	}
};

class iot_devifaceclass__keyboard_CL : public iot_devifaceclass__CLBASE, public iot_devifaceclass__keyboard_BASE {

public:
	iot_devifaceclass__keyboard_CL(const iot_devifaceclass_data *devclass) : iot_devifaceclass__CLBASE(devclass),
																				iot_devifaceclass__keyboard_BASE(devclass) {
	}

	int request_state(const iot_connid_t &connid, iot_driver_client_base *client_inst) {
		return send_request(connid, client_inst, REQ_GET_STATE);
	}

private:
	int send_request(const iot_connid_t &connid, iot_driver_client_base *client_inst, req_t req_code) {
		uint32_t statesize=(attr->max_keycode / 32)+1;
		char buf[sizeof(msg)+statesize*sizeof(uint32_t)];
		msg* msgp=(msg*)buf;
		memset(msgp, 0, sizeof(buf));
		msgp->req_code = req_code;
		msgp->statesize = uint8_t(statesize);
		msgp->key = 0;

		return send_driver_msg(connid, client_inst, buf, sizeof(buf));
	}

};


#endif //IOT_DEVCLASS_KEYBOARD_H
