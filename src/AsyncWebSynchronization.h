#ifndef ASYNCWEBSYNCHRONIZATION_H_
#define ASYNCWEBSYNCHRONIZATION_H_

// Synchronisation is only available on ESP32, as the ESP8266 isn't using FreeRTOS by default

#include <ESPAsyncWebServer.h>

#ifdef ESP32

// This is the ESP32 version of the Sync Lock, using the FreeRTOS Semaphore
// Modified 'AsyncWebLock' to just only use mutex.
// AsyncWebLock does not work in ServerSide Events with _RunQueue locking, as it is called from different proceses?
// Name is upto discussion, right now it's just simple mutex implementation
class AsyncWebLockMQ
{
private:
  SemaphoreHandle_t _lock;
  mutable void *_lockedBy;

public:
  AsyncWebLockMQ() {
    _lock = xSemaphoreCreateBinary();
    //_lockedBy = NULL;
    xSemaphoreGive(_lock);
  }

  ~AsyncWebLockMQ() {
    vSemaphoreDelete(_lock);
  }

  bool lock() const {
    //extern void *pxCurrentTCB;
    //if (_lockedBy != pxCurrentTCB) {
      xSemaphoreTake(_lock, portMAX_DELAY);
     // _lockedBy = pxCurrentTCB;
      return true;
    // }
    // return false;
  }

  void unlock() const {
    //_lockedBy = NULL;
    xSemaphoreGive(_lock);
  }
};

// This is the ESP32 version of the Sync Lock, using the FreeRTOS Semaphore
class AsyncWebLock
{
private:
  SemaphoreHandle_t _lock;
  mutable void *_lockedBy;

public:
  AsyncWebLock() {
    _lock = xSemaphoreCreateBinary();
    _lockedBy = NULL;
    xSemaphoreGive(_lock);
  }

  ~AsyncWebLock() {
    vSemaphoreDelete(_lock);
  }

  bool lock() const {
    extern void *pxCurrentTCB;
    if (_lockedBy != pxCurrentTCB) {
      xSemaphoreTake(_lock, portMAX_DELAY);
      _lockedBy = pxCurrentTCB;
      return true;
    }
    return false;
  }

  void unlock() const {
    _lockedBy = NULL;
    xSemaphoreGive(_lock);
  }
};

#else

// This is the 8266 version of the Sync Lock which is currently unimplemented
class AsyncWebLock
{

public:
  AsyncWebLock() {
  }

  ~AsyncWebLock() {
  }

  bool lock() const {
    return false;
  }

  void unlock() const {
  }
};
#endif

class AsyncWebLockGuard
{
private:
  const AsyncWebLock *_lock;

public:
  AsyncWebLockGuard(const AsyncWebLock &l) {
    if (l.lock()) {
      _lock = &l;
    } else {
      _lock = NULL;
    }
  }

  ~AsyncWebLockGuard() {
    if (_lock) {
      _lock->unlock();
    }
  }
};

#endif // ASYNCWEBSYNCHRONIZATION_H_