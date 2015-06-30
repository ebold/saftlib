#ifndef FUNCTION_GENERATOR_H
#define FUNCTION_GENERATOR_H

#include <deque>

#include "interfaces/FunctionGenerator.h"
#include "Owned.h"

namespace saftlib {

class TimingReceiver;

class FunctionGeneratorChannelAllocation : public Glib::Object
{
  public:
    std::vector<int> indexes;
};

class FunctionGenerator : public iFunctionGenerator, public Owned
{
  public:
    typedef FunctionGenerator_Service ServiceType;
    struct ConstructorType {
      TimingReceiver* dev;
      Glib::RefPtr<FunctionGeneratorChannelAllocation> allocation;
      eb_address_t fgb;
      eb_address_t swi;
      unsigned num_channels;
      unsigned buffer_size;
      unsigned int index;
      guint32 macro;
    };
    
    static Glib::RefPtr<FunctionGenerator> create(const Glib::ustring& objectPath, ConstructorType args);
    
    // iFunctionGenerator overrides
    void Arm();
    void Abort();
    void AppendParameterSet(const std::vector< gint16 >& coeff_a, const std::vector< gint16 >& coeff_b, const std::vector< gint32 >& coeff_c, const std::vector< unsigned char >& step, const std::vector< unsigned char >& freq, const std::vector< unsigned char >& shift_a, const std::vector< unsigned char >& shift_b);
    void Flush();
    guint32 getVersion() const;
    unsigned char getSCUbusSlot() const;
    unsigned char getDeviceNumber() const;
    unsigned char getOutputWindowSize() const;
    bool getEnabled() const;
    bool getArmed() const;
    bool getRunning() const;
    guint32 getStartTag() const;
    guint64 getFillLevel() const;
    guint64 getSafeFillLevel() const;
    bool getAboveSafeFillLevel() const;
    guint16 getExecutedParameterCount() const;
    void setStartTag(guint32 val);
    void setSafeFillLevel(guint64 val);
    
  protected:
    FunctionGenerator(ConstructorType args);
    ~FunctionGenerator();
    void updateAboveSafeFillLevel();
    void irq_handler(eb_data_t status);
    void refill();
    void releaseChannel();
    void acquireChannel();
    void ownerQuit();
    
    TimingReceiver* dev;
    Glib::RefPtr<FunctionGeneratorChannelAllocation> allocation;
    eb_address_t shm;
    eb_address_t swi;
    unsigned num_channels;
    unsigned buffer_size;
    unsigned int index;
    unsigned char scubusSlot;
    unsigned char deviceNumber;
    unsigned char version;
    unsigned char outputWindowSize;
    eb_address_t irq;

    int channel; // -1 if no channel assigned
    bool enabled;
    bool armed;
    bool running;
    bool abort;
    guint32 startTag;
    unsigned executedParameterCount;
    
    struct ParameterTuple {
      gint16 coeff_a;
      gint16 coeff_b;
      gint32 coeff_c;
      guint8 step;
      guint8 freq;
      guint8 shift_a;
      guint8 shift_b;
      
      guint64 duration() const;
    };
    
    // These 4 variables must be kept in sync:
    bool aboveSafeFillLevel;
    guint64 safeFillLevel;
    guint64 fillLevel;
    unsigned filled; // # of fifo entries currently on LM32
    std::deque<ParameterTuple> fifo;
};

}

#endif