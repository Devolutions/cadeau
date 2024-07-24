macro_rules! external_library(
    (
        feature = $feature:expr, $structname:ident, $link:expr,
        $(statics: $($sname:ident: $stype:ty),+,)|*
        $(functions: $(fn $fname:ident($($fargn:ident: $fargt:ty),*) -> $fret:ty),+,)|*
        $(varargs: $(fn $vname:ident($($vargs:ty),+) -> $vret:ty),+,)|*
    ) => (
        ::dlib::external_library!(
            feature=$feature, $structname, $link,
            $(statics: $($sname: $stype),+,)|*
            $(functions: $(fn $fname($($fargt),*) -> $fret),+,)|*
            $(varargs: $(fn $vname($($vargs),+) -> $vret),+,)|*
        );

        $crate::macros::__global_wrapper!(
            feature = $feature, $structname, $link,
            $(functions: $(fn $fname($($fargn: $fargt),*) -> $fret),+,)|*
        );
    );
);

pub(crate) use external_library;

macro_rules! __global_wrapper {
    (
        feature = $feature:expr, $link:expr,
        fn $fname:ident($($fargn:ident: $fargt:ty),*) -> $fret:ty
    ) => {
        #[doc=::core::concat!("Performs a FFI call to ", ::core::stringify!($fname), " using the dynamic library ", $link, " loaded and installed globally")]
        #[doc=""]
        #[doc="# Safety"]
        #[doc=""]
        #[doc="- The ABI of the function must be the one we expect"]
        #[doc="- See other preconditions in documentation of the C library"]
        pub unsafe extern "C" fn $fname($($fargn: $fargt),*) -> $fret {
            #[cfg(feature = $feature)]
            // SAFETY: Same preconditions as defined above.
            unsafe {
                (API.get().expect(::core::concat!($link, " library must be loaded and installed globally")).$fname)($($fargn),*)
            }

            #[cfg(not(feature = $feature))]
            // SAFETY: Same preconditions as defined above.
            unsafe {
                super::$fname($($fargn),*)
            }
        }
    };
    (
        feature = $feature:expr, $structname:ident, $link:expr,
        $(functions: $(fn $fname:ident($($fargn:ident: $fargt:ty),*) -> $fret:ty),+,)|*
    ) => {
        #[allow(unreachable_pub)]
        pub mod global {
            use super::*;

            #[cfg(feature = $feature)]
            static API: ::std::sync::OnceLock<$structname> = ::std::sync::OnceLock::new();

            /// Loads and installs the shared library globally.
            ///
            /// Returns true if the library was loaded and installed by this call and false if it was already installed
            /// by a previous call.
            ///
            /// # Safety
            ///
            /// When a library is loaded, initialisation routines contained within it are executed. For the purposes of safety, the execution of these routines is conceptually the same calling an unknown foreign function and may impose arbitrary requirements on the caller for the call to be sound.
            ///
            /// Additionally, the callers of this function must also ensure that execution of the termination routines contained within the library is safe as well. These routines may be executed when the library is unloaded.
            #[cfg(feature = $feature)]
            pub unsafe fn init(name: &str) -> ::core::result::Result<bool, ::dlib::DlError> {
                // SAFETY: Same preconditions as defined above.
                let api = unsafe { $structname::open(name)? };

                let installed = API.set(api).is_ok();

                ::core::result::Result::Ok(installed)
            }

            #[inline]
            pub fn is_init() -> bool {
                #[cfg(feature = $feature)]
                {
                    API.get().is_some()
                }

                #[cfg(not(feature = $feature))]
                {
                    true
                }
            }

            $($(
                $crate::macros::__global_wrapper!(feature = $feature, $link, fn $fname($($fargn: $fargt),*) -> $fret);
            )+)*
        }
    };
}

pub(crate) use __global_wrapper;
