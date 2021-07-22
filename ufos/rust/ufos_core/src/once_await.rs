use std::sync::{
    atomic::{AtomicPtr, Ordering},
    Arc, Condvar, Mutex,
};

pub(crate) struct OnceAwait<T> {
    value: AtomicPtr<T>,
    mutex: Mutex<()>,
    condition: Condvar,
}

pub(crate) trait OnceFulfiller<T> {
    fn try_init(&self, value: T);
}

impl<T> OnceFulfiller<T> for OnceAwait<T> {
    fn try_init(&self, value: T) {
        let current = self.value.load(Ordering::Acquire);
        if std::ptr::null_mut() != current {
            return; // already initialized
        }

        // needs initialization
        let boxed = Box::new(value);
        let as_ptr = Box::into_raw(boxed);
        let res = self.value.compare_exchange(
            std::ptr::null_mut(),
            as_ptr,
            Ordering::Release,
            Ordering::Relaxed,
        );
        match res {
            Ok(_) => {
                // on success we wake everyone up!
                let guard = self.mutex.lock().unwrap();
                self.condition.notify_all();
                Mutex::unlock(guard);
            }
            Err(_) => {
                // On an error we did not initialize the value, reconstruct and drop the box
                unsafe { Box::from_raw(as_ptr) };
            }
        }
    }
}

impl<T> OnceFulfiller<T> for Arc<OnceAwait<T>> {
    fn try_init(&self, value: T) {
        let s = &**self;
        s.try_init(value);
    }
}

impl<T> OnceAwait<T> {
    pub fn new() -> Self {
        OnceAwait {
            value: AtomicPtr::new(std::ptr::null_mut()),
            mutex: Mutex::new(()),
            condition: Condvar::new(),
        }
    }

    pub fn get(&self) -> &T {
        let ptr = self.value.load(Ordering::Acquire);
        if std::ptr::null_mut() != ptr {
            return unsafe { &*self.value.load(Ordering::Acquire) };
        }

        let guard = self.mutex.lock().unwrap();
        let _guard = self
            .condition
            .wait_while(guard, |_| {
                std::ptr::null_mut() == self.value.load(Ordering::Acquire)
            })
            .unwrap();
        unsafe { &*self.value.load(Ordering::Acquire) }
    }
}
