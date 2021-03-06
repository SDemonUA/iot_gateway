#include "iot_module.h"
#include "iot_devclass_toneplayer.h"


uint32_t iot_deviface_params_toneplayer::get_c2d_maxmsgsize(void) const {
	return iot_deviface__toneplayer_BASE::get_maxmsgsize();
}
uint32_t iot_deviface_params_toneplayer::get_d2c_maxmsgsize(void) const {
	return iot_deviface__toneplayer_BASE::get_maxmsgsize();
}

iot_devifacetype_metaclass_toneplayer iot_devifacetype_metaclass_toneplayer::object;
const iot_deviface_params_toneplayer iot_deviface_params_toneplayer::object;

/*void iot_devifacetype_toneplayer::init_classdata(iot_devifacetype* devclass) {
		devclass->classid=IOT_DEVIFACETYPEID_TONEPLAYER;
		char* data=devclass->data;
		*((uint32_t*)data)=0;
	}
*/