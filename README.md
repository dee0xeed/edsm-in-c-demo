# Event driven state machines in C for Linux (epoll)

## About `.smd` files

There are a number of files with `.smd` suffix:

```
./echo-server/smd/worker.smd
./echo-server/smd/poolman.smd
./echo-server/smd/rx.smd
./echo-server/smd/listener.smd
./echo-server/smd/tx.smd
./echo-server/smd/manager.smd

```

'smd' stands for 'state machine description'.

These files have very simple syntax, below are two examples
(NOTE: a line starting with at least one space is a comment):

### rx.smd
```
T180000
  3 min timer, used for timeout

$init
$idle
$work
  these are states

+init M0 idle
  this is transition

@idle M1
  this is action (program must have function named 'rx_idle_M0')
+idle M0 work
  transition 'idle->work' on M0 message

@work D0
  D0 is POLLIN, rx_work_D0() will be called
@work D2
  D2 is POLLERR, rx_work_D2() will be called
@work T0
  timeout, rx_work_T0() will be called
+work M0 idle
```

### manager.smd
```
S2
  SIGINT
S15
  SIGTERM

$init
$work
  two states

+init M0 work
  transition from init to work on M0 message

@work M0
@work M1
@work S0
@work S1
```
