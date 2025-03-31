#include "Shop.h"

Shop::Shop(int nBarbers,
           int nChairs)  // initialize a Shop object with nBarbers and nChairs
{
  this->nBarbers = nBarbers;
  this->nChairs = nChairs;
  closed = false;

  barbers = new Barber[nBarbers];
  for (int i = 0; i < nBarbers; i++) {
    barbers[i].id = i;
  }
}

Shop::Shop()  // initialize a Shop object with 1 barber and 3 chairs
{
  this->nBarbers = DEFAULT_BARBERS;
  this->nChairs = DEFAULT_CHAIRS;
  closed = false;

  barbers = new Barber[nBarbers];
  for (int i = 0; i < nBarbers; i++) {
    barbers[i].id = i;
  }
}

int Shop::visitShop(int customerId)  // return a non-negative number only when a
                                     // customer got a service
{
  mutex1.lock();
  if (((int)waitingCustomers.size() == nChairs) && (sleepingBarbers.empty())) {
    printf(
        "customer[%i]: leaves the shop because of no available waiting chairs. "
        "\n",
        customerId);
    nDropsOff++;
    mutex1.unlock();
    return -1;
  }

  customers[customerId].id = customerId;
  int barberId;

  if (sleepingBarbers.empty()) {
    waitingCustomers.push(customerId);

    printf(
        "customer[%i]]: takes a waiting chair. # waiting seats available = %i "
        "\n",
        customerId, (int)(nChairs - waitingCustomers.size()));

    while (customers[customerId].myBarber == -1) {
      customers[customerId].customerCond.wait(locker);
      if (closed) return -1;
    }
    barberId = customers[customerId].myBarber;
  } else {  // there are sleeping barbers in a queue
    barberId = sleepingBarbers.front();
    sleepingBarbers.pop();
    customers[customerId].myBarber = barberId;
    getBarber(barberId)->myCustomer = customerId;
  }

  printf(
      "customer[%i]: moves to a service chair[%i], # waiting seats available = "
      "%i \n",
      customerId, barberId, (int)(nChairs - waitingCustomers.size()));

  customers[customerId].state = CHAIR;
  getBarber(barberId)->barberCond.notify_one();
  mutex1.unlock();
  return barberId;
}

void Shop::leaveShop(int customerId, int barberId) {
  mutex1.lock();
  printf("customer[%i]: wait for barber[%i] to be done with hair-cut.\n",
         customerId, barberId);

  while (customers[customerId].myBarber != -1) {
    customers[customerId].customerCond.wait(locker);
  }

  printf("customer[%i]: says good-bye to barber[%i]\n", customerId, barberId);

  customers[customerId].state = LEAVING;
  getBarber(barberId)->barberCond.notify_one();

  mutex1.unlock();
}

void Shop::helloCustomer(int barberId) {
  mutex1.unlock();

  if (getBarber(barberId)->myCustomer == -1) {
    printf("barber  [%i]: sleeps because of no customers.\n", barberId);
    sleepingBarbers.push(barberId);
    while (getBarber(barberId)->myCustomer == -1) {
      getBarber(barberId)->barberCond.wait(locker);
    }
  }

  while (customers[getBarber(barberId)->myCustomer].state !=
         CHAIR)  // synchronization with customer thread
  {
    getBarber(barberId)->barberCond.wait(locker);
  }

  printf("barber  [%i]: starts a hair-cut service for customer[%i]\n", barberId,
         getBarber(barberId)->myCustomer);

  mutex1.unlock();
}

void Shop::byeCustomer(int barberId) {
  mutex1.lock();
  printf(
      "barber  [%i]: says he's done with a hair-cut service for customer[%i]\n",
      barberId, getBarber(barberId)->myCustomer);

  customers[getBarber(barberId)->myCustomer].myBarber = -1;
  customers[getBarber(barberId)->myCustomer].customerCond.notify_one();
  while (customers[getBarber(barberId)->myCustomer].state !=
         LEAVING)  // synchronization with customer thread
  {
    getBarber(barberId)->barberCond.wait(locker);
  }
  getBarber(barberId)->myCustomer = -1;

  printf("barber  [%i]: calls in another customer.\n", barberId);
  if (!waitingCustomers.empty()) {
    int customerId = waitingCustomers.front();
    waitingCustomers.pop();
    getBarber(barberId)->myCustomer = customerId;
    customers[customerId].myBarber = barberId;  //?
    customers[customerId].customerCond.notify_one();
  }

  mutex1.unlock();
}

Shop::Barber* Shop::getBarber(int barberId) {
  for (int i = 0; i < nBarbers; i++) {
    if (barbers[i].id == barberId) {
      return &barbers[i];
    }
  }
  return NULL;
}

void Shop::CloseShop() {
  closed = true;
  for (int i = 0; i < nBarbers; i++) {
    barbers[i].barberCond.notify_one();
  }
  for (auto customer : customers) {
    customer.second.customerCond.notify_one();
  }
}
