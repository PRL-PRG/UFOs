use thiserror::Error;

#[derive(Error, Debug)]
pub enum UfoLookupErr {
    #[error("Core shutdown")]
    CoreShutdown,
    #[error("Core non functional, {0}")]
    CoreBroken(String),
    #[error("Ufo Lock is broken")]
    UfoLockBroken,
    #[error("Ufo not found")]
    UfoNotFound,
}

impl<T> From<std::sync::PoisonError<T>> for UfoLookupErr {
    fn from(_: std::sync::PoisonError<T>) -> Self {
        UfoLookupErr::UfoLockBroken
    }
}

impl<T> From<std::sync::mpsc::SendError<T>> for UfoLookupErr {
    fn from(_e: std::sync::mpsc::SendError<T>) -> Self {
        UfoLookupErr::CoreBroken("Error when sending messsge to the core".into())
    }
}

impl<T> From<crossbeam::channel::SendError<T>> for UfoLookupErr {
    fn from(_e: crossbeam::channel::SendError<T>) -> Self {
        UfoLookupErr::CoreBroken("Error when sending messsge to the core".into())
    }
}

#[derive(Error, Debug)]
pub enum UfoAllocateErr {
    #[error("Could not send message, messaage channel broken")]
    MessageSendError,
    #[error("Could not recieve message")]
    MessageRecvError,
}

impl<T> From<std::sync::mpsc::SendError<T>> for UfoAllocateErr {
    fn from(_e: std::sync::mpsc::SendError<T>) -> Self {
        UfoAllocateErr::MessageSendError
    }
}

impl From<std::sync::mpsc::RecvError> for UfoAllocateErr {
    fn from(_e: std::sync::mpsc::RecvError) -> Self {
        UfoAllocateErr::MessageSendError
    }
}

impl<T> From<crossbeam::channel::SendError<T>> for UfoAllocateErr {
    fn from(_e: crossbeam::channel::SendError<T>) -> Self {
        UfoAllocateErr::MessageSendError
    }
}

impl From<crossbeam::channel::RecvError> for UfoAllocateErr {
    fn from(_e: crossbeam::channel::RecvError) -> Self {
        UfoAllocateErr::MessageSendError
    }
}
