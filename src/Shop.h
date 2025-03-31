#ifndef _SHOP_H_
#define _SHOP_H_
#include <pthread.h>  // the header file for the pthread library

#include <condition_variable>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>  // the STL library: queue

#define DEFAULT_CHAIRS 3   // the default number of chairs for waiting = 3
#define DEFAULT_BARBERS 1  // the default number of barbers = 1

class Shop {
 public:
  bool closed;
  Shop(int nBarbers, int nChairs);
  Shop();
  int visitShop(int customerId);  // return a non-negative number only when a
                                  // customer got a service

  void leaveShop(int customerId, int barberId);
  void helloCustomer(int barberId);

  void byeCustomer(int barberId);

  int nDropsOff = 0;  // the number of customers dropped off

  void CloseShop();

 private:
  int nBarbers;
  int nChairs;

  enum customerState { WAIT, CHAIR, LEAVING };

  struct Barber {
    int id;
    std::condition_variable barberCond;
    int myCustomer = -1;  // no customer by default
  };

  struct Customer {
    int id;
    std::condition_variable customerCond;
    customerState state = WAIT;  // waiting state by default
    int myBarber = -1;           // no barber by default
  };

  Barber* barbers;                    // array of barber objects
  std::map<int, Customer> customers;  // container for customer objects

  std::queue<int> waitingCustomers;
  std::queue<int> sleepingBarbers;

  std::mutex mutex1;
  std::unique_lock<std::mutex> locker{mutex1};

  Barber* getBarber(int barberId);
};

#endif
