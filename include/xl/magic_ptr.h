#pragma once

namespace xl {



class magic_ptr_exception : public std::exception {
    std::string reason;

public:

    magic_ptr_exception(std::string const & reason) : reason(reason) {}
    magic_ptr_exception(std::string && reason) : reason(std::move(reason)) {}

    const char * what() const noexcept {
        return reason.c_str();
    }

};

/**
 * A magic_ptr is similar to a unique_ptr but it knows if it's holding a "reference" or an owning
 * pointer based on whether it was created with an rvalue or lvalue reference.  Both kinds of
 * magic_ptr's have the same type, however, so they can be used interchangeably
 * magic_ptr's are move-only as they can act like a unique_ptr
 */
template<class T>
class magic_ptr {
public:
    using Deleter = std::function<void(T *)>;
private:
    T * t;
    Deleter deleter;

public:


    /**
     * Deleter which does nothing, for non-owning magic_ptr's
     */
    static inline Deleter const noop_deleter = [](T *) {};


    /**
     * Deleter which calls delete on the object, for owning magic_ptr's
     */
    static inline Deleter const default_deleter = [](T * t) { delete t; };


    /**
     * Constructs empty object
     */
    magic_ptr() :
        t(nullptr),
        deleter(magic_ptr::noop_deleter)
    {}

    /**
     * Takes a pointer to an object and a deleter
     * Caller must specify the deleter because it can't otherwise be known if the object should be "owned" or not
     * @param t pointer to object
     * @param deleter
     */
    magic_ptr(T * t, Deleter deleter) :
        t(t),
        deleter(deleter) {}


    /**
     * Non-owning constructor for existing T objects
     * @param t
     */
    magic_ptr(T & t) :
        t(&t),
        deleter(magic_ptr::noop_deleter) {}

    /**
     * Owning constructor calls move constructor for type T
     * @param t object to move out of
     */
    magic_ptr(T && t) :
        t(new T(std::move(t))),
        deleter(magic_ptr::default_deleter) {}


    /**
     * Owning constructor which takes ownership from unique_ptr and uses the unique_ptr's deleter as well.
     * Empty unique_ptr's are fine
     * @tparam UniquePtrDeleter the Deleter type of the unique_ptr
     * @param unique_ptr unique_ptr from which to transfer object ownership
     */
    template<class UniquePtrDeleter>
    magic_ptr(std::unique_ptr<T, UniquePtrDeleter> unique_ptr) :
        t(unique_ptr.release()),
        deleter(UniquePtrDeleter()) {}

    /**
     * Move constructor for magic_ptr
     * @param other
     */
    magic_ptr(magic_ptr && other) :
        t(other.t),
        deleter(other.deleter) {
        other.t = nullptr;
        other.deleter = magic_ptr::noop_deleter;
    }


    magic_ptr(magic_ptr const &) = delete;

    /**
     * Move assignment operator for magic_ptr
     * @param other
     * @return
     */
    magic_ptr & operator=(magic_ptr && other) {
        this->t = other.t;
        this->deleter = other.deleter;

        other.t = nullptr;
        other.deleter = magic_ptr::noop_deleter;
    }


    magic_ptr & operator=(magic_ptr const & other) = delete;


    ~magic_ptr() {
        deleter(t);
    }

    /**
     * Returns the pointer to the contained object, or nullptr if none
     * @return
     */
    T * get() const {
        return this->t;
    }

    /**
     * Returns reference to the contained object
     * @return
     * @throws magic_ptr_exception if magic_ptr is empty
     */
    T & operator*() const {
        if (this->t == nullptr) {
            throw magic_ptr_exception("Cannot dereference nullptr");
        }
        return *t;
    }


    /**
     * Calls deleter (which may do nothing) on contained object and then becomes empty
     * @return
     */
    T * release() {
        this->deleter(this->t);
        this->deleter = magic_ptr::noop_deleter;
        this->t = nullptr;
    }

    bool empty() const {
        return this->t == nullptr;
    }

    operator bool() const {
        return !this->empty();
    }

};


} // end namespace xl