#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
mod bindings;
pub use bindings::*;

/* Allow libsettings_ctx_t to move across threads */
unsafe impl Send for libsettings_ctx_t {}

/* Allow sbp_state_t to move across threads */
unsafe impl Send for sbp_state_t {}
