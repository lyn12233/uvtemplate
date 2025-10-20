# notes on USB 2.0 specification
## nomenclature
- EP: endpoint
## 4 architectual overview

## 5 data flow model

### 5.1 implementer viewpoints
implementation areas:
 - client software - function
 - system software - usb logical device
 - host controller - bus interface

### 5.2 bus topology
- physical topology
- logical topology
- client-software - function relationship
### 5.3 communication flow
- host controller driver to system software: framed usb data
- system software to client software: free-style data

logical flow: logical device[ endpoints,... ] --simplex pipes-- buffer|client software

#### 5.3.1 endpoints

endpoint 0: always accessible, connects to Default Control Pipe, requires to: respond to standard requests

other endpoints can not be used until successful config

#### 5.3.2 pipes

data flow in pipes: stream and message(with usb structs); attributes: bandwidth usage, transfer type, direction,...

denote IRP(io request packet) as an identifiable software requests to move data in between. software client will either be notified or wait. IRP will be aborted/retired when STALL is received or bus err occurs (for non isochronus pipe), with client notified. short packet not filling IRP buffer indicates 'end of data' when variable-sized data is desired or indicates error when fix-sized data is desired. host controller dirver must be informed of the 2 cases. endpoint can also respond NAK to inform it is busy, not an error.

stream pipes support bulk, isochronus, interrupt transfers; message pipes support control transfer

1ms frame is established for low/full speed bus, containing several transactions. transactions are restricted by transfer type.

### 5.4 transfer types

#### 5.4.1 performance calculation

given maximum number of transactions in a frame

### 5.5 control transfer
see 8.5.3 and 9
### 5.6 isochronous transfer
### 5.7 interrupt transfer
### 5.8 bulk transfer

## 6 mechanical
cable, receptable and icon's engineering drawings

## 7 electrical

## 8 

