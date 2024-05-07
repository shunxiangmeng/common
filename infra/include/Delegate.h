#pragma once
#include "SignalDetail/Delegate.h"

namespace infra {
//TDelegate
#define DELEGATE_NUMBER 
#define DELEGATE_CLASS_TYPES    class R, class... Args
#define DELEGATE_TYPES          Args...
#define DELEGATE_TYPE_ARGS      Args... args
#define DELEGATE_ARGS           args...
#include "SignalDetail/DelegateTemplate.h"

} // namespace infra
