use crate::api::RdLogArg;

// -------------------- Automatic type conversion trait --------------------

pub trait IntoLogArg {
    fn into_log_arg(self) -> RdLogArg;
}

impl IntoLogArg for &str {
    fn into_log_arg(self) -> RdLogArg {
        RdLogArg::string(self)
    }
}

impl IntoLogArg for bool {
    fn into_log_arg(self) -> RdLogArg {
        RdLogArg::bool(self)
    }
}

macro_rules! impl_into_log_arg_int {
    ($($t:ty => $method:ident),* $(,)?) => {
        $(
            impl IntoLogArg for $t {
                fn into_log_arg(self) -> RdLogArg {
                    RdLogArg::$method(self as _)
                }
            }
        )*
    };
}

impl_into_log_arg_int! {
    i8 => i64,
    i16 => i64,
    i32 => i64,
    i64 => i64,
    isize => isize,
    u8 => u64,
    u16 => u64,
    u32 => u64,
    u64 => u64,
    usize => usize,
    f32 => f64,
    f64 => f64,
}

// -------------------- Convenience macros --------------------

#[macro_export]
macro_rules! rd_log {
    ($fmt:expr) => {
        $crate::api::HOST_API.log_fmt($fmt, &[])
    };
    ($fmt:expr, $($arg:expr),+ $(,)?) => {{
        let __args = [$($crate::logging::IntoLogArg::into_log_arg($arg)),+];
        $crate::api::HOST_API.log_fmt($fmt, &__args)
    }};
}

#[macro_export]
macro_rules! rd_warn {
    ($fmt:expr) => {
        $crate::api::HOST_API.warn_fmt($fmt, &[])
    };
    ($fmt:expr, $($arg:expr),+ $(,)?) => {{
        let __args = [$($crate::logging::IntoLogArg::into_log_arg($arg)),+];
        $crate::api::HOST_API.warn_fmt($fmt, &__args)
    }};
}

#[macro_export]
macro_rules! rd_error {
    ($fmt:expr) => {
        $crate::api::HOST_API.error_fmt($fmt, &[])
    };
    ($fmt:expr, $($arg:expr),+ $(,)?) => {{
        let __args = [$($crate::logging::IntoLogArg::into_log_arg($arg)),+];
        $crate::api::HOST_API.error_fmt($fmt, &__args)
    }};
}
