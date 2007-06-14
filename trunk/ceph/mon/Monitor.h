// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */


#ifndef __MONITOR_H
#define __MONITOR_H

#include "include/types.h"
#include "msg/Messenger.h"

#include "common/Timer.h"

#include "MonMap.h"
#include "Elector.h"
#include "Paxos.h"


class MonitorStore;
class OSDMonitor;
class MDSMonitor;
class ClientMonitor;

#define PAXOS_TEST       0
#define PAXOS_OSDMAP     1
#define PAXOS_MDSMAP     2
#define PAXOS_CLIENTMAP  3

class Monitor : public Dispatcher {
protected:
  // me
  int whoami;
  Messenger *messenger;
  Mutex lock;

  MonMap *monmap;

  // timer.
  SafeTimer timer;
  Context *tick_timer;
  void cancel_tick();
  void reset_tick();
  friend class C_Mon_Tick;

  // -- local storage --
  MonitorStore *store;


  // -- monitor state --
private:
  const static int STATE_STARTING = 0; // electing
  const static int STATE_LEADER =   1;
  const static int STATE_PEON =     2;
  int state;

public:
  bool is_starting() { return state == STATE_STARTING; }
  bool is_leader() { return state == STATE_LEADER; }
  bool is_peon() { return state == STATE_PEON; }


  // -- elector --
private:
  Elector elector;
  friend class Elector;
  
  epoch_t  mon_epoch;    // monitor epoch (election instance)
  int leader;            // current leader (to best of knowledge)
  set<int> quorum;       // current active set of monitors (if !starting)
  utime_t last_called_election;  // [starting] last time i called an election
  
public:
  // initiate election
  void call_election();

  // end election (called by Elector)
  void win_election(epoch_t epoch, set<int>& q);
  void lose_election(epoch_t epoch, int l);

  int get_leader() { return leader; }
  const set<int>& get_quorum() { return quorum; }


  // -- paxos --
  Paxos test_paxos;
  friend class Paxos;
  

  // -- services --
  OSDMonitor *osdmon;
  MDSMonitor *mdsmon;
  ClientMonitor *clientmon;

  friend class OSDMonitor;
  friend class MDSMonitor;
  friend class ClientMonitor;


  // messages
  void handle_shutdown(Message *m);
  void handle_ping_ack(class MPingAck *m);
  void handle_command(class MMonCommand *m);



 public:
  Monitor(int w, Messenger *m, MonMap *mm) : 
    whoami(w), 
    messenger(m),
    monmap(mm),
    timer(lock), tick_timer(0),
    store(0),

    state(STATE_STARTING),

    elector(this, w),
    mon_epoch(0), 
    leader(0),
    
    test_paxos(this, w, PAXOS_TEST, "tester"),  // tester state machine

    osdmon(0), mdsmon(0), clientmon(0)
  {
  }

  void init();
  void shutdown();
  void dispatch(Message *m);
  void tick();

  void do_stop();

};

#endif
