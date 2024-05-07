#pragma once
#include <list>
#include <mutex>
#include "Delegate.h"

namespace infra {

//TSignal
#define SIGNAL_NUMBER 
#define SIGNAL_CLASS_TYPES  class R, class ...Args
#define SIGNAL_TYPES_COMMA  , Args...
#define SIGNAL_TYPE_ARGS    Args... args
#define SIGNAL_ARGS         args...
#include "SignalDetail/SignalTemplate.h"

} // namespace infra
