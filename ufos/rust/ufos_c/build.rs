extern crate cbindgen;

use std::env;

fn main() {
    let crate_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let mut config: cbindgen::Config = Default::default();
    config.language = cbindgen::Language::C;

    cbindgen::Builder::new()
        .with_pragma_once(true)
        .with_config(config)
        .with_crate(&crate_dir)
        .generate()
        .unwrap()
        .write_to_file("target/ufos_c.h");
}
