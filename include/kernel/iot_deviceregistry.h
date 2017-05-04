#ifndef IOT_DEVICEREGISTRY_H
#define IOT_DEVICEREGISTRY_H
//Contains constants, methods and data structures for LOCAL hardware devices storing and searching


#include<stdint.h>
#include<assert.h>
//#include<time.h>

#include<ecb.h>


#include <iot_kapi.h>
#include <kernel/iot_common.h>


class hwdev_registry_t;
struct iot_hwdevregistry_item_t;
extern hwdev_registry_t* hwdev_registry;

#include<kernel/iot_moduleregistry.h>
#include<kernel/iot_kernel.h>

#define IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV 8


struct iot_hwdevregistry_item_t {
	iot_hwdevregistry_item_t *next, *prev; //position in actual_dev_head or removed_dev_head (when is_removed true)
	//TODO add next-prev fields for locations inside search indexes (by detector_module_id or devcontype) if necessary

	iot_hwdev_data_t devdata; //custom_data field will be assigned to custom_data buffer in current struct

	iot_modinstance_locker devdrv_modinstlk; //NULL if no driver connected or ref to driver module instance

	uint32_t blocked_modules[IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV]; //zero for unused slot
	uint32_t blocked_tms[IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV]; //0xFFFFFFFF if blocked forever

	uint32_t custom_len_alloced:24, //real size of allocated space for custom_data (could be allocated with reserve or have more space from previous use)
			is_blocked:1,		//flag that hw device is blocked from finding a driver
			is_removed:1;		//flag that this item is in removed_dev_head list
	alignas(4) char custom_data[];  //depends on devcontype and actual data


	bool is_module_blocked(uint32_t module_id, uint32_t now32) {
		for(int i=0;i<IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV;i++) {
			if(!blocked_modules[i] || blocked_modules[i]!=module_id) continue;
			if(blocked_tms[i]==0xFFFFFFFFu || blocked_tms[i]>now32) return true;
			//timeout ended
			blocked_modules[i]=0;
			break;
		}
		return false;
	}
	void clear_module_block(uint32_t module_id) { //zero value clears for all
		if(!module_id) {
			memset(blocked_modules,0,sizeof(blocked_modules));
			return;
		}
		for(int i=0;i<IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV;i++) {
			if(!blocked_modules[i] || blocked_modules[i]!=module_id) continue;
			blocked_modules[i]=0;
			break;
		}
	}
	bool block_module(uint32_t module_id, uint32_t till, uint32_t now32) {
		//returns false if all slots are busy
		for(int i=0;i<IOT_CONFIG_MAX_BLOCKED_MODULES_PER_HWDEV;i++) {
			if(!blocked_modules[i] || blocked_modules[i]==module_id || blocked_tms[i]<=now32) {
				blocked_modules[i]=module_id;
				blocked_tms[i]=till;
				return true;
			}
		}
		return false;
	}
};

//singleton class to keep and manage registry of LOCAL HARDWARE devices
class hwdev_registry_t {
	iot_hwdevregistry_item_t* actual_dev_head; //bi-linked list of actual hw devices
	iot_hwdevregistry_item_t* removed_dev_head; //bi-linked list of hw devices which were removed but have active reference in driver modules


public:
	bool have_unconnected_devs; //flag that there are hw devices without driver and some drivers were delayed due to temp errors, so periodic search must be attempted
								//TODO. make periodic recheck every 2 minutes

	hwdev_registry_t(void) : actual_dev_head(NULL), removed_dev_head(NULL), have_unconnected_devs(false) {
		assert(hwdev_registry==NULL);
		hwdev_registry=this;
	}
	void list_action(iot_action_t action, iot_hwdev_localident_t* ident, size_t custom_len, void* custom_data); //main thread
	//finish removal or removed device after stopping bound driver
	void finish_hwdev_removal(iot_hwdevregistry_item_t* it) { //main thread
		assert(it->is_removed);
		assert(!it->devdrv_modinstlk);
		BILINKLIST_REMOVE(it, next, prev); //remove from removed list
		free(it);
	}

	void try_find_hwdev_for_driver(iot_module_item_t* module); //main thread
	int try_connect_local_driver(iot_device_connection_t* conn);

	iot_hwdevregistry_item_t* find_item_byaddr(iot_hwdev_localident_t* ident) { //looks for device item by contype and address
		iot_hwdevregistry_item_t* it=actual_dev_head;
		while(it) {
			if(it->devdata.ident_iface->matches_addr(&it->devdata.dev_ident.dev, ident)) return it;
			it=it->next;
		}
		return NULL;
	}
	iot_hwdevregistry_item_t* find_item_bytmpl(iot_hwdev_localident_t* tmpl) { //looks for device item by contype and address
		iot_hwdevregistry_item_t* it=actual_dev_head;
		while(it) {
			if(it->devdata.ident_iface->matches(&it->devdata.dev_ident.dev, tmpl)) return it;
			it=it->next;
		}
		return NULL;
	}
};




#endif //IOT_DEVICEREGISTRY_H
