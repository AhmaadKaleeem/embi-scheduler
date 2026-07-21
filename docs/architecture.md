# Architecture

The simulator is structured into distinct, testable modules:

* `EventLoop`: Core event dispatcher processing Schedule, Arrival, and Lock events.
* `LockManager`: Maintains FIFO queues for contented resources. Tracks holder CPU accumulation.
* `BaseScheduler`: Abstract class. Implementations include `EMBIScheduler`, `MaxWeightScheduler`, `FCFSScheduler`, and `RoundRobinScheduler`.
* `ConfigLoader`: Parses experiment JSON structures to initialize `Config` instances.
