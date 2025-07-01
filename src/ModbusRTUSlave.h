#ifndef ModbusRTUSlave_h
#define ModbusRTUSlave_h

#define MODBUS_RTU_SLAVE_BUF_SIZE 256
#define NO_DE_PIN 255
#define NO_ID 0

#include "Arduino.h"
#ifdef __AVR__
#include <SoftwareSerial.h>
#endif

// As per modbus spec
enum ModbusFunctionCode {
    READ_COILS                          = 0x01, // Read Coils
    READ_DISCRETE_INPUTS                = 0x02, // Read Discrete Inputs
    READ_HOLDING_REGISTERS              = 0x03, // Read Holding Registers
    READ_INPUT_REGISTERS                = 0x04, // Read Input Registers
    WRITE_SINGLE_COIL                   = 0x05, // Write Single Coil
    WRITE_SINGLE_HOLDING_REGISTER       = 0x06, // Write Single Holding Register
    WRITE_MULTIPLE_COILS                = 0x0F, // Write Multiple Coils
    WRITE_MULTIPLE_HOLDING_REGISTERS    = 0x10, // Write Multiple Holding Registers
};

// Executes with effected registers
// On reads: Triggers before replying.
// On writes: Triggers after writing to registers and replying.
typedef void (*RequestMiddleware)(
                                    ModbusFunctionCode functionCode,
                                    uint16_t startAddress, 
                                    uint8_t quantity 
                                    );


class ModbusRTUSlave {
  public:
    ModbusRTUSlave(HardwareSerial& serial, uint8_t dePin = NO_DE_PIN);
    #ifdef __AVR__
    ModbusRTUSlave(SoftwareSerial& serial, uint8_t dePin = NO_DE_PIN);
    #endif
    #ifdef HAVE_CDCSERIAL
    ModbusRTUSlave(Serial_& serial, uint8_t dePin = NO_DE_PIN);
    #endif
    void configureCoils(bool coils[], uint16_t numCoils);
    void configureDiscreteInputs(bool discreteInputs[], uint16_t numDiscreteInputs);
    void configureHoldingRegisters(uint16_t holdingRegisters[], uint16_t numHoldingRegisters);
    void configureInputRegisters(uint16_t inputRegisters[], uint16_t numInputRegisters);
    #ifdef ESP32
    void begin(uint8_t id, unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin = -1, int8_t txPin = -1, bool invert = false);
    #else
    void begin(uint8_t id, unsigned long baud, uint32_t config = SERIAL_8N1);
    #endif
    void poll();
    
    RequestMiddleware middlewareFunction;

  private:
    HardwareSerial *_hardwareSerial;
    #ifdef __AVR__
    SoftwareSerial *_softwareSerial;
    #endif
    #ifdef HAVE_CDCSERIAL
    Serial_ *_usbSerial;
    #endif
    Stream *_serial;
    uint8_t _dePin;
    uint8_t _buf[MODBUS_RTU_SLAVE_BUF_SIZE];
    bool *_coils;
    bool *_discreteInputs;
    uint16_t *_holdingRegisters;
    uint16_t *_inputRegisters;
    uint16_t _numCoils = 0;
    uint16_t _numDiscreteInputs = 0;
    uint16_t _numHoldingRegisters = 0;
    uint16_t _numInputRegisters = 0;
    uint8_t _id;
    unsigned long _charTimeout;
    unsigned long _frameTimeout;
    #ifdef ARDUINO_ARCH_RENESAS
    unsigned long _flushCompensationDelay;
    #endif

    void _processReadCoils();
    void _processReadDiscreteInputs();
    void _processReadHoldingRegisters();
    void _processReadInputRegisters();
    void _processWriteSingleCoil();
    void _processWriteSingleHoldingRegister();
    void _processWriteMultipleCoils();
    void _processWriteMultipleHoldingRegisters();

    bool _readRequest();
    void _writeResponse(uint8_t len);
    void _exceptionResponse(uint8_t code);
    void _clearRxBuffer();

    void _calculateTimeouts(unsigned long baud, uint32_t config);
    uint16_t _crc(uint8_t len);
    uint16_t _div8RndUp(uint16_t value);
    uint16_t _bytesToWord(uint8_t high, uint8_t low);
};

#endif
