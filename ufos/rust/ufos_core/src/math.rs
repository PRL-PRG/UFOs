use num::Integer;

pub fn up_to_nearest<T>(x: T, nearest: T) -> T
where
    T: Integer,
{
    x.div_ceil(&nearest) * nearest
}

pub fn down_to_nearest<T>(x: T, nearest: T) -> T
where
    T: Integer,
{
    x.div_floor(&nearest) * nearest
}
