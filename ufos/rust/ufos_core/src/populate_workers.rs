use std::sync::{Arc, Condvar, Mutex};

#[derive(Debug, PartialEq)]
pub enum ShouldRun {
    Running,
    Shutdown,
}

pub(crate) trait RequestWorker {
    fn await_work(&self) -> ShouldRun;
    fn request_worker(&self);
}

struct PoolState {
    workers_waiting: u32,
    workers_requested: u32,
    should_run: bool,
}
pub(crate) struct PopulateWorkers<F> {
    name: String,
    state: Mutex<PoolState>,
    awake: Condvar,
    work: F,
}

impl<F> PopulateWorkers<F> {
    pub fn shutdown(&self) {
        let mut state = self.state.lock().unwrap();
        state.should_run = false;
        self.awake.notify_all();
    }
}

impl<F> RequestWorker for Arc<PopulateWorkers<F>>
where
    F: 'static + Send + Sync + Fn(&dyn RequestWorker),
{
    fn await_work(&self) -> ShouldRun {
        let mut state = self.state.lock().unwrap();

        state.workers_waiting += 1;

        let mut state = self
            .awake
            .wait_while(state, |s| s.workers_requested == 0 && s.should_run)
            .unwrap();

        state.workers_waiting -= 1;

        if !state.should_run {
            return ShouldRun::Shutdown;
        }

        state.workers_requested -= 1;

        if state.workers_waiting < 1 {
            PopulateWorkers::spawn_worker(Arc::clone(self));
        }

        std::sync::Mutex::unlock(state);
        ShouldRun::Running
    }

    fn request_worker(&self) {
        self.state.lock().unwrap().workers_requested += 1;
        self.awake.notify_one();
    }
}

impl<F> PopulateWorkers<F>
where
    F: 'static + Send + Sync + Fn(&dyn RequestWorker),
{
    pub fn new(name: &str, work: F) -> Arc<PopulateWorkers<F>> {
        Arc::new(PopulateWorkers {
            name: name.to_string(),
            state: Mutex::new(PoolState {
                workers_waiting: 0,
                workers_requested: 1,
                should_run: true,
            }),
            awake: Condvar::new(),
            work,
        })
    }

    pub fn spawn_worker(this: Arc<Self>) {
        std::thread::Builder::new()
            .name(this.name.clone())
            .spawn(move || (this.work)(&this))
            .unwrap();
    }
}
