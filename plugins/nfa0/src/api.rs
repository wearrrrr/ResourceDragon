use std::{
    ffi::{CStr, CString},
    sync::OnceLock,
};

// API defs
// Really don't care about the fields here
#[repr(C)]
pub struct SdkCtx;

pub type LogFn = extern "C" fn(ctx: *mut SdkCtx, fmt: *const i8);

#[repr(C)]
#[derive(Copy, Clone, Debug)]
pub enum RdLogLevel {
    Info = 0,
    Warn = 1,
    Error = 2,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub enum RdLogArgType {
    String = 0,
    Bool = 1,
    S64 = 2,
    U64 = 3,
    F64 = 4,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct RdLogStringArg {
    pub data: *const u8,
    pub size: usize,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub union RdLogArgValue {
    pub string: RdLogStringArg,
    pub boolean: u8,
    pub s64: i64,
    pub u64: u64,
    pub f64: f64,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct RdLogArg {
    pub arg_type: RdLogArgType,
    _padding: [u8; 4],
    pub value: RdLogArgValue,
}

impl RdLogArg {
    pub fn string(s: &str) -> Self {
        Self {
            arg_type: RdLogArgType::String,
            _padding: [0; 4],
            value: RdLogArgValue {
                string: RdLogStringArg {
                    data: s.as_ptr(),
                    size: s.len(),
                },
            },
        }
    }

    pub fn bool(b: bool) -> Self {
        Self {
            arg_type: RdLogArgType::Bool,
            _padding: [0; 4],
            value: RdLogArgValue {
                boolean: b as u8,
            },
        }
    }

    pub fn i64(i: i64) -> Self {
        Self {
            arg_type: RdLogArgType::S64,
            _padding: [0; 4],
            value: RdLogArgValue { s64: i },
        }
    }

    pub fn u64(u: u64) -> Self {
        Self {
            arg_type: RdLogArgType::U64,
            _padding: [0; 4],
            value: RdLogArgValue { u64: u },
        }
    }

    pub fn f64(f: f64) -> Self {
        Self {
            arg_type: RdLogArgType::F64,
            _padding: [0; 4],
            value: RdLogArgValue { f64: f },
        }
    }

    pub fn usize(u: usize) -> Self {
        Self::u64(u as u64)
    }

    pub fn isize(i: isize) -> Self {
        Self::i64(i as i64)
    }
}

pub type LogFmtvFn = extern "C" fn(
    level: RdLogLevel,
    fmt: *const i8,
    args: *const RdLogArg,
    arg_count: usize,
);

#[repr(C)]
pub struct HostApi {
    get_sdk_context: extern "C" fn() -> *mut SdkCtx,
    pub log: LogFn,
    pub warn: LogFn,
    pub error: LogFn,
    pub log_fmtv: LogFmtvFn,
}

pub struct HostApiWrapper {
    api: OnceLock<&'static HostApi>,
}

impl HostApiWrapper {
    pub fn init(&self, api: &'static HostApi) {
        let _ = self.api.set(api);
    }

    pub fn get_sdk_context(&self) -> *mut SdkCtx {
        if let Some(api) = self.api.get() {
            return (api.get_sdk_context)();
        }
        core::ptr::null_mut()
    }

    fn log_base(&self, log_fn: LogFn, message: &str) {
        let ctx = self.get_sdk_context();
        let str = CString::new(message);
        log_fn(
            ctx,
            str.as_ref()
                .map(|s| s.as_ptr())
                .unwrap_or(c"Embedded null in log!!!".as_ptr()),
        );
    }

    #[allow(dead_code)]
    pub fn log(&self, message: &str) {
        if let Some(api) = self.api.get() {
            self.log_base(api.log, message);
        }
    }

    #[allow(dead_code)]
    pub fn warn(&self, message: &str) {
        if let Some(api) = self.api.get() {
            self.log_base(api.warn, message);
        }
    }

    #[allow(dead_code)]
    pub fn error(&self, message: &str) {
        if let Some(api) = self.api.get() {
            self.log_base(api.error, message);
        }
    }

    #[allow(dead_code)]
    pub fn log_fmt(&self, fmt: &str, args: &[RdLogArg]) {
        self.log_fmtv(RdLogLevel::Info, fmt, args);
    }

    #[allow(dead_code)]
    pub fn warn_fmt(&self, fmt: &str, args: &[RdLogArg]) {
        self.log_fmtv(RdLogLevel::Warn, fmt, args);
    }

    #[allow(dead_code)]
    pub fn error_fmt(&self, fmt: &str, args: &[RdLogArg]) {
        self.log_fmtv(RdLogLevel::Error, fmt, args);
    }

    fn log_fmtv(&self, level: RdLogLevel, fmt: &str, args: &[RdLogArg]) {
        if let Some(api) = self.api.get() {
            // SAFETY: hold the CString until after the FFI call completes
            let fmt_c = match CString::new(fmt) {
                Ok(s) => s,
                Err(_) => return,
            };
        
            (api.log_fmtv)(
                level,
                fmt_c.as_ptr(),
                args.as_ptr(),
                args.len(),
            );
        }
    }
}

pub static HOST_API: HostApiWrapper = HostApiWrapper {
    api: OnceLock::new(),
};

pub type ArchiveHandle = *mut core::ffi::c_void;
pub type ArchiveInstance = *mut core::ffi::c_void;

#[repr(C)]
pub struct ArchiveBaseVTable {
    pub get_entry_count: extern "C" fn(inst: ArchiveInstance) -> usize,
    pub get_entry_name: extern "C" fn(inst: ArchiveInstance, index: usize) -> *const i8,
    pub get_entry_size: extern "C" fn(inst: ArchiveInstance, index: usize) -> usize,
    pub open_stream: extern "C" fn(inst: ArchiveInstance, index: usize, out_size: *mut usize) -> *const u8,
    pub destroy: extern "C" fn(inst: *mut ArchiveBaseHandle),
}

#[repr(C)]
pub struct ArchiveBaseHandle {
    pub inst: ArchiveInstance,
    pub vtable: *const ArchiveBaseVTable,
}

impl ArchiveBaseHandle {
    pub fn empty() -> Self {
        Self {
            inst: core::ptr::null_mut(),
            vtable: core::ptr::null(),
        }
    }
}

#[repr(C)]
pub struct ArchiveFormatVTable {
    pub new: extern "C" fn(ctx: *mut SdkCtx) -> ArchiveHandle,
    pub can_handle_file:
        extern "C" fn(inst: ArchiveHandle, buffer: *const u8, size: u64, ext: *const i8) -> i32,
    pub try_open: extern "C" fn(
        inst: ArchiveHandle,
        buffer: *const u8,
        size: u64,
        file_name: *const i8,
    ) -> *mut ArchiveBaseHandle,
    pub get_tag: extern "C" fn(inst: ArchiveHandle) -> *const i8,
    pub get_description: extern "C" fn(inst: ArchiveHandle) -> *const i8,
}

#[repr(transparent)]
pub struct ShutUpAboutSync(pub *const i8);
unsafe impl Sync for ShutUpAboutSync {}

extern "C" fn get_entry_count_wrapper<T: RdFormat<'static> + 'static>(
    inst: ArchiveInstance,
) -> usize {
    if inst.is_null() {
        return 0;
    }

    let archive = unsafe { &*(inst as *const T) };
    archive.get_entry_count()
}

extern "C" fn get_entry_name_wrapper<T: RdFormat<'static> + 'static>(
    inst: ArchiveInstance,
    index: usize,
) -> *const i8 {
    if inst.is_null() {
        return core::ptr::null();
    }

    let archive = unsafe { &*(inst as *const T) };
    if let Some(str) = archive.get_entry_name(index) {
        str.as_ptr()
    } else {
        core::ptr::null()
    }
}

extern "C" fn get_entry_size_wrapper<T: RdFormat<'static> + 'static>(
    inst: ArchiveInstance,
    index: usize,
) -> usize {
    if inst.is_null() {
        return 0;
    }

    let archive = unsafe { &*(inst as *const T) };
    archive.get_entry_size(index)
}

extern "C" fn open_stream_wrapper<T: RdFormat<'static> + 'static>(
    inst: ArchiveInstance,
    index: usize,
    out_size: *mut usize,
) -> *const u8 {
    if inst.is_null() {
        return core::ptr::null();
    }

    let archive = unsafe { &mut *(inst as *mut T) };
    let ret = archive.open_stream(index);

    if let Some(data) = ret {
        if !out_size.is_null() {
            unsafe {
                *out_size = data.len();
            }
        }
        
        data.leak() as *mut _ as *const _
    } else {
        if !out_size.is_null() {
            unsafe {
                *out_size = 0;
            }
        }
        core::ptr::null()
    }
}

extern "C" fn destroy_wrapper<T: RdFormat<'static> + 'static>(handle: *mut ArchiveBaseHandle) {
    if handle.is_null() {
        return;
    }

    unsafe {
        if !(*handle).inst.is_null() {
            let inst = Box::from_raw((*handle).inst as *mut T);
            drop(inst);
        }
    }
}

pub trait RdFormat<'a: 'static>: Sized
where
    Self: 'static,
{
    fn plugin_init() -> bool;
    fn plugin_shutdown();

    fn tag() -> &'static CStr;
    fn description() -> &'static CStr;

    fn can_handle_file(buffer: &[u8], ext: &CStr) -> bool;
    fn try_open(buffer: &[u8], file_name: &CStr) -> Option<Self>;

    fn get_entry_count(&'a self) -> usize;
    fn get_entry_name(&'a self, idx: usize) -> Option<&'a CStr>;
    fn get_entry_size(&'a self, idx: usize) -> usize;
    fn open_stream(&'a mut self, idx: usize) -> Option<Vec<u8>>;

    const BASE_VTABLE: ArchiveBaseVTable = ArchiveBaseVTable {
        get_entry_count: get_entry_count_wrapper::<Self>,
        get_entry_name: get_entry_name_wrapper::<Self>,
        get_entry_size: get_entry_size_wrapper::<Self>,
        open_stream: open_stream_wrapper::<Self>,
        destroy: destroy_wrapper::<Self>,
    };

    const FORMAT_VTABLE: ArchiveFormatVTable = ArchiveFormatVTable {
        new: new_wrapper::<Self>,
        can_handle_file: can_handle_file_wrapper::<Self>,
        try_open: try_open_wrapper::<Self>,
        get_tag: get_tag_wrapper::<Self>,
        get_description: get_description_wrapper::<Self>,
    };
}

#[repr(transparent)]
struct FormatDummy;
extern "C" fn new_wrapper<T: RdFormat<'static> + 'static>(_ctx: *mut SdkCtx) -> ArchiveHandle {
    Box::leak(Box::new(FormatDummy {})) as *mut _ as ArchiveHandle
}

extern "C" fn can_handle_file_wrapper<T: RdFormat<'static> + 'static>(
    _inst: ArchiveHandle,
    buffer: *const u8,
    size: u64,
    ext: *const i8,
) -> i32 {
    let buffer = if !buffer.is_null() {
        unsafe { std::slice::from_raw_parts(buffer, size as usize) }
    } else {
        return 0;
    };
    let ext = if !ext.is_null() {
        unsafe { CStr::from_ptr(ext) }
    } else {
        return 0;
    };

    T::can_handle_file(buffer, ext) as i32
}

extern "C" fn try_open_wrapper<T: RdFormat<'static> + 'static>(
    _inst: ArchiveHandle,
    buffer: *const u8,
    size: u64,
    file_name: *const i8,
) -> *mut ArchiveBaseHandle {
    let buffer = if !buffer.is_null() {
        unsafe { std::slice::from_raw_parts(buffer, size as usize) }
    } else {
        return core::ptr::null_mut();
    };
    let file_name = if !file_name.is_null() {
        unsafe { CStr::from_ptr(file_name) }
    } else {
        return core::ptr::null_mut();
    };

    let Some(arc) = T::try_open(buffer, file_name) else {
        return core::ptr::null_mut();
    };

    let arc = Box::leak(Box::new(arc)) as *mut _;
    Box::leak(Box::new(ArchiveBaseHandle {
        inst: arc as ArchiveInstance,
        vtable: &T::BASE_VTABLE as *const _,
    }))
}

extern "C" fn get_tag_wrapper<T: RdFormat<'static> + 'static>(_inst: ArchiveHandle) -> *const i8 {
    T::tag().as_ptr()
}

extern "C" fn get_description_wrapper<T: RdFormat<'static> + 'static>(
    _inst: ArchiveHandle,
) -> *const i8 {
    T::description().as_ptr()
}

#[macro_export]
macro_rules! register_plugin {
    ($name:expr, $version:expr, $ty:ident) => {
        #[unsafe(no_mangle)]
        #[allow(non_upper_case_globals)]
        pub static RD_PluginName: crate::api::ShutUpAboutSync =
            crate::api::ShutUpAboutSync($name.as_ptr());

        #[unsafe(no_mangle)]
        #[allow(non_upper_case_globals)]
        pub static RD_PluginVersion: crate::api::ShutUpAboutSync =
            crate::api::ShutUpAboutSync($version.as_ptr());

        #[unsafe(no_mangle)]
        pub extern "C" fn RD_PluginInit(api: *mut crate::api::HostApi) -> bool {
            if api.is_null() {
                return false;
            }
            let api = unsafe { &*api };
            crate::api::HOST_API.init(api);

            $ty::plugin_init()
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn RD_PluginShutdown() {
            $ty::plugin_shutdown();
        }

        #[unsafe(no_mangle)]
        pub extern "C" fn RD_GetArchiveFormat(
            _ctx: *const crate::api::SdkCtx,
        ) -> *const crate::api::ArchiveFormatVTable {
            &$ty::FORMAT_VTABLE as *const _
        }
    };
}
