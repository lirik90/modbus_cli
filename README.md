# modbus_cli
Simple modbus client based on Qt framework

## Build:
```bash
git clone https://github.com/lirik90/modbus_cli.git
cd modbus_cli
qmake
make -j2
```

## Run
`./modbus_cli -h`

## Example
### Read two holding registers from MTCP
`./modbus_cli -r -t 4 -a 1 -s 1103 -c 2 mtcp://10.10.2.106:502`

### Read two holding registers from RTU
`./modbus_cli -r -t 4 -a 1 -s 0 -c 2 rtu:///dev/ttyUSB0?baudRate=9600`

### Send raw request
`./modbus_cli --address 1 --func_hex 10 --raw 00020001020002 rtu:///dev/ttyS0?baudRate=9600`

### Write values 15,24,35 to multiple holding registers and use full RTU connection string
`./modbus_cli -w 15,24,35 -t 4 -a 1 -s 0 rtu:///dev/ttyUSB0?baudRate=9600&dataBits=8&parity=2&stopBits=1&flowControl=0`

## RTU Connection parameters
### dataBits
Can use values 5, 6, 7, or 8

### parity
- 0 is NoParity
- 2 is EvenParity
- 3 is OddParity
- 4 is SpaceParity
- 5 is MarkParity

### stopBits
- 1 is OneStop
- 2 is TwoStop
- 3 is OneAndHalfStop

### flowControl
- 0 is NoFlowControl
- 1 is HardwareControl
- 2 is SoftwareControl
