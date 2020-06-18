# C++ Channels

Implementation of Golang channles in c++.

## zero capacity channel

* Pusing thread is blocked until consumer thread pops.
* Or Popping thread is blocked until producing thread pushes.

## Capacity >= 1

* Producing thread can push utill capacity is full then it blocked.
* Consuming thread is blocked when popping an empty channel.