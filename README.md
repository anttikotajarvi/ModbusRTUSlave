# ModbusRTUSlave legacy w/ middleware
Fork of [CMB27/ModbusRTUSlave](https://github.com/CMB27/ModbusRTUSlave) version 2.0.6 (legacy version).
The legacy version was preffered instead of the latest 3.x.x version because of the v3's seemingly unnecessary dependency split.

## Modbus Middleware
This implements middleware for the modbus slave allowing for simple reactive systems.

The middleware function is defined as:
``` C++
typedef void (*RequestMiddleware)(
  ModbusFunctionCode functionCode,
  uint16_t startAddress, 
  uint8_t quantity 
);
```
It reports the subject registers on a modbus call.\

**On modbus READ events:**
- The middleware function is invoked BEFORE copying the register(s) for the reply.
- This allows for the implementation of a system which can dynamically copy or translate system state into only the requested registers.


**On modbus WRITE events:**
- The middleware function is invoked AFTER writing to the modbus registers.
- This allows a system which can react to commands (e.g.) without having to sift through registers and checking which ones were changed.

**Thus:** \
-> More dynamic \
-> More robust \
-> Less modbus overhead \
-> Cool


## Example implementation:
``` C++
void _readInputRegisters(uint16_t startAddress, uint8_t quantity) {
    uint8_t counter = 0;
    switch (startAddress) {
        // VERSION
        case 0x0000: counter++;
            inputRegisters[VERSION] = serializeSysVersion(version);
            if(counter >= quantity) break;
        // LIVE_RTC_TIME
        case 0x0001: counter++;
            inputRegisters[LIVE_RTC_TIME] = minutesFromMidnight(rtc.now());
            if(counter >= quantity) break;
        // ...
        default:
          return;
    }
}
void _writeMultipleHoldingRegisters(uint16_t startAddress, uint8_t quantity) {
    uint8_t counter = 0;
    switch(startAddress) {
        case 0x0000: counter++; // APPLY_SYSTEM_SETTINGS
        {
            // Apply
            Mutators::readSysSettingsFromHoldingRegisters(holdingRegisters);
            // Save
            POST_HW_UPDATE(saveSystemSettings);
            Mutators::AddCallbackTask([]() {
                if(State.saveSystemSettingsOK) {
                    // Nothing <- since the settings are already applied
                } else {
                    Mutators::setLastCommandError(WRITE_MULTIPLE_HOLDING_REGISTERS, APPLY_SYSTEM_SETTINGS);
                    Mutators::error(EEPROM_WRITE_ERROR, false); // Should still remain operational
                }
            });
        }
       if(counter >= quantity) break;
        // ...
        default:
            return;
}
void modbusMiddleware(ModbusFunctionCode functionCode, uint16_t startAddress, uint8_t quantity) {
    uint8_t counter = 0;

    switch(functionCode) {
        case READ_COILS:                        // 0x01
            break;
        case READ_DISCRETE_INPUTS:              // 0x02
            _readDiscreteInputs(startAddress, quantity);
            break;
        case READ_HOLDING_REGISTERS:            // 0x03
            break;
        case READ_INPUT_REGISTERS:              // 0x04
            _readInputRegisters(startAddress, quantity);
            break;
        case WRITE_SINGLE_COIL:                 // 0x05
        case WRITE_MULTIPLE_COILS:              // 0x0F
            _writeMultipleCoils(startAddress, quantity);
            break;
        case WRITE_SINGLE_HOLDING_REGISTER:     // 0x06
        case WRITE_MULTIPLE_HOLDING_REGISTERS:  // 0x10
            _writeMultipleHoldingRegisters(startAddress, quantity);
            break;
        default:
            break;
    }
    return;
}    
void setup() {
  //...
  modbus.begin(1, 9600);
  modbus.configureCoils(coils, C_MAX);
  modbus.configureDiscreteInputs(discreteInputs, DI_MAX);
  modbus.configureHoldingRegisters(holdingRegisters, HR_MAX);
  modbus.configureInputRegisters(inputRegisters, IR_MAX);
  modbus.middlewareFunction = modbusMiddleware;
  //...
}
```
**Note**
In actual use you would need more than a couple of registers to justify the need for this. \
It is very easy to shoot yourself in the foot if not wfollowing good coding practices. You're going to want to keep reads idempotentic; even something like:
```C++
        // LIVE_RTC_TIME
        case 0x0001: counter++;
            inputRegisters[LIVE_RTC_TIME] = minutesFromMidnight(rtc.now());
            if(counter >= quantity) break;
```
on a read event can be dangerous if you are not sure of the side effects (in this case from `rtc.now()`). \
Also -- while the switch system in this example works great it will be a pain to maintain.
