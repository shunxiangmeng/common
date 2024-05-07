

template<DELEGATE_CLASS_TYPES>
class TDelegate {
private:
    typedef typename detail::DefaultVoidToVoid<R>::type DesiredRetType;
    typedef DesiredRetType (*StaticFunctionPtr)(DELEGATE_TYPES);
    typedef R (*UnvoidStaticFunctionPtr)(DELEGATE_TYPES);
    typedef R (detail::GenericClass::*GenericMemFn)(DELEGATE_TYPES);
    typedef detail::ClosurePtr<GenericMemFn, StaticFunctionPtr, UnvoidStaticFunctionPtr> ClosureType;
    ClosureType mClosure;
public:
    // Typedefs to aid generic programming
    typedef TDelegate type;

    // Construction and comparison functions
    TDelegate() {
        clear();
    }

    TDelegate(const TDelegate &x) {
        mClosure.CopyFrom(this, x.mClosure);
    }

    void operator = (const TDelegate &x)  {
        mClosure.CopyFrom(this, x.mClosure);
    }
    
    bool operator ==(const TDelegate &x) const {
        return mClosure.IsEqual(x.mClosure);
    }

    bool operator !=(const TDelegate &x) const {
        return !mClosure.IsEqual(x.mClosure);
    }

    bool operator <(const TDelegate &x) const {
        return mClosure.IsLess(x.mClosure);
    }
    
    bool operator >(const TDelegate &x) const {
        return x.mClosure.IsLess(mClosure);
    }
        
    // Static functions. We convert them into a member function call.
    // This constructor also provides implicit conversion
    TDelegate(DesiredRetType (*method)(DELEGATE_TYPE_ARGS) ) {
        bind(method);
    }

    // binding to non-const member functions
    template < class X, class Y >
    TDelegate(DesiredRetType (X::* method)(DELEGATE_TYPE_ARGS), Y *pthis) {
        mClosure.bindmemfunc(detail::implicit_cast<X*>(pthis), method);
    }

    // binding to const member functions.
    template < class X, class Y >
    TDelegate(DesiredRetType (X::* method)(DELEGATE_TYPE_ARGS) const, const Y *pthis) {
        mClosure.bindconstmemfunc(detail::implicit_cast<const X*>(pthis), method);
    }
    
    template < class X, class Y >
    inline void bind(DesiredRetType (X::* method)(DELEGATE_TYPE_ARGS), Y *pthis) {
        mClosure.bindmemfunc(detail::implicit_cast<X*>(pthis), method);
    }

    template < class X, class Y >
    inline void bind(const Y *pthis, DesiredRetType (X::* method)(DELEGATE_TYPE_ARGS) const) {
        mClosure.bindconstmemfunc(detail::implicit_cast<const X *>(pthis), method);
    }

    // for efficiency, prevent creation of a temporary
    void operator = (DesiredRetType (*method)(DELEGATE_TYPE_ARGS) ) {
        bind(method);
    }
    
    inline void bind(DesiredRetType (*method)(DELEGATE_TYPE_ARGS)) {
        mClosure.bindstaticfunc(this, &TDelegate::InvokeStaticFunction, method);
    }
    
    // Invoke the delegate
    R operator() (DELEGATE_TYPE_ARGS) const {
        return (mClosure.GetClosureThis()->*(mClosure.GetClosureMemPtr()))(DELEGATE_ARGS);
    }
    // Implicit conversion to "bool" using the safe_bool idiom
private:
    typedef struct SafeBoolStruct {
        int a_data_pointer_to_this_is_0_on_buggy_compilers;
        StaticFunctionPtr m_nonzero;
    } UselessTypedef;
    typedef StaticFunctionPtr SafeBoolStruct::*unspecified_bool_type;

public:
    operator unspecified_bool_type() const {
        return empty()? 0: &SafeBoolStruct::m_nonzero;
    }

    // necessary to allow ==0 to work despite the safe_bool idiom
    inline bool operator==(StaticFunctionPtr funcptr) {
        return mClosure.IsEqualToStaticFuncPtr(funcptr);
    }
    
    inline bool operator!=(StaticFunctionPtr funcptr) {
        return !mClosure.IsEqualToStaticFuncPtr(funcptr);
    }
    
    // Is it bound to anything?
    inline bool operator ! () const {
            return !mClosure;
    }
    
    inline bool empty() const {
        return !mClosure;
    }
    
    void clear() {
        mClosure.clear();
    }
    
    // Conversion to and from the DelegateMemento storage class
    const detail::DelegateMemento & getMemento() {
        return mClosure;
    }

    void setMemento(const detail::DelegateMemento &any) {
        mClosure.CopyFrom(this, any);
    }

private:
    // Invoker for static functions
    R InvokeStaticFunction(DELEGATE_TYPE_ARGS) const {
        return (*(mClosure.GetStaticFunction()))(DELEGATE_ARGS);
    }
};
