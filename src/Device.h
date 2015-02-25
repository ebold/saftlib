#ifndef SAFTLIB_DEVICES_H
#define SAFTLIB_DEVICES_H

#include <etherbone.h>
#include <giomm.h>

namespace saftlib {

class Device : public etherbone::Device {
  public:
    eb_address_t request_irq(const sigc::slot<void,eb_data_t>& slot);
    void release_irq(eb_address_t);
    
    Device(etherbone::Device d, eb_address_t low, eb_address_t high);
    
  private:
    eb_address_t low, high;
};

typedef std::vector<Device> Devices;

}

#endif
