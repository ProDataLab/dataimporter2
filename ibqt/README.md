# ibqt
A port of the Interactive Brokers API to the Qt framework

## USAGE

1. All functionality and data types are in the ibqt.h file.. so include "ibqt.h" into your project and instantiate an IBQt object.

2. See the [IB API reference guide for C++ (POSIX)](https://www.interactivebrokers.com/en/software/api/api.htm)

3. All EClientSocket Functions (requests to the server) are called the same way, but with Qt data types.

4. All EWrapper callback functions (responses from the server) are implemented here as Qt Signals.. meaning you provide a Qt slot and then connect them as per Qt Signals & Slots.

5. See the test app to get started.

