/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <iostream>
#include <iomanip>

#include <assert.h>

#include "RegisteredObject.h"
#include "WrMilGateway.h"
#include "TimingReceiver.h"
#include "wr_mil_gw_regs.h" 
#include "clog.h"

namespace saftlib {

WrMilGateway::WrMilGateway(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   base(args.base),
   have_wrmilgw(false)
{
  // get all LM32 devices and see if any of them has the WR-MIL-Gateway magic number
  std::vector<struct sdb_device> devices;
  dev->getDevice().sdb_find_by_identity(UINT64_C(0x651), UINT32_C(0x54111351), devices);
  for(auto device: devices) {
    eb_data_t wr_mil_gw_magic_number = 0;
    dev->getDevice().read(device.sdb_component.addr_first + WR_MIL_GW_SHARED_OFFSET, 
                          EB_DATA32, 
                          &wr_mil_gw_magic_number);
    if (wr_mil_gw_magic_number == WR_MIL_GW_MAGIC_NUMBER) {
      wrmilgw_device = device;
      base = wrmilgw_device.sdb_component.addr_first;
      have_wrmilgw = true;
      break;
    }
  }

  // just throw if there is no active firmware
  if (!have_wrmilgw) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "No WR-MIL-Gateway found");
  }


  // the firmware is not running if the command value was not overwritten
  if (!firmwareRunning()) {
    throw IPC_METHOD::Error(IPC_METHOD::Error::FAILED, "WR-MIL-Gateway not running");
  }

  std::cerr << "WR-MIL Gateway firmware running" << std::endl;

  // read all registers once
  //getRegisterContent();
}

WrMilGateway::~WrMilGateway()
{
  std::cerr << "~WrMilGateway called " << std::endl;
}

bool WrMilGateway::firmwareRunning() const
{
  // intentionally cast away the constness, because this is a temporary modification of a register
  // and saft daemon makes sure that this method is not called by two instances simultaneously
  WrMilGateway *nonconst = const_cast<WrMilGateway*>(this);

   // see if the firmware is running (it should reset the CMD register to 0 after a command is put there)
   // submit a test command 
  nonconst->writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_TEST);
  usleep(50000);
  // command register will be cleared if the firmware is running;
  return (nonconst->readRegisterContent(WR_MIL_GW_REG_COMMAND) == 0);
}

bool WrMilGateway::getFirmwareRunning()  const
{
  return firmwareRunning();
}

guint32 WrMilGateway::readRegisterContent(guint32 reg_offset) const
{
    eb_data_t value;
    dev->getDevice().read(base + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, &value);
    return value;
}
void WrMilGateway::writeRegisterContent(guint32 reg_offset, guint32 value)
{
    dev->getDevice().write(base + WR_MIL_GW_SHARED_OFFSET + reg_offset, EB_DATA32, (eb_data_t)value);
}

Glib::RefPtr<WrMilGateway> WrMilGateway::create(const ConstructorType& args)
{
  std::cerr << "WrMilGateway::create() called" << std::endl;
  return RegisteredObject<WrMilGateway>::create(args.objectPath, args);
}

std::vector< guint32 > WrMilGateway::getRegisterContent() const
{
  std::vector<guint32> registerContent((WR_MIL_GW_REG_LAST-WR_MIL_GW_REG_MAGIC_NUMBER) / 4, 0);
  for (unsigned i = 0; i < registerContent.size(); ++i) {
    eb_data_t value;
    dev->getDevice().read(base + WR_MIL_GW_SHARED_OFFSET + 4*i, EB_DATA32, &value);
    registerContent.push_back(value);
  }
  return registerContent;
}

void WrMilGateway::StartSIS18()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_SIS);
}
void WrMilGateway::StartESR()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_CONFIG_ESR);
}
void WrMilGateway::ResetGateway()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_RESET);
}
void WrMilGateway::KillGateway()
{
  writeRegisterContent(WR_MIL_GW_REG_COMMAND, WR_MIL_GW_CMD_KILL);
}


guint32 WrMilGateway::getWrMilMagic() const
{
  return readRegisterContent(WR_MIL_GW_REG_MAGIC_NUMBER);
}
guint32 WrMilGateway::getFirmwareState() const
{
  return readRegisterContent(WR_MIL_GW_REG_STATE);
}
guint32 WrMilGateway::getEventSource() const
{
  return readRegisterContent(WR_MIL_GW_REG_EVENT_SOURCE);
}
unsigned char WrMilGateway::getUtcTrigger() const
{
  return readRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER);
}
guint32 WrMilGateway::getEventLatency() const
{
  return readRegisterContent(WR_MIL_GW_REG_LATENCY);
}
guint32 WrMilGateway::getUtcUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_UTC_DELAY);
}
guint32 WrMilGateway::getTriggerUtcDelay() const
{
  return readRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY);
}
guint64 WrMilGateway::getUtcOffset() const
{
  guint64 result = readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO);
  return result;
}
guint64 WrMilGateway::getNumMilEvents() const
{
  guint64 result = readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_HI);
  result <<= 32;
  result |= readRegisterContent(WR_MIL_GW_REG_NUM_EVENTS_LO);
  return result;
}
guint32 WrMilGateway::getNumLateMilEvents() const
{
  return readRegisterContent(WR_MIL_GW_REG_LATE_EVENTS);
}

void WrMilGateway::setUtcTrigger(unsigned char val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_TRIGGER, val);
}
void WrMilGateway::setEventLatency(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_LATENCY, val);
}
void WrMilGateway::setUtcUtcDelay(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_DELAY, val);
}
void WrMilGateway::setTriggerUtcDelay(guint32 val)
{
  writeRegisterContent(WR_MIL_GW_REG_TRIG_UTC_DELAY, val);
}
void WrMilGateway::setUtcOffset(guint64 val)
{
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
  val >>= 32;
  writeRegisterContent(WR_MIL_GW_REG_UTC_OFFSET_LO, val & 0x00000000ffffffff);
}



void WrMilGateway::Reset() 
{
}

void WrMilGateway::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

}