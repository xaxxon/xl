#pragma once

#include <memory>
#include <functional>
#include <type_traits>

// data structure to allow code to be run after parent type is constructed
// but before any of the current class's data members are constructed.
template<typename T>
struct RunLaterData {
    T * data;
    std::function<void()> run_later;

    operator bool() const {return data != nullptr;}
    operator T*() const {return data;}

    RunLaterData(RunLaterData &&) = delete; // not movable only copyable
    RunLaterData & operator=(RunLaterData && other) {
	this->data = other.data;
	return *this;
    }
    RunLaterData(T * data = nullptr, std::function<void()> run_later = {}) :
	data(data),
	run_later(run_later)
    {}

    RunLaterData(RunLaterData const & other) :
	data(other.data)
    {}

    RunLaterData & operator=(RunLaterData const & other) {
	this->data = other.data;
	return *this;
    }

    template<class U>
    RunLaterData(RunLaterData<U> const & other) :
	data(other.data)
    {
	static_assert(std::is_convertible_v<U*, T*>);
    }

    ~RunLaterData() {
	if(run_later) { run_later();}
    }
};



#define PIMPL_HEADER_COMMON \
    struct Pimpl; \
    struct Allocation; \
    struct PimplHolder; \
    Pimpl * get_impl() const;
    
#define PIMPL_HEADER \
    PIMPL_HEADER_COMMON;

#define PIMPL_HEADER_ROOT \
    PIMPL_HEADER_COMMON; \
    std::unique_ptr<Pimpl, void(*)(Pimpl *)> impl;



#define PIMPL_PARAM_HEADER			\
    Allocation * allocation = nullptr

#define PIMPL_PARAM(class_name)			\
    class_name::Allocation * allocation

#define PIMPL(class_name)			\
    PIMPL_CORE(class_name)			\
    struct class_name::Allocation :		\
    class_name::Super::Allocation,		\
	PimplHolder				\
    {};

#define PIMPL_ROOT(class_name)			\
    PIMPL_CORE(class_name)			\
    struct class_name::Allocation :		\
    PimplHolder					\
    {};						\

#define PIMPL_CORE(class_name)						\
    struct class_name::PimplHolder {					\
	std::aligned_storage_t<sizeof(Pimpl), alignof(Pimpl)> a;	\
    };									\
    class_name::Pimpl * class_name::get_impl() const {return reinterpret_cast<Pimpl*>(this->impl.get());}

#define PIMPL_PARENT(f, ...)						\
    Super(__VA_OPT__(__VA_ARGS__,) [this, allocation]() mutable {	\
	    RunLaterData<Allocation> new_run_later;			\
	    if (allocation != nullptr) { /* memory supplied from derived type */	\
		new_run_later = allocation;				\
	    } else { /* memory needs to be allocated */			\
		new_run_later = RunLaterData<Allocation>(new Allocation); \
	    }								\
	    /* Set up the callback for after parent is constructed */	\
	    new_run_later.run_later = [this, new_run_later](){		\
		/* if there's a failure who is responsible for cleaning this up? */ \
		this->impl.get_deleter() = [](auto ptr) {		\
		    delete reinterpret_cast<Allocation *>(ptr);		\
		};							\
		::new(static_cast<Pimpl*>(reinterpret_cast<Pimpl *>(static_cast<PimplHolder *>(new_run_later)))) Pimpl(f); \
	    };								\
	    return new_run_later;					\
	}())



#define PIMPL_INIT(...)							\
    impl([&](){								\
	    if (allocation != nullptr) {						\
		::new(static_cast<PimplHolder*>(allocation)) Pimpl(__VA_ARGS__); \
		return std::unique_ptr<Pimpl, void(*)(Pimpl*)>(reinterpret_cast<Pimpl *>(static_cast<PimplHolder*>(allocation)), nullptr); \
	    } else {							\
		return std::unique_ptr<Pimpl, void(*)(Pimpl*)>(new Pimpl(__VA_ARGS__), nullptr); \
	    }								\
	}())

#define PIMPL_DESTROY \
    get_impl()->Pimpl::~Pimpl();
